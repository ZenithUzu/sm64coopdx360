#ifndef GFX_XENOS_H
#define GFX_XENOS_H
#include "gfx_rendering_api.h"
extern struct GfxRenderingAPI gfx_xenos_api;
void gfx_xenos_request_capture(void);
void gfx_xenos_present(uint32_t address, uint32_t output_width, uint32_t output_height);
#endif
