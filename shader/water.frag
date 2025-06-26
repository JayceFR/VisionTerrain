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


vec3 mod289(vec3 x){ return x - floor(x*(1.0/289.0))*289.0; }
vec2 mod289(vec2 x){ return x - floor(x*(1.0/289.0))*289.0; }
vec3 permute(vec3 x){ return mod289(((x*34.0)+1.0)*x); }

float snoise(vec2 v){
    const vec4 C = vec4(0.211324865405187, 0.366025403784439,
                        -0.577350269189626, 0.024390243902439);
    vec2  i  = floor(v + dot(v, C.yy));
    vec2  x0 = v - i + dot(i, C.xx);
    vec2  i1 = (x0.x > x0.y) ? vec2(1.0, 0.0) : vec2(0.0, 1.0);
    vec4  x12 = x0.xyxy + C.xxzz; x12.xy -= i1;
           i  = mod289(i);
    vec3  p  = permute(permute(i.y + vec3(0.0, i1.y, 1.0))
                     + i.x + vec3(0.0, i1.x, 1.0));
    vec3  m  = max(0.5 - vec3(dot(x0,x0), dot(x12.xy,x12.xy),
                               dot(x12.zw,x12.zw)), 0.0);
          m  = m * m * m * m;
    vec3  x  = 2.0*fract(p*C.www) - 1.0;
    vec3  h  = abs(x) - 0.5;
    vec3  ox = floor(x + 0.5);
    vec3  a0 = x - ox;
          m *= 1.79284291400159 - 0.85373472095314*(a0*a0 + h*h);
    vec3  g;
          g.x = a0.x*x0.x + h.x*x0.y;
          g.y = a0.y*x12.x + h.y*x12.y;
          g.z = a0.z*x12.z + h.z*x12.w;
    return 130.0*dot(m,g);
}

float fbm(vec2 uv){
    float v=0.0, a=0.5, f=1.0;
    for(int i=0;i<5;i++){
        v += a * snoise(uv * f);
        f *= 2.0;
        a *= 0.5;
    }
    return v;
}

/* ----------  Main  ---------- */
void main(){
    /* 1. Distorted reflection UVs with DuDv -------------------------------- */
    vec2 ndc        = (clipSpace.xy/clipSpace.w)*0.5 + 0.5;
    vec2 reflectUV  = vec2(ndc.x, 1.0 - ndc.y);

    const vec2 SCROLL1 = vec2( 0.07,  0.03);
    const vec2 SCROLL2 = vec2(-0.04,  0.06);
    float waveStrength = 0.05;

    vec2 dUV1 = fract(worldUV + SCROLL1* time);
    vec2 dUV2 = fract(worldUV + SCROLL2* time*1.3);
    vec2 offset = (texture(dudvMap,dUV1).rg*2.0-1.0
                 + texture(dudvMap,dUV2).rg*2.0-1.0)*0.5;

    reflectUV  += offset * waveStrength;

    float edgeFade = smoothstep(0.05,0.1,reflectUV.x)*smoothstep(0.05,0.1,reflectUV.y)*
                     smoothstep(0.95,0.9,reflectUV.x)*smoothstep(0.95,0.9,reflectUV.y);
    reflectUV   = clamp(reflectUV,0.0,1.0);

    /* 2. Wave height from FBM + small ripples ------------------------------ */
    float n1 = fbm(worldUV*3.5 + vec2(time*0.6, time*0.4));
    float n2 = fbm(worldUV*8.0 - vec2(time*1.0, time*0.7));
    float noise  = smoothstep(0.3,0.7, 0.6*n1 + 0.4*n2);

    float ripple = (sin(worldUV.x*20.0 + time*8.0)
                  + sin(worldUV.y*18.0 - time*7.0))*0.05;

    float waveH  = noise + ripple;

    /* 3. Greenish-blue colour ramp ---------------------------------------- */
    vec3 deepColor  = vec3(0.015, 0.05, 0.10);  // deep teal-blue
    vec3 midColor   = vec3(0.035,0.18, 0.20);   // mid aqua
    vec3 crestColor = vec3(0.13, 0.43, 0.45);   // sunlit crest, green-blue

    float mixAmt  = smoothstep(0.4,0.7,waveH);
    vec3 waveCol  = mix(midColor,  crestColor, mixAmt);
    vec3 baseCol  = mix(deepColor, waveCol,   mixAmt);

    float brightness = clamp(0.3 + 0.7*waveH, 0.3, 1.0);
    vec3  color      = baseCol * brightness;

    /* Subtle crest shimmer */
    float crest = smoothstep(0.75,1.0,waveH);
    color += vec3(1.0,0.95,0.90) * crest * 0.035;

    /* Gentle aqua tint (makes shallow areas a bit greener) */
    color = mix(color, vec3(0.25,0.55,0.55), 0.03);
    color = clamp(color,0.0,1.0);

    /* 4. Lighting ---------------------------------------------------------- */
    vec3 norm      = normalize(normal);
    vec3 lightDir  = normalize(lightPos - FragPos);
    lightDir.x    += sin(time)*0.2;
    lightDir.y    += cos(time)*0.2;

    float diff     = max(dot(norm,lightDir),0.0);
    vec3  diffuse  = diff * lightColor;
    vec3  ambient  = 0.3 * lightColor;
    vec3  viewDir  = normalize(viewPos - FragPos);
    vec3  halfDir  = normalize(lightDir + viewDir);
    float spec     = pow(max(dot(norm,halfDir),0.0),32.0);
    vec3  specular = vec3(0.3)*spec;

    vec3 lighting  = ambient + diffuse + specular;

    /* 5. Fresnel reflection ------------------------------------------------ */
    float fresnel  = pow(1.0 - dot(norm,viewDir), 4.0);
    vec3  refl     = texture(reflectedTexture, reflectUV).rgb;

    vec3 finalCol  = mix(color, refl, fresnel*edgeFade*1.6) * lighting;

    FragColor = vec4(finalCol,1.0);
}