#if defined(__XSDK__)
#include <limits.h>
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xsdk/gpu.h"
#include "xsdk/log.h"
#include "xsdk/shader.h"
#include "xsdk/texture.h"
#include "gfx_cc.h"
#include "gfx_xsdk.h"
#include "gfx_xenos.h"
#include "coopdx_shaders.h"
#include "material_shaders.h"

enum { VERTEX_WORDS = 48, VERTEX_CAPACITY = 4096, ARENA_VERTICES = 32768 };
struct ShaderProgram { uint64_t hash; struct CCFeatures features; struct CombineMode mode; uint8_t commands[16]; int material; };
struct Texture { XsdkTexture image; XsdkSamplerState sampler; bool ready; };
static struct ShaderProgram programs[CC_MAX_SHADERS], *current;
static unsigned program_count, program_next;
static struct Texture *textures;
static uint32_t texture_count, selected[2], upload_tile;
static XsdkShader vertex_shader, pixel_shader, upscale_vertex, upscale_pixel;
static XsdkTexture scene_texture;
static XsdkSamplerState scene_sampler;
static volatile uint32_t *upscale_vertices;
static XsdkShader material_shaders[MATERIAL_COUNT];
static bool material_ready[MATERIAL_COUNT];
static unsigned stat_specialized;
static volatile uint32_t *vertices;
static size_t vertex_cursor;
static XsdkTexture retired[64];
static unsigned retired_count, stat_draws, stat_uploads, stat_waits;
static uint64_t retired_bytes, stat_wait_ns, stat_upload_ns;
static XsdkTexture white;
static XsdkSamplerState white_sampler;
static uint32_t width, height;
static bool depth_test, depth_write, alpha_blend, decal;
static int viewport[4], scissor[4];
extern unsigned char gVertexColor[3];
static void fail(const char *name) { fprintf(stderr, "Xenos renderer: %s\n", name); exit(EXIT_FAILURE); }
static void checked(NTSTATUS status, const char *name) { if (!NT_SUCCESS(status)) fail(name); }
static void wait_gpu(void) {
    uint64_t before = SDL_GetTicksNS();
    checked(xsdk_gpu_wait(xsdk_gpu_fence(), 5000), "GPU fence timed out");
    stat_wait_ns += SDL_GetTicksNS() - before; ++stat_waits;
    // All commands referencing retired allocations have now finished.
    for (unsigned i=0; i<retired_count; ++i) xsdk_texture_free(&retired[i]);
    retired_count = 0; retired_bytes = 0;
}
static bool z_range(void) { return true; }
static void unload(struct ShaderProgram *old) { (void)old; current = NULL; }
static void load(struct ShaderProgram *program) { current = program; }
static struct ShaderProgram *lookup(struct ColorCombiner *cc) {
    for (unsigned i = 0; i < program_count; i++) if (programs[i].hash == cc->hash) return &programs[i];
    return NULL;
}
static struct ShaderProgram *create(struct ColorCombiner *cc) {
    struct ShaderProgram *p = lookup(cc);
    if (!p) {
        p = program_count < CC_MAX_SHADERS ? &programs[program_count++] : &programs[program_next++ % CC_MAX_SHADERS]; p->hash = cc->hash; p->mode = cc->cm;
        memcpy(p->commands, cc->shader_commands, sizeof p->commands);
        p->material=-1;
        for (unsigned i=0;i<MATERIAL_COUNT;++i) {
            if (memcmp(p->commands,material_definitions[i].commands,16)==0) {p->material=(int)i;break;}
        }
        gfx_cc_get_features(cc, &p->features);
        if (p->features.num_inputs > 8) fail("more than eight combiner inputs");
    }
    current = p;
    return p;
}
static void info(struct ShaderProgram *p, uint8_t *inputs, bool used[2]) {
    *inputs = (uint8_t)p->features.num_inputs;
    memcpy(used, p->features.used_textures, sizeof p->features.used_textures);
}
static uint32_t new_texture(void) {
    if (texture_count == UINT32_MAX) fail("texture ID overflow");
    struct Texture *next = realloc(textures, (size_t)(texture_count + 1) * sizeof *next);
    if (!next) fail("texture cache allocation");
    textures = next; memset(&textures[texture_count], 0, sizeof *textures);
    return texture_count++;
}
static void select_texture(int tile, uint32_t id) {
    if ((unsigned)tile > 1 || id >= texture_count) fail("invalid texture selection");
    selected[tile] = id; upload_tile = (uint32_t)tile;
}
static void sampler(int tile, bool linear, uint32_t cms, uint32_t cmt) {
    if ((unsigned)tile > 1 || selected[tile] >= texture_count) fail("invalid sampler");
    XsdkSampler state = {0};
    state.mag_filter = state.min_filter = linear ? XSDK_FILTER_LINEAR : XSDK_FILTER_POINT;
    state.address_u = (cms & 2) ? XSDK_ADDRESS_CLAMP : (cms & 1) ? XSDK_ADDRESS_MIRROR : XSDK_ADDRESS_WRAP;
    state.address_v = (cmt & 2) ? XSDK_ADDRESS_CLAMP : (cmt & 1) ? XSDK_ADDRESS_MIRROR : XSDK_ADDRESS_WRAP;
    xsdk_texture_sampler_state(&textures[selected[tile]].sampler, &state);
    upload_tile = (uint32_t)tile;
}
static void upload(const uint8_t *rgba, int w, int h) {
    static unsigned uploads;
    const unsigned uploadId=uploads++;
    if (uploadId<24) xsdk_log_write("texture %u: tile %u id %u size %dx%d source %08X",uploadId,upload_tile,selected[upload_tile],w,h,(unsigned)(uintptr_t)rgba);
    if (selected[upload_tile] >= texture_count || !rgba || w <= 0 || h <= 0 || w > 8192 || h > 8192) fail("invalid texture upload");
    struct Texture *t = &textures[selected[upload_tile]];
    uint64_t before = SDL_GetTicksNS();
    if (t->ready) {
        if (retired_count == 64 || retired_bytes + t->image.size_bytes > 16 * 1024 * 1024) wait_gpu();
        retired[retired_count++] = t->image;
        retired_bytes += t->image.size_bytes;
    }
    // Allocate separately: queued draws still own the previous texture storage.
    checked(xsdk_texture_create(&t->image, XSDK_FORMAT_R8G8B8A8, (uint32_t)w, (uint32_t)h, 1, true), "texture allocation");
    t->ready = true;
    xsdk_texture_upload(&t->image, 0, rgba, (uint32_t)w * 4);
    xsdk_gpu_invalidate(t->image.address, t->image.size_bytes);
    stat_upload_ns += SDL_GetTicksNS() - before; ++stat_uploads;
    if (uploadId<24) xsdk_log_write("texture %u: uploaded",uploadId);
}
static void set_depth(bool enabled) { depth_test = enabled; }
static void set_depth_write(bool enabled) { depth_write = enabled; }
static void set_decal(bool enabled) { decal = enabled; }
static void set_alpha(bool enabled) { alpha_blend = enabled; }
static void set_viewport(int x, int y, int w, int h) { viewport[0]=x; viewport[1]=y; viewport[2]=w; viewport[3]=h; }
static void set_scissor(int x, int y, int w, int h) { scissor[0]=x; scissor[1]=y; scissor[2]=w; scissor[3]=h; }
static void begin(void) {
    static uint64_t last;
    uint64_t now = SDL_GetTicksNS();
    if (now - last >= 5000000000ULL) {
        xsdk_log_write("render stats: draws %u specialized %u uploads %u upload_ms %u waits %u wait_ms %u", stat_draws, stat_specialized, stat_uploads, (unsigned)(stat_upload_ns/1000000), stat_waits, (unsigned)(stat_wait_ns/1000000));
        stat_draws=stat_uploads=stat_waits=stat_specialized=0; stat_upload_ns=stat_wait_ns=0; last=now;
    }
    gfx_xsdk.get_dimensions(&width, &height);
    xsdk_gpu_set_target(width, height);
    // XAM may overwrite both color and depth in eDRAM. Discard that content
    // before the next game pass, never treating it as game depth or color.
    static uint32_t seen_system_draws;
    static bool initialized_target;
    XsdkGpuStats gpu;xsdk_gpu_stats(&gpu);
    if(!initialized_target || gpu.system_draws!=seen_system_draws) {
        xsdk_gpu_resolve_texture(&scene_texture,0xFF000000);
        xsdk_gpu_set_target(width,height);
        seen_system_draws=gpu.system_draws;initialized_target=true;
    }
}
static void draw(float data[], size_t count, size_t triangles) {
    static unsigned draws;
    const unsigned drawId=draws++;
    if (drawId<24) xsdk_log_write("draw %u: floats %u triangles %u shader %08X",drawId,(unsigned)count,(unsigned)triangles,(unsigned)(uintptr_t)current);
    if (!current || !triangles) return;
    ++stat_draws;
    if (triangles > VERTEX_CAPACITY / 3) fail("vertex batch exceeds capacity");
    unsigned stride = 4 + 2 * (current->features.used_textures[0] + current->features.used_textures[1]) +
        (current->mode.use_fog ? 4 : 0) + (current->mode.light_map ? 2 : 0) +
        current->features.num_inputs * (current->mode.use_alpha ? 4 : 3);
    size_t n = triangles * 3;
    if (count != n * stride) fail("engine vertex layout mismatch");
    // Append batches without a CPU/GPU round trip. Fence only before reusing
    // the arena, so every queued draw retains its own vertex data.
    if (vertex_cursor + n > ARENA_VERTICES) {
        wait_gpu();
        vertex_cursor = 0;
    }
    volatile uint32_t *batch = vertices + vertex_cursor * VERTEX_WORDS;
    vertex_cursor += n;
    const float *src = data;
    for (size_t v = 0; v < n; v++) {
        float packed[VERTEX_WORDS] = {0};
        memcpy(packed, src, 4*sizeof(float)); src += 4;
        for (unsigned t=0; t<2; t++) if (current->features.used_textures[t]) {
            memcpy(packed+4+2*t, src, 2*sizeof(float)); src += 2;
        }
        if (current->mode.use_fog) { memcpy(packed+8, src, 4*sizeof(float)); src += 4; }
        if (current->mode.light_map) { memcpy(packed+12, src, 2*sizeof(float)); src += 2; }
        for (int i=0; i<8 && i<current->features.num_inputs; i++) {
            unsigned components = current->mode.use_alpha ? 4 : 3;
            memcpy(packed+16+4*i, src, components*sizeof(float)); src += components;
            if (components == 3) packed[16+4*i+3] = 1;
        }
        uint32_t words[VERTEX_WORDS]; memcpy(words, packed, sizeof words);
        for (unsigned j=0; j<VERTEX_WORDS; j++) batch[v*VERTEX_WORDS+j] = words[j];
    }
    const uint32_t bytes = (uint32_t)(n * VERTEX_WORDS * 4);
    xsdk_gpu_invalidate(xsdk_gpu_address(batch), bytes);
    const XsdkBuiltShader *built=&coopdx_frag;
    XsdkShader *pixel=&pixel_shader;
    unsigned push=COOPDX_FRAG_PUSH, registers=COOPDX_FRAG_PUSH_REGISTERS;
    int slots[2]={COOPDX_FRAG_SAMPLER_0_0,COOPDX_FRAG_SAMPLER_0_1};
    if(current->material>=0) {
        unsigned i=(unsigned)current->material;
        const struct MaterialDefinition *material=&material_definitions[i];
        if(!material_ready[i]) {
            checked(xsdk_shader_load(&material_shaders[i],material->code->container,material->code->container_bytes),"material shader load");
            material_ready[i]=true;
        }
        built=material->code;pixel=&material_shaders[i];push=material->push;registers=material->registers;
        slots[0]=material->sampler[0];slots[1]=material->sampler[1];++stat_specialized;
    }
    if(registers>6) fail("material push constants exceed parameter block");
    xsdk_gpu_set_shaders(&vertex_shader, pixel);
    // Restore alongside each binding; the system UI and resolves also use GPU constants.
    xsdk_built_shader_set_literals(&coopdx_vert);
    xsdk_built_shader_set_literals(built);
    float constants[24] = {0};
    for (unsigned j=0; j<16; j++) constants[j]=(float)current->commands[j];
    constants[16]=current->mode.use_2cycle; constants[17]=current->mode.use_alpha;
    constants[18]=current->mode.texture_edge; constants[19]=current->mode.use_fog;
    for (unsigned j=0; j<3; j++) constants[20+j]=gVertexColor[j]/255.0f;
    constants[23]=current->mode.light_map;
    xsdk_gpu_set_pixel_constants(push, constants, registers);
    for (unsigned t=0; t<2; t++) {
        if(slots[t]<0) continue;
        struct Texture *image = selected[t] < texture_count ? &textures[selected[t]] : NULL;
        xsdk_gpu_set_texture((unsigned)slots[t],
            image && image->ready ? &image->image : &white,
            image && image->ready ? &image->sampler : &white_sampler);
    }
    XsdkDepthStencil depth = {0}; XsdkDepthStencilState ds;
    depth.depth_test=depth_test; depth.depth_write=depth_test && depth_write; depth.depth_compare=XSDK_COMPARE_LESS_EQUAL;
    xsdk_gpu_depth_stencil_state(&ds, &depth); xsdk_gpu_set_depth_stencil(&ds);
    XsdkBlend blend = {0}; XsdkBlendState bs;
    blend.color_source=alpha_blend ? XSDK_BLEND_SRC_ALPHA : XSDK_BLEND_ONE;
    blend.color_dest=alpha_blend ? XSDK_BLEND_ONE_MINUS_SRC_ALPHA : XSDK_BLEND_ZERO;
    blend.alpha_source=XSDK_BLEND_ONE; blend.alpha_dest=XSDK_BLEND_ZERO; blend.write_mask=XSDK_WRITE_ALL;
    xsdk_gpu_blend_state(&bs, &blend); xsdk_gpu_set_blend(&bs);
    XsdkRaster raster = {0}; XsdkRasterState rs;
    // Xenos takes normalized depth bias; OpenGL takes integer depth-buffer units.
    raster.depth_bias=decal ? -2.0f / 16777215.0f : 0; raster.depth_bias_slope=decal ? -2 : 0;
    xsdk_gpu_raster_state(&rs, &raster); xsdk_gpu_set_raster(&rs);
    xsdk_gpu_set_viewport(viewport[0], (float)height-viewport[1]-viewport[3], viewport[2], viewport[3], 0, 1);
    int x=scissor[0], y=(int)height-scissor[1]-scissor[3], w=scissor[2], h=scissor[3];
    if (x<0) { w+=x; x=0; } if (y<0) { h+=y; y=0; }
    if (x>(int)width || y>(int)height || w<=0 || h<=0) return;
    if (w>(int)width-x) w=(int)width-x;
    if (h>(int)height-y) h=(int)height-y;
    xsdk_gpu_set_scissor((uint32_t)x,(uint32_t)y,(uint32_t)w,(uint32_t)h);
    xsdk_gpu_set_vertices(0, xsdk_gpu_address(batch), bytes);
    xsdk_gpu_draw(XSDK_PRIMITIVE_TRIANGLE_LIST, (uint32_t)n);
    if (drawId<24) xsdk_log_write("draw %u: submitted",drawId);
}
// The game uses a smaller render target. XAM always receives the full 720p
// front buffer, so opening the Guide doesn't change the display's mode or layout.
void gfx_xenos_present(uint32_t address, uint32_t output_width, uint32_t output_height) {
    xsdk_gpu_resolve_texture(&scene_texture,0xFF000000);
    xsdk_gpu_invalidate(scene_texture.address,scene_texture.size_bytes);
    xsdk_gpu_set_target(output_width,output_height);
    xsdk_gpu_set_shaders(&upscale_vertex,&upscale_pixel);
    xsdk_built_shader_set_literals(&upscale_vert);
    xsdk_built_shader_set_literals(&upscale_frag);
    XsdkDepthStencil depth={0};XsdkDepthStencilState ds;
    xsdk_gpu_depth_stencil_state(&ds,&depth);xsdk_gpu_set_depth_stencil(&ds);
    XsdkBlend blend={0};XsdkBlendState bs;
    blend.color_source=blend.alpha_source=XSDK_BLEND_ONE;blend.write_mask=XSDK_WRITE_ALL;
    xsdk_gpu_blend_state(&bs,&blend);xsdk_gpu_set_blend(&bs);
    XsdkRaster raster={0};XsdkRasterState rs;
    xsdk_gpu_raster_state(&rs,&raster);xsdk_gpu_set_raster(&rs);
    xsdk_gpu_set_viewport(0,0,output_width,output_height,0,1);
    xsdk_gpu_set_scissor(0,0,output_width,output_height);
    xsdk_gpu_set_texture(UPSCALE_FRAG_SAMPLER_0_0,&scene_texture,&scene_sampler);
    xsdk_gpu_set_vertices(0,xsdk_gpu_address(upscale_vertices),6*4*sizeof(float));
    xsdk_gpu_draw(XSDK_PRIMITIVE_TRIANGLE_LIST,6);
    xsdk_gpu_resolve(address,output_width,output_height,0xFF000000);
}
static void init(void) {
    checked(xsdk_shader_load(&vertex_shader, coopdx_vert.container, coopdx_vert.container_bytes), "vertex shader load");
    checked(xsdk_shader_load(&pixel_shader, coopdx_frag.container, coopdx_frag.container_bytes), "pixel shader load");
    XsdkVertexElement layout[12] = {0};
    for (unsigned i=0; i<12; i++) { layout[i].usage=i==0 ? XSDK_USAGE_POSITION : i==1 ? XSDK_USAGE_COLOR : XSDK_USAGE_TEXCOORD; layout[i].usage_index=i<2 ? 0 : i-2; layout[i].format=XSDK_VERTEX_FLOAT4; layout[i].offset_bytes=i*16; }
    const uint32_t stride = VERTEX_WORDS*4;
    checked(xsdk_shader_set_vertex_layout(&vertex_shader,layout,12,&stride), "vertex layout");
    vertices=xsdk_gpu_alloc(ARENA_VERTICES*VERTEX_WORDS*4);
    vertex_cursor = 0;
    if (!vertices) fail("vertex allocation");
    checked(xsdk_texture_create(&white,XSDK_FORMAT_R8G8B8A8,1,1,1,true),"fallback texture");
    const uint32_t pixel=0xFFFFFFFF;
    xsdk_texture_upload(&white,0,&pixel,4); xsdk_gpu_invalidate(white.address,white.size_bytes);
    XsdkSampler sampler_state={0}; xsdk_texture_sampler_state(&white_sampler,&sampler_state);
    selected[0]=selected[1]=UINT32_MAX;
    gfx_xsdk.get_dimensions(&width,&height);
    set_viewport(0,0,(int)width,(int)height); set_scissor(0,0,(int)width,(int)height);
    checked(xsdk_texture_create(&scene_texture,XSDK_FORMAT_R8G8B8A8,width,height,1,true),"scene target allocation");
    XsdkSampler scene_filter={0};
    scene_filter.mag_filter=scene_filter.min_filter=XSDK_FILTER_LINEAR;
    scene_filter.address_u=scene_filter.address_v=XSDK_ADDRESS_CLAMP;
    xsdk_texture_sampler_state(&scene_sampler,&scene_filter);
    checked(xsdk_shader_load(&upscale_vertex,upscale_vert.container,upscale_vert.container_bytes),"upscale vertex shader");
    checked(xsdk_shader_load(&upscale_pixel,upscale_frag.container,upscale_frag.container_bytes),"upscale pixel shader");
    XsdkVertexElement quad_layout={0};quad_layout.usage=XSDK_USAGE_POSITION;quad_layout.format=XSDK_VERTEX_FLOAT4;
    const uint32_t quad_stride=4*sizeof(float);
    checked(xsdk_shader_set_vertex_layout(&upscale_vertex,&quad_layout,1,&quad_stride),"upscale vertex layout");
    const float quad[]={-1,-1,0,1, -1,1,0,0, 1,1,1,0, -1,-1,0,1, 1,1,1,0, 1,-1,1,1};
    upscale_vertices=xsdk_gpu_alloc(sizeof quad);
    if(!upscale_vertices)fail("upscale vertex allocation");
    uint32_t quad_words[24];memcpy(quad_words,quad,sizeof quad);
    for(unsigned i=0;i<24;++i)upscale_vertices[i]=quad_words[i];
    xsdk_gpu_invalidate(xsdk_gpu_address(upscale_vertices),sizeof quad);

}
static void shutdown(void) {
    wait_gpu();
    for (uint32_t i=0; i<texture_count; i++) if (textures[i].ready) xsdk_texture_free(&textures[i].image);
    free(textures); textures=NULL; texture_count=0;
    xsdk_texture_free(&white); xsdk_gpu_free(vertices); vertices=NULL;
    xsdk_texture_free(&scene_texture);xsdk_gpu_free(upscale_vertices);upscale_vertices=NULL;
    xsdk_shader_free(&upscale_vertex);xsdk_shader_free(&upscale_pixel);
    xsdk_shader_free(&vertex_shader); xsdk_shader_free(&pixel_shader);
    for(unsigned i=0;i<MATERIAL_COUNT;++i) if(material_ready[i]) {
        xsdk_shader_free(&material_shaders[i]);material_ready[i]=false;
    }
    program_count=program_next=0; current=NULL;
}
static void nothing(void) {}
static const char *name(void) { return "Xenos/xSDK"; }
struct GfxRenderingAPI gfx_xenos_api = {
    z_range,unload,load,create,lookup,info,new_texture,select_texture,upload,sampler,
    set_depth,set_depth_write,set_decal,set_viewport,set_scissor,set_alpha,draw,
    init,nothing,begin,nothing,nothing,name,shutdown
};
#endif




