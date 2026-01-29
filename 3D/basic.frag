#version 330 core

#define MAX_LIGHTS 32

uniform sampler2D uTex;
uniform int useTex;
uniform vec4 uColor;
uniform int transparent;

uniform vec3 uViewPos;

// point lights
uniform int   uLightCount;
uniform vec3  uLightPos[MAX_LIGHTS];
uniform vec3  uLightColor[MAX_LIGHTS];
uniform float uLightIntensity[MAX_LIGHTS];

// “glow” (npr. dugme kad je aktivno)
uniform vec3 uEmissive;

in vec2 vUV;
in vec3 vWorldPos;
in vec4 vCol;

out vec4 outCol;

void main() {
    vec4 base = (useTex == 1) ? (texture(uTex, vUV) * uColor) : uColor;

    // ako ima providnosti (ikonice), ostavi alpha
    float alpha = base.a;
    if (transparent == 0) alpha = 1.0;

    vec3 albedo = base.rgb;

    // normal bez dodatnog atributa: iz derivacija (radi super za tvoje box-ove)
    vec3 N = normalize(cross(dFdx(vWorldPos), dFdy(vWorldPos)));
    if (!gl_FrontFacing) N = -N;

    vec3 V = normalize(uViewPos - vWorldPos);

    // mali global ambient da scena nikad ne bude “mrtva”
    vec3 result = albedo * 0.08;

    float shininess = 32.0;

    int n = min(uLightCount, MAX_LIGHTS);
    for (int i = 0; i < n; i++) {
        vec3 Lvec = uLightPos[i] - vWorldPos;
        float dist = length(Lvec);
        vec3 L = Lvec / max(dist, 0.0001);

        // attenuation (tuned za tvoje dimenzije ~ par do ~10 jedinica)
        float atten = 1.0 / (1.0 + 0.14 * dist + 0.07 * dist * dist);

        vec3 lc = uLightColor[i] * uLightIntensity[i] * atten;

        // ambient+diffuse
        float diff = max(dot(N, L), 0.0);
        vec3 ambient = 0.10 * lc * albedo;
        vec3 diffuse = diff * lc * albedo;

        // specular (umereno da ne “izbeli” sobu)
        vec3 R = reflect(-L, N);
        float spec = pow(max(dot(V, R), 0.0), shininess);
        vec3 specular = spec * lc * 0.20;

        result += ambient + diffuse + specular;
    }

    result += uEmissive;

    outCol = vec4(result, alpha);
}
