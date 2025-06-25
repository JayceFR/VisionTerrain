#version 330 core

in vec2 uvs;
in vec2 worldUV;
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
    float amplitude = 0.4; // reduced from 0.5
    for (int i = 0; i < 4; i++) { // reduced octaves
        value += amplitude * snoise(uv);
        uv *= 2.0;
        amplitude *= 0.4; // fade faster
    }
    return value;
}

float waveStrength = 0.02;
const vec2 SCROLL_SPEED = vec2(0.07, 0.03);

void main() {
    vec2 ndc = (clipSpace.xy/clipSpace.w)/2.0 + 0.5;
    vec2 reflectUvs = vec2(ndc.x, 1.0 -ndc.y);
 
    float moveFactor = fract(0.04 * time);
    vec2 scroll = fract(uvs + time * SCROLL_SPEED);

    vec2 distortion1 = (texture(dudvMap, vec2(uvs.x + moveFactor, uvs.y)).rg * 2.0 - 1.0) * waveStrength;
    vec2 distortion2 = (texture(dudvMap, vec2(-uvs.x + moveFactor, uvs.y + moveFactor)).rg * 2.0 - 1.0) * waveStrength;
    reflectUvs += distortion1 + distortion2;

    float fresnel = pow(1.0 - dot(normalize(normal), normalize(viewPos - FragPos)), 3.0);
    if (reflectUvs.x < 0.0 || reflectUvs.x > 1.0 || reflectUvs.y < 0.0 || reflectUvs.y > 1.0) {
        discard;
    }
    vec4 reflColor = texture(reflectedTexture, reflectUvs);
    vec4 baseColor = vec4(0.0, 0.3, 0.5, 1.0);
    FragColor = mix(reflColor, baseColor, 0.2) ;




    // // Smoothed FBM (less contrast)
    // float n1 = fbm(worldUV * 2.5 + vec2(time * 0.3, time * 0.2));
    // float n2 = fbm(worldUV * 6.0 - vec2(time * 0.5, time * 0.3));
    // float combinedNoise = 0.5 * n1 + 0.5 * n2;
    // float noise01 = (combinedNoise + 1.0) * 0.5;

    // // Ocean swell
    // float swell = 0.2 * sin(worldUV.x * 1.5 + time * 0.8)
    //             + 0.15 * sin(worldUV.y * 1.8 - time * 1.0)
    //             + 0.1 * sin((worldUV.x + worldUV.y) * 1.2 + time * 0.5);
    // float verticalWave = swell;

    // // Animate slight flow motion
    // vec2 waveOffset = vec2(
    //     sin(worldUV.y * 0.3 + time * 0.5),
    //     cos(worldUV.x * 0.3 - time * 0.5)
    // ) * 0.1 * combinedNoise;

    // vec2 warpedUV = worldUV + waveOffset;

    // // Wave tile variation
    // float waveX = 0.5 + 0.5 * sin(warpedUV.x * 0.15 + combinedNoise + verticalWave);
    // float waveY = 0.5 + 0.5 * sin(warpedUV.y * 0.1 + combinedNoise + verticalWave);

    // // Brightness
    // float brightness = clamp(smoothstep(0.3, 0.7, noise01 + verticalWave * 0.3), 0.4, 0.75);

    // // Colors
    // vec3 deepBlue = vec3(0.04, 0.08, 0.15);
    // vec3 midBlue = vec3(0.08, 0.14, 0.25);
    // vec3 crestBlue = vec3(0.25, 0.4, 0.55);
    // vec3 waveColor = mix(midBlue, crestBlue, pow(waveY, 2.0));
    // vec3 baseColor = mix(deepBlue, waveColor, waveX);

    // vec3 color = baseColor * brightness;

    // // Gentle crest shimmer
    // float crest = smoothstep(0.75, 1.0, noise01 + verticalWave * 0.4);
    // vec3 shimmer = vec3(1.0, 0.8, 0.9) * crest * 0.08;
    // color += shimmer;

    // // Final tint
    // color = mix(color, vec3(0.7, 0.5, 0.6), 0.03);
    // color += verticalWave * 0.03 * vec3(0.03, 0.06, 0.1);
    // color = clamp(color, 0.0, 1.0);

    // // Lighting
    // vec3 norm = normalize(normal);
    // vec3 lightDir = normalize(lightPos - FragPos);
    // lightDir.x += sin(time) * 0.2;
    // lightDir.y += cos(time) * 0.2;

    // float diff = max(dot(norm, lightDir), 0.0);
    // vec3 diffuse = diff * lightColor;

    // float ambientStrength = 0.3;
    // vec3 ambient = ambientStrength * lightColor;

    // vec3 viewDir = normalize(viewPos - FragPos);
    // vec3 halfwayDir = normalize(lightDir + viewDir);
    // float spec = pow(max(dot(norm, halfwayDir), 0.0), 32.0);
    // vec3 specular = vec3(0.3) * spec;

    // vec3 finalColor = (ambient + diffuse + specular) * color;

    // FragColor = vec4(finalColor, 1.0);
}
