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
out vec2 worldUV;
out vec4 clipSpace;
out vec2 waterUvs; 

void main() {
  vec4 worldPos = model * vec4(pos, 1.0);
  worldUV = worldPos.xz * 0.08;
  clipSpace = projection * view * worldPos;
  gl_Position = clipSpace;
  uvs = texCoord;
  normal = mat3(transpose(inverse(model))) * anormal;
  FragPos = vec3(model * vec4(pos, 1.0));
  waterUvs = vec2(pos.x / 2.0 + 0.5, pos.y / 2.0 + 0.5) * 6.0;
}
