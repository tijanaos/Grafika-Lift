#version 330 core
layout(location=0) in vec3 inPos;
layout(location=1) in vec3 inNor;
layout(location=2) in vec2 inTex;

uniform mat4 uM;
uniform mat4 uV;
uniform mat4 uP;

out vec3 vWorldPos;
out vec3 vNormal;
out vec2 vUV;

void main() {
    vec4 world = uM * vec4(inPos, 1.0);
    vWorldPos = world.xyz;
    vNormal = mat3(transpose(inverse(uM))) * inNor;
    vUV = inTex;
    gl_Position = uP * uV * world;
}
