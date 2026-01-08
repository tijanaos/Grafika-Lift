#version 330 core

in vec2 TexCoord;
out vec4 FragColor;

uniform vec4 uColor;
uniform sampler2D uTexture;
uniform bool uUseTexture;

void main()
{
    vec4 color = uColor;

    if (uUseTexture)
    {
        color = texture(uTexture, TexCoord);
    }

    FragColor = color;
}
