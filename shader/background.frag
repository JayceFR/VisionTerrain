#version 330 core
in vec2 uvs;
out vec4 FragColor;

void main() {
    // Slight pink at horizon, deep blue at top
    vec3 bottomColor = vec3(0.35, 0.1, 0.2); // Soft pinkish purple
    vec3 topColor    = vec3(0.0, 0.0, 0.2);  // Night blue

    vec3 color = mix(bottomColor, topColor, uvs.y);
    FragColor = vec4(color, 1.0);
}