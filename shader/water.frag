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


// --- FBM / Noise Helpers ---
vec3 mod289(vec3 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }
vec2 mod289(vec2 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }
vec3 permute(vec3 x) { return mod289(((x * 34.0) + 1.0) * x); }

float snoise(vec2 v) {
    const vec4 C = vec4(0.211324865405187, 0.366025403784439,
                        -0.577350269189626, 0.024390243902439);
    vec2 i = floor(v + dot(v, C.yy));
    vec2 x0 = v - i + dot(i, C.xx);
    vec2 i1 = (x0.x > x0.y) ? vec2(1.0, 0.0) : vec2(0.0, 1.0);
    vec4 x12 = x0.xyxy + C.xxzz; x12.xy -= i1;
    i = mod289(i);
    vec3 p = permute(permute(i.y + vec3(0.0, i1.y, 1.0)) + i.x + vec3(0.0, i1.x, 1.0));
    vec3 m = max(0.5 - vec3(dot(x0,x0), dot(x12.xy,x12.xy), dot(x12.zw,x12.zw)), 0.0);
    m *= m * m * m;
    vec3 x = 2.0 * fract(p * C.www) - 1.0;
    vec3 h = abs(x) - 0.5;
    vec3 ox = floor(x + 0.5);
    vec3 a0 = x - ox;
    m *= 1.79284291400159 - 0.85373472095314 * (a0*a0 + h*h);
    vec3 g;
    g.x = a0.x * x0.x + h.x * x0.y;
    g.y = a0.y * x12.x + h.y * x12.y;
    g.z = a0.z * x12.z + h.z * x12.w;
    return 130.0 * dot(m, g);
}

float fbm(vec2 uv) {
    float value = 0.0;
    float amplitude = 0.5;
    float frequency = 1.0;
    for (int i = 0; i < 5; i++) {
        value += amplitude * snoise(uv * frequency);
        frequency *= 2.0;
        amplitude *= 0.5;
    }
    return value;
}

void main() {
    vec2 ndc = (clipSpace.xy / clipSpace.w) / 2.0 + 0.5;
    vec2 reflectUvs = vec2(ndc.x, 1.0 - ndc.y);

    const vec2 SCROLL_SPEED_1 = vec2(0.07, 0.03);
    const vec2 SCROLL_SPEED_2 = vec2(-0.04, 0.06);
    float waveStrength = 0.05;

    vec2 dudvUV1 = fract(worldUV + SCROLL_SPEED_1 * time);
    vec2 dudvUV2 = fract(worldUV + SCROLL_SPEED_2 * time * 1.3);
    vec2 dudv1 = texture(dudvMap, dudvUV1).rg * 2.0 - 1.0;
    vec2 dudv2 = texture(dudvMap, dudvUV2).rg * 2.0 - 1.0;
    vec2 baseDistortion = (dudv1 + dudv2) * 0.5;

    reflectUvs += baseDistortion * waveStrength;

    float edgeFade = smoothstep(0.05, 0.1, reflectUvs.x) * smoothstep(0.05, 0.1, reflectUvs.y) *
                     smoothstep(0.95, 0.9, reflectUvs.x) * smoothstep(0.95, 0.9, reflectUvs.y);
    reflectUvs = clamp(reflectUvs, 0.0, 1.0);

    float n1 = fbm(worldUV * 3.5 + vec2(time * 0.6, time * 0.4));
    float n2 = fbm(worldUV * 8.0 - vec2(time * 1.0, time * 0.7));
    float combinedNoise = 0.6 * n1 + 0.4 * n2;

    combinedNoise = smoothstep(0.3, 0.7, combinedNoise);

    float rippleX = sin(worldUV.x * 20.0 + time * 8.0);
    float rippleY = sin(worldUV.y * 18.0 - time * 7.0);
    float ripple = (rippleX + rippleY) * 0.05;

    float waveHeight = combinedNoise + ripple;

    // Slightly darker base and crest colors, more muted, realistic blues
    vec3 deepBlue = vec3(0.015, 0.03, 0.12);
    vec3 midBlue = vec3(0.04, 0.08, 0.20);
    vec3 crestBlue = vec3(0.15, 0.25, 0.45);

    float waveMix = smoothstep(0.4, 0.7, waveHeight);
    vec3 waveColor = mix(midBlue, crestBlue, waveMix);
    vec3 baseColor = mix(deepBlue, waveColor, waveMix);

    float brightness = clamp(0.3 + 0.7 * waveHeight, 0.3, 1.0);
    vec3 color = baseColor * brightness;

    // Crest shimmer (keep subtle)
    float crest = smoothstep(0.75, 1.0, waveHeight);
    vec3 shimmer = vec3(1.0, 0.9, 0.85) * crest * 0.04;
    color += shimmer;

    // Slight desaturation to reduce oversaturation and make reflection pop
    float gray = dot(color, vec3(0.3, 0.59, 0.11));
    color = mix(color, vec3(gray), 0.15);

    color = clamp(color, 0.0, 1.0);

    vec3 norm = normalize(normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    lightDir.x += sin(time) * 0.2;
    lightDir.y += cos(time) * 0.2;

    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    float ambientStrength = 0.3;
    vec3 ambient = ambientStrength * lightColor;

    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfwayDir), 0.0), 32.0);
    vec3 specular = vec3(0.3) * spec;

    vec3 lighting = ambient + diffuse + specular;

    float fresnel = pow(1.0 - dot(norm, viewDir), 4.0);

    vec4 reflColor = texture(reflectedTexture, reflectUvs);

    // Increase reflection influence to make reflection stand out more
    vec3 finalColor = mix(color, reflColor.rgb, fresnel * edgeFade * 1.6);

    finalColor *= lighting;

    FragColor = vec4(finalColor, 1.0);
}