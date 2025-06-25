#version 330 core

in float flicker;
in float distToCamera;
out vec4 FragColor;

void main() {
    vec2 p = gl_PointCoord - 0.5;
    float dist = length(p);

    float alpha = clamp(1.0 - dist / 0.25, 0.0, 1.0) * flicker;

    float fadeStart = 10.0;
    float fadeEnd = 25.0;
    float fade = 1.0 - smoothstep(fadeStart, fadeEnd, distToCamera);

    vec3 color = vec3(0.6, 0.8, 1.0) * fade;
    FragColor = vec4(color * flicker, 1.0 * fade);
}
