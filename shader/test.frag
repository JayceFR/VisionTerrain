#version 330 core

out vec4 FragColor;
in vec2 uvs;
in vec3 normal;
in vec3 FragPos;
in vec3 viewPos;
in float visibility; // fog

uniform vec3 lightColor;
uniform vec3 lightPos;
uniform sampler2D baseTexture;

uniform float time;

void main() {
  vec4 colour = texture(baseTexture, uvs);
  float ambientStrength = 0.5;
  vec3 ambient = ambientStrength * lightColor;

  vec3 norm = normalize(normal);
  vec3 lightDir = normalize(lightPos - FragPos);
  lightDir.x = lightDir.x + sin(time) * 0.2;
  lightDir.y = lightDir.y + cos(time) * 0.2;
  float diff = max(dot(norm, lightDir), 0.0);
  vec3 diffuse = diff * lightColor;

  float specularStrength = 0.5;
  vec3 viewDir = normalize(viewPos - FragPos);
  vec3 reflectDir = reflect(-lightDir, norm);

  vec3 halfwayDir = normalize(lightDir + viewDir);
  float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
  //float spec = pow(max(dot(viewDir, reflectDir), 0.0), 8.0);
  vec3 specular = vec3(0.3) * spec ;

  vec4 result = vec4((ambient + diffuse + specular), 1.0) * colour;
  vec3 skyColor = vec3(0.0, 0.0, 0.0);

  result = mix(vec4(skyColor, 1.0), result, visibility);

  FragColor = result;
}
