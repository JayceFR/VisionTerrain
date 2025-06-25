#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <stdbool.h>

#include "glad/glad.h"
#include <GLFW/glfw3.h>

#include "world.h"
#include "../adts/hash.h"
#include "../utils/math.h"
#include "chunk.h"
#include "../utils/stringManipulate.h"

struct world{
  hash chunks;
  int  width;
  int  height; 
};

typedef struct world *world;

// wrap around the freeChunk in chunk.c
static void freeChunks(void *el){
  chunk c = (chunk) el;
  freeChunk(c);
}

world createWorld(int width, int height){
  world new = malloc(sizeof(struct world));
  assert(new != NULL);
  new->chunks = hashCreate(NULL, &freeChunks, NULL);
  assert(new->chunks != NULL);
  new->width  = width;
  new->height = height;
  for (int x = 0; x < width; x++){
    for (int z = 0; z < width; z++){
      char buffer[12];
      sprintf(buffer, "(%d, %d)", x, z);
      hashSet(new->chunks, clone(buffer), createChunk((float)x * 16, 0.0f, (float)z * 16));
    }
  }
  return new;
}

void freeWorld(world w){
  hashFree(w->chunks);
  free(w);
}

void renderWorld(
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
) {
  int renderDistance = 5; 

  // Get the chunk position of the camera
  int camChunkX = (int)(camPos->x / 16.0f);
  int camChunkZ = (int)(camPos->z / 16.0f);

  for (int dx = -renderDistance; dx <= renderDistance; dx++) {
    for (int dz = -renderDistance; dz <= renderDistance; dz++) {
      int chunkX = camChunkX + dx;
      int chunkZ = camChunkZ + dz;

      if (chunkX < 0 || chunkX >= w->width || chunkZ < 0 || chunkZ >= w->height){
        continue;
      }

      char buffer[20];
      sprintf(buffer, "(%d, %d)", chunkX, chunkZ);

      chunk c = hashFind(w->chunks, buffer);
      if (c != NULL) {
        renderChunk(c, program, waterShader, view, proj, lightPos, viewPos, time, texture, fake, reflectedTex, dudvTex);
      }
    }
  }
}






