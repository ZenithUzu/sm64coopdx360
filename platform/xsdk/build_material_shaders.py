"""Build specialized material shaders from coopdx's precompiled combiner list.
Unknown combinations continue to use the generic shader. No reference binary data is used.
"""
import argparse,json,pathlib,re,subprocess,sys

def shader_commands(words,two_cycle):
    inputs={};out=[]
    fixed={0:0,1:9,2:11,6:10,8:13,9:12,10:14 if two_cycle else 0,11:15 if two_cycle else 0,15:16}
    for word in words:
        for shift in range(0,32,8):
            value=(word>>shift)&255
            if value in fixed: out.append(fixed[value])
            elif value in (3,4,5,7,12,13,14):
                if value not in inputs:inputs[value]=len(inputs)+1
                out.append(inputs[value])
            else:out.append(0)
    return out

def main():
    p=argparse.ArgumentParser();p.add_argument('--repo',type=pathlib.Path,required=True);p.add_argument('--out',type=pathlib.Path,required=True)
    p.add_argument('--xsdk',type=pathlib.Path,required=True);p.add_argument('--glslang',required=True);a=p.parse_args()
    out=a.out;out.mkdir(parents=True,exist_ok=True)
    text=(a.repo/'src/pc/gfx/gfx_cc.c').read_text()
    fixtures=[[int(v,16) for v in row] for row in re.findall(r'gfx_pc_precomp_shader\((0x[0-9a-f]+), (0x[0-9a-f]+), (0x[0-9a-f]+), (0x[0-9a-f]+), (0x[0-9a-f]+)\)',text)]
    # Basic texture, texture/shade and flat-color combinations cover menu and simple materials too.
    fixtures += [[0x01000000,0x06000000,0x02000000,0x09000000,1],[0x00040001,0x000d0006,0,0,1],[0x04000000,0x0d000000,0,0,1]]
    patterns=[]
    for row in fixtures:
        commands=shader_commands(row[:4],bool(row[4]&16))
        if commands not in patterns:patterns.append(commands)
    template=(a.repo/'platform/xsdk/shaders/coopdx.frag').read_text()
    values=['vec4(0)']+[f'i{i}' for i in range(8)]+['t0','vec4(t0.a)','t1','vec4(t1.a)','vec4(1)','combined','vec4(combined.a)','vec4(0.5)']
    sources=[]
    for index,commands in enumerate(patterns):
        s=template
        for n,name in enumerate(['c0','a0','c1','a1']):
            av,bv,cv,dv=[values[c] for c in commands[4*n:4*n+4]]
            expr=f'(({av}-{bv})*{cv}+{dv})'
            s=s.replace(f'formula(p.{name},t0,t1,combined)',expr)
        source=out/f'material_{index}.frag';source.write_text(s);sources.append(source)
    header=out/'material_code.h'
    subprocess.run([sys.executable,str(a.xsdk/'tools/build_shaders.py'),'--glslang',a.glslang,'--output',str(header),*[str(s) for s in sources]],check=True)
    generated=header.read_text()
    lines=['#ifndef COOPDX_MATERIAL_SHADERS_H','#define COOPDX_MATERIAL_SHADERS_H','#include "material_code.h"',
      'struct MaterialDefinition { uint8_t commands[16]; const XsdkBuiltShader *code; unsigned push, registers; int sampler[2]; };',
      'static const struct MaterialDefinition material_definitions[] = {']
    for i,commands in enumerate(patterns):
        name=f'MATERIAL_{i}_FRAG'
        samplers=[f'{name}_SAMPLER_0_{t}' if f'{name}_SAMPLER_0_{t} =' in generated else '-1' for t in range(2)]
        lines.append('    {{'+','.join(map(str,commands))+'}, &material_'+str(i)+'_frag, '+name+'_PUSH, '+name+'_PUSH_REGISTERS, {'+','.join(samplers)+'}},')
    lines+=['};','#define MATERIAL_COUNT (sizeof material_definitions / sizeof material_definitions[0])','#endif']
    (out/'material_shaders.h').write_text('\n'.join(lines)+'\n')
    (out/'manifest.json').write_text(json.dumps({'fixtures':fixtures,'commands':patterns},indent=2))
    print('Specialized material patterns:',len(patterns))
if __name__=='__main__':main()
