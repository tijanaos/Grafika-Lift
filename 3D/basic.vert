#version 330 core

layout(location=0) in vec3 inPos;
layout(location=1) in vec4 inCol;
layout(location=2) in vec2 inTex;

uniform mat4 uM;
uniform mat4 uV;
uniform mat4 uP;
uniform vec2 uTexScale;

out vec2 vUV;
out vec3 vWorldPos;
out vec4 vCol;

void main() {
    vec4 world = uM * vec4(inPos, 1.0);
    vWorldPos = world.xyz;
    vUV = inTex * uTexScale;
    vCol = inCol;
    gl_Position = uP * uV * world;
}
