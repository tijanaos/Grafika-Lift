#version 330 core
#define MAX_LIGHTS 32

uniform sampler2D uTex;
uniform int useTex;
uniform vec4 uColor;
uniform int transparent;

uniform vec3 uViewPos;

uniform int   uLightCount;
uniform vec3  uLightPos[MAX_LIGHTS];
uniform vec3  uLightColor[MAX_LIGHTS];
uniform float uLightIntensity[MAX_LIGHTS];

uniform vec3 uEmissive;

in vec2 vUV;
in vec3 vWorldPos;
in vec4 vCol;

out vec4 outCol;

void main() {
    vec4 base = (useTex == 1) ? (texture(uTex, vUV) * uColor) : uColor;

    float alpha = (transparent == 0) ? 1.0 : base.a;

    vec3 albedo = base.rgb;

    vec3 N = normalize(cross(dFdx(vWorldPos), dFdy(vWorldPos)));
    if (!gl_FrontFacing) N = -N;

    vec3 V = normalize(uViewPos - vWorldPos);

    vec3 result = albedo * 0.08;
    float shininess = 32.0;

    int n = min(uLightCount, MAX_LIGHTS);
    for (int i = 0; i < n; i++) {
        vec3 Lvec = uLightPos[i] - vWorldPos;
        float dist = length(Lvec);
        vec3 L = Lvec / max(dist, 0.0001);

        float atten = 1.0 / (1.0 + 0.14 * dist + 0.07 * dist * dist);
        vec3 lc = uLightColor[i] * uLightIntensity[i] * atten;

        float diff = max(dot(N, L), 0.0);
        vec3 ambient = 0.10 * lc * albedo;
        vec3 diffuse = diff * lc * albedo;

        vec3 R = reflect(-L, N);
        float spec = pow(max(dot(V, R), 0.0), shininess);
        vec3 specular = spec * lc * 0.20;

        result += ambient + diffuse + specular;
    }

    result += uEmissive;

    outCol = vec4(result, alpha);
}
