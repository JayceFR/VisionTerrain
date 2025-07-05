#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>

#include "camera.h"
#include "../utils/math.h"
#include "chunk.h"
#include "world.h"
#include "../adts/hash.h"

#define PLAYER_WIDTH 1.0f 
#define PLAYER_HEIGHT 2.0f
#define CHUNK_SIZE 16.0f 

static bool collisionCheck(world w, vec3d newPos) {
  float minX = newPos->x - PLAYER_WIDTH / 2.0f;
  float maxX = newPos->x + PLAYER_WIDTH / 2.0f;
  float minY = newPos->y;
  float maxY = newPos->y + PLAYER_HEIGHT;
  float minZ = newPos->z - PLAYER_WIDTH / 2.0f;
  float maxZ = newPos->z + PLAYER_WIDTH / 2.0f;

  if (minY >= 16) {
    return false;
  }

  int minChunkX = floor(minX / CHUNK_SIZE);
  int maxChunkX = ceil(maxX / CHUNK_SIZE);
  int minChunkZ = floor(minZ / CHUNK_SIZE);
  int maxChunkZ = ceil(maxZ / CHUNK_SIZE);

  for (int cx = minChunkX; cx <= maxChunkX; cx++) {
    for (int cz = minChunkZ; cz <= maxChunkZ; cz++) {
      char key[20];
      sprintf(key, "(%d, %d)", cx, cz);
      chunk c = hashFind(getChunks(w), key);
      if (!c) continue;

      int startX = fmax(0, (int)(floor(minX - cx * CHUNK_SIZE)));
      int endX   = fmin(15, (int)(ceil(maxX - cx * CHUNK_SIZE)));
      int startY = fmax(0, (int)(floor(minY)));
      int endY   = fmin(15, (int)(ceil(maxY)));
      int startZ = fmax(0, (int)(floor(minZ - cz * CHUNK_SIZE)));
      int endZ   = fmin(15, (int)(ceil(maxZ - cz * CHUNK_SIZE)));

      for (int x = startX; x <= endX; x++) {
        for (int y = startY; y <= endY; y++) {
          for (int z = startZ; z <= endZ; z++) {
            if (chunkBlockIsSolid(c, x, y, z)) {
              return true;
            }
          }
        }
      }
    }
  }
  return false;
}


#define SUBSTEPS 4
#define EPSILON 0.001f

void physics(world w, camera cam, vec3d velocity, bool* isGrounded, float dt) {
  *isGrounded = false;
  vec3d position = getPosition(cam);
  vec3d stepVel = multiply(velocity, dt / SUBSTEPS);

  for (int i = 0; i < SUBSTEPS; i++) {
    vec3d newPos = copyVector(position);

    // Vertical movement (Y)
    newPos->y += stepVel->y;
    if (collisionCheck(w, newPos)) {
      // Try stepping just slightly off the floor
      newPos->y = position->y;
      if (velocity->y < 0.0f) *isGrounded = true;
      velocity->y = 0.0f;
    } else {
      position->y = newPos->y;
    }

    // Horizontal X
    newPos->x = position->x + stepVel->x;
    if (collisionCheck(w, newPos)) {
      newPos->x = position->x;
      velocity->x = 0.0f;
    } else {
      position->x = newPos->x;
    }

    // Horizontal Z
    newPos->z = position->z + stepVel->z;
    if (collisionCheck(w, newPos)) {
      newPos->z = position->z;
      velocity->z = 0.0f;
    } else {
      position->z = newPos->z;
    }

    free(newPos);
  }

  setXPosition(cam, position->x);
  setYPosition(cam, position->y);
  setZPosition(cam, position->z);
  free(stepVel);
}