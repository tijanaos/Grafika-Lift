#version 330 core

#define MAX_LIGHTS 32

uniform sampler2D uDiffMap1;

uniform vec3 uViewPos;

uniform int   uLightCount;
uniform vec3  uLightPos[MAX_LIGHTS];
uniform vec3  uLightColor[MAX_LIGHTS];
uniform float uLightIntensity[MAX_LIGHTS];

in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vUV;

out vec4 outCol;

void main() {
    vec3 albedo = texture(uDiffMap1, vUV).rgb;

    vec3 N = normalize(vNormal);
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

    outCol = vec4(result, 1.0);
}
