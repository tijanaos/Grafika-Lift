#version 330 core

in vec4 channelCol;
in vec2 channelTex;

out vec4 outCol;

uniform sampler2D uTex;
uniform bool useTex;
uniform bool transparent;

// NOVO:
uniform vec4 uColor;

void main()
{
    if (!useTex) {
        outCol = uColor;              // koristimo uniform boju, ignorišemo teksture
    }
    else {
        outCol = texture(uTex, channelTex);
        if (!transparent && outCol.a < 1) {
            outCol = vec4(1.0, 1.0, 1.0, 1.0);
        }
    }
}
