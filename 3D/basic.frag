#version 330 core

in vec4 channelCol;
in vec2 channelTex;

out vec4 outCol;

uniform sampler2D uTex;
uniform int useTex;        // 0 = samo uColor, 1 = tekstura
uniform vec4 uColor;       // tint (radi i za teksturu)
uniform int transparent;   // ako je 0 i tekstura ima alpha < 1, ignorisi alpha

void main() {
    vec4 col = uColor;

    if (useTex == 1) {
        vec4 texCol = texture(uTex, channelTex);
        if (transparent == 0 && texCol.a < 0.99) texCol.a = 1.0;
        col = texCol * uColor;
    }

    outCol = col;
}
