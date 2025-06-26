#version 330 core

in vec2 uvs;
in vec2 worldUV;
in vec2 waterUvs;
out vec4 FragColor;

in vec3 normal;
in vec3 FragPos;
uniform vec3 viewPos;
in vec4 clipSpace;

uniform vec3 lightColor;
uniform vec3 lightPos;
uniform sampler2D baseTexture;

uniform sampler2D reflectedTexture; 
uniform sampler2D dudvMap; 
uniform sampler2D normalMap; 

uniform float time;


float waveStrength = 0.02;
const vec2 SCROLL_SPEED_1 = vec2(0.07, 0.03);
const vec2 SCROLL_SPEED_2 = vec2(-0.04, 0.06);

// ----- 2D Perlin Noise implementation -----

// Hash function for permutation
vec2 hash(vec2 p) {
    p = vec2(dot(p, vec2(127.1, 311.7)),
             dot(p, vec2(269.5, 183.3)));
    return -1.0 + 2.0 * fract(sin(p) * 43758.5453123);
}

// Linear interpolation
float lerp(float a, float b, float t) {
    return a + t * (b - a);
}

// Fade function for smoothing
float fade(float t) {
    return t * t * t * (t * (t * 6.0 - 15.0) + 10.0);
}

// Perlin noise function
float perlinNoise(vec2 p) {
    vec2 pi = floor(p);
    vec2 pf = fract(p);

    // Four gradient vectors at corners
    vec2 g00 = hash(pi + vec2(0.0, 0.0));
    vec2 g10 = hash(pi + vec2(1.0, 0.0));
    vec2 g01 = hash(pi + vec2(0.0, 1.0));
    vec2 g11 = hash(pi + vec2(1.0, 1.0));

    // Compute dot product of gradient and distance vectors
    float d00 = dot(g00, pf - vec2(0.0, 0.0));
    float d10 = dot(g10, pf - vec2(1.0, 0.0));
    float d01 = dot(g01, pf - vec2(0.0, 1.0));
    float d11 = dot(g11, pf - vec2(1.0, 1.0));

    // Interpolate
    float u = fade(pf.x);
    float v = fade(pf.y);
    float nx0 = lerp(d00, d10, u);
    float nx1 = lerp(d01, d11, u);
    float nxy = lerp(nx0, nx1, v);

    return nxy;
}

// Fractal noise: sum of multiple octaves of perlinNoise
float fractalNoise(vec2 p) {
    float total = 0.0;
    float amplitude = 1.0;
    float frequency = 1.0;
    float persistence = 0.5;
    int octaves = 4;

    for(int i = 0; i < octaves; i++) {
        total += perlinNoise(p * frequency) * amplitude;
        amplitude *= persistence;
        frequency *= 2.0;
    }
    return total;
}

void main() {
    vec2 ndc = (clipSpace.xy / clipSpace.w) / 2.0 + 0.5;
    vec2 reflectUvs = vec2(ndc.x, 1.0 - ndc.y);

    // Base dudv distortion (two layers with different speeds/directions)
    vec2 dudvUV1 = fract(worldUV + SCROLL_SPEED_1 * time);
    vec2 dudvUV2 = fract(worldUV + SCROLL_SPEED_2 * time * 1.3);

    vec2 dudv1 = texture(dudvMap, dudvUV1).rg * 2.0 - 1.0;
    vec2 dudv2 = texture(dudvMap, dudvUV2).rg * 2.0 - 1.0;

    vec2 baseDistortion = (dudv1 + dudv2) * 0.5;

    // Procedural fractal noise distortion (animated over time)
    float noiseX = fractalNoise(worldUV * 3.0 + vec2(time * 0.2, 0.0));
    float noiseY = fractalNoise(worldUV * 3.0 + vec2(0.0, time * 0.25));
    vec2 noiseDistortion = vec2(noiseX, noiseY) * 2.0 - 1.0;

    // Combine dudv distortion + noise, scale by waveStrength
    vec2 totalDistortion = (baseDistortion + noiseDistortion * 0.4) * waveStrength;

    reflectUvs += totalDistortion;

    // Fresnel effect
    float fresnel = pow(1.0 - dot(normalize(normal), normalize(viewPos - FragPos)), 3.0);

    // Edge fade to avoid harsh edges near border
    float edgeFade = smoothstep(0.05, 0.1, reflectUvs.x) * smoothstep(0.05, 0.1, reflectUvs.y) *
                     smoothstep(0.95, 0.9, reflectUvs.x) * smoothstep(0.95, 0.9, reflectUvs.y);

    reflectUvs = clamp(reflectUvs, 0.0, 1.0);

    vec4 reflColor = texture(reflectedTexture, reflectUvs);
    vec4 baseColor = vec4(0.0, 0.3, 0.5, 1.0);

    FragColor = mix(reflColor, baseColor, fresnel * edgeFade);
}
