#version 330 core

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 anormal;
layout (location = 2) in vec2 texCoord;

uniform mat4 projection;
uniform mat4 view; 
uniform mat4 model;

out vec2 uvs;
out vec3 normal;
out vec3 FragPos;
out float visibility; // fog calculation

const float density = 0.07;
const float gradient = 1.5;


void main() {
  gl_Position = projection * view * model * vec4(pos, 1.0);
  uvs = texCoord;
  normal = mat3(transpose(inverse(model))) * anormal;
  FragPos = vec3(model * vec4(pos, 1.0));
  // fog stuff
  vec4 worldPos = model * vec4(pos, 1.0);
  vec4 viewSpacePos = view * worldPos;

  float distance = abs(viewSpacePos.z); // camera distance
  visibility = exp(-pow((distance * density), gradient));
  visibility = clamp(visibility, 0.0, 1.0);

}