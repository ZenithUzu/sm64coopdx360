#version 450
layout(location=0) in vec4 uv;
layout(location=1) in vec4 fog;
layout(location=2) in vec4 lightUV;
layout(location=3) in vec4 i0;
layout(location=4) in vec4 i1;
layout(location=5) in vec4 i2;
layout(location=6) in vec4 i3;
layout(location=7) in vec4 i4;
layout(location=8) in vec4 i5;
layout(location=9) in vec4 i6;
layout(location=10) in vec4 i7;
layout(location=0) out vec4 outColor;
layout(set=0,binding=0) uniform sampler2D tex0;
layout(set=0,binding=1) uniform sampler2D tex1;
layout(push_constant) uniform Params { vec4 c0; vec4 a0; vec4 c1; vec4 a1; vec4 flags; vec4 lighting; vec4 noiseParams; } p;
float noise=0.0;
vec4 selectValue(float code, vec4 t0, vec4 t1, vec4 combined) {
 vec4 result=vec4(0);
 if(code<0.5) result=vec4(0);
 else if(code<1.5) result=i0;
 else if(code<2.5) result=i1;
 else if(code<3.5) result=i2;
 else if(code<4.5) result=i3;
 else if(code<5.5) result=i4;
 else if(code<6.5) result=i5;
 else if(code<7.5) result=i6;
 else if(code<8.5) result=i7;
 else if(code<9.5) result=t0;
 else if(code<10.5) result=vec4(t0.a);
 else if(code<11.5) result=t1;
 else if(code<12.5) result=vec4(t1.a);
 else if(code<13.5) result=vec4(1);
 else if(code<14.5) result=combined;
 else if(code<15.5) result=vec4(combined.a);
 else result=vec4(noise);
 return result;
}
vec4 formula(vec4 cmd,vec4 t0,vec4 t1,vec4 combined) {
 return (selectValue(cmd.x,t0,t1,combined)-selectValue(cmd.y,t0,t1,combined))*selectValue(cmd.z,t0,t1,combined)+selectValue(cmd.w,t0,t1,combined);
}
void main() {
 if(p.noiseParams.y>0.5) {
   vec3 cell=floor(vec3(gl_FragCoord.xy,p.noiseParams.x));
   noise=floor(fract(sin(dot(sin(cell),vec3(12.9898,78.233,37.719)))*143758.5453)+0.5);
 }
 vec4 t0=texture(tex0,uv.xy);
 vec4 t1=texture(tex1,p.lighting.w>0.5 ? lightUV.xy : uv.zw);
 if(p.lighting.w>0.5) { t0.rgb*=p.lighting.rgb; t1.rgb=t1.rgb*t1.rgb+t1.rgb; }
 vec4 combined=vec4(0);
 vec3 rgb=formula(p.c0,t0,t1,combined).rgb;
 float alpha=formula(p.a0,t0,t1,combined).a;
 combined=vec4(rgb,alpha);
 if(p.flags.x>0.5) {
   rgb=formula(p.c1,t0,t1,combined).rgb;
   alpha=formula(p.a1,t0,t1,combined).a;
 }
 if(p.flags.y<0.5) alpha=1.0;
 if(p.flags.z>0.5) { if(alpha<=0.3) discard; alpha=1.0; }
 if(p.flags.w>0.5) rgb=mix(rgb,fog.rgb,fog.a);
 if(p.noiseParams.z>0.5) alpha*=noise;
 outColor=vec4(rgb,alpha);
}

