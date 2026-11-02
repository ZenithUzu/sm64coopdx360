#version 450
layout(location=0) in vec4 aPos;
layout(location=1) in vec4 aUV;
layout(location=2) in vec4 aFog;
layout(location=3) in vec4 aLight;
layout(location=4) in vec4 a0;
layout(location=5) in vec4 a1;
layout(location=6) in vec4 a2;
layout(location=7) in vec4 a3;
layout(location=8) in vec4 a4;
layout(location=9) in vec4 a5;
layout(location=10) in vec4 a6;
layout(location=11) in vec4 a7;
layout(location=0) out vec4 uv;
layout(location=1) out vec4 fog;
layout(location=2) out vec4 lightUV;
layout(location=3) out vec4 i0;
layout(location=4) out vec4 i1;
layout(location=5) out vec4 i2;
layout(location=6) out vec4 i3;
layout(location=7) out vec4 i4;
layout(location=8) out vec4 i5;
layout(location=9) out vec4 i6;
layout(location=10) out vec4 i7;
void main() {
 gl_Position=aPos;
 uv=aUV; fog=aFog; lightUV=aLight;
 i0=a0; i1=a1; i2=a2; i3=a3; i4=a4; i5=a5; i6=a6; i7=a7;
}
