#version 330 core
layout(location = 0) in vec2 aPos;
out vec2 uvs;
void main() {
    uvs = (aPos + 1.0) / 2.0; // Convert from [-1,1] to [0,1]
    gl_Position = vec4(aPos, 0.0, 1.0);
}
