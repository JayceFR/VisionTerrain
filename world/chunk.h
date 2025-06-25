#ifndef CHUNK_H
#define CHUNK_H

#include "../utils/math.h"

#define CHUNK_SIZE_X 16
#define CHUNK_SIZE_Y 16
#define CHUNK_SIZE_Z 16
#define VERTEX_COUNT 48
#define FACE_COUNT   6

#define INITIAL_CAPACITY 1024

struct chunk;

typedef struct chunk *chunk;

typedef enum {
  BLOCK_AIR,
  BLOCK_DIRT,
  BLOCK_GRASS,
  BLOCK_WATER,
  BLOCK_OAK,
  BLOCK_LEAF,
  BLOCK_NULL
} BLOCK_TYPE;

typedef enum {
  BACK,
  FRONT,
  LEFT,
  RIGHT,
  BOTTOM,
  TOP
} FACE; 

typedef struct{
  BLOCK_TYPE type;
  FACE face; 
  int value;
} textureMap; 

// functions provided
extern chunk createChunk(float x, float y, float z);
extern void freeChunk(chunk c);
extern void renderChunk(
  chunk c, 
  GLuint program, 
  GLuint waterShader,
  mat4x4 view, 
  mat4x4 proj,
  vec3d lightPos,
  vec3d viewPos,
  float time, GLuint texture, 
  bool fake, GLuint reflectedTex, GLuint dudvTex);

#endif