#version 330 core

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec4 inCol;
layout(location = 2) in vec2 inTex;

uniform mat4 uM;
uniform mat4 uV;
uniform mat4 uP;

// koliko puta se ponavlja tekstura po U i V
uniform vec2 uTexScale;

out vec4 channelCol;
out vec2 channelTex;

void main() {
    gl_Position = uP * uV * uM * vec4(inPos, 1.0);
    channelCol = inCol;
    channelTex = inTex * uTexScale;
}
