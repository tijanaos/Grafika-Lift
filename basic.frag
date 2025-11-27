#version 330 core

in vec2 TexCoord;
out vec4 FragColor;

uniform vec4 uColor;        // Osnovna boja
uniform sampler2D uTexture; // Za kasnije, kad dodamo teksture
uniform bool uUseTexture;   // Da li koristimo teksturu ili ne

void main()
{
    vec4 baseColor = uColor;

    if (uUseTexture) {
        baseColor = texture(uTexture, TexCoord);
    }

    FragColor = baseColor;
}
