#version 330 core
layout(location = 0) in vec3 aPos;

void main() {
    // aPos.xy is expected to be in clip space [-1, 1]
    gl_Position = vec4(aPos.xy, 0.0, 1.0);
    gl_PointSize = 5.0;  // Size of each landmark point in pixels
}