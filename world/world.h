#ifndef WORLD_H
#define WORLD_H

#include "../utils/math.h"

struct world;
typedef struct world *world;

extern world createWorld(int width, int height);
extern void freeWorld(world w);
extern void renderWorld(
  world w, 
  vec3d camPos, 
  GLuint program, 
  GLuint waterShader,
  mat4x4 view, 
  mat4x4 proj,
  vec3d lightPos,
  vec3d viewPos,
  float time, GLuint texture, 
  bool fake, GLuint reflectedTex, GLuint dudvTex
);

#endif