#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>

#include "camera.h"
#include "../utils/math.h"
#include "chunk.h"
#include "world.h"
#include "../adts/hash.h"

#define PLAYER_WIDTH 0.6f 
#define PLAYER_HEIGHT 1.8f
#define CHUNK_SIZE 16.0f 

static bool collisionCheck(world w, vec3d newPos) {
  float minX = newPos->x - PLAYER_WIDTH / 2.0f;
  float maxX = newPos->x + PLAYER_WIDTH / 2.0f;
  float minY = newPos->y;
  float maxY = newPos->y + PLAYER_HEIGHT;
  float minZ = newPos->z - PLAYER_WIDTH / 2.0f;
  float maxZ = newPos->z + PLAYER_WIDTH / 2.0f;

  if (minY >= 16) {
    return false; // No collision possible above 16
  }

  int minChunkX = floor(minX / CHUNK_SIZE);
  int maxChunkX = floor(maxX / CHUNK_SIZE);
  int minChunkZ = floor(minZ / CHUNK_SIZE);
  int maxChunkZ = floor(maxZ / CHUNK_SIZE);

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


#define MAX_PHYSICS_STEPS 5

void physics(world w, camera cam, vec3d velocity, bool* isGrounded, float dt) {
  *isGrounded = false;

  vec3d pos = getPosition(cam);
  vec3d newPos = copyVector(pos);

  float subDt = dt / MAX_PHYSICS_STEPS;
  for (int step = 0; step < MAX_PHYSICS_STEPS; step++) {
    // Y axis
    newPos->y += velocity->y * subDt;
    if (collisionCheck(w, newPos)) {
      newPos->y -= velocity->y * subDt;
      if (velocity->y < 0.0f) {
        *isGrounded = true;
      }
      velocity->y = 0.0f;
    }

    // X axis
    newPos->x += velocity->x * subDt;
    if (collisionCheck(w, newPos)) {
      newPos->x -= velocity->x * subDt;
      velocity->x = 0.0f;
    }

    // Z axis
    newPos->z += velocity->z * subDt;
    if (collisionCheck(w, newPos)) {
      newPos->z -= velocity->z * subDt;
      velocity->z = 0.0f;
    }
  }

  setXPosition(cam, newPos->x);
  setYPosition(cam, newPos->y);
  setZPosition(cam, newPos->z);

  free(newPos);
}

