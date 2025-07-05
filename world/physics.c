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


void physics(world w, camera cam, vec3d velocity, bool* isGrounded, float dt) {
  *isGrounded = false;

  vec3d originalPos = getPosition(cam);
  vec3d testPos = copyVector(originalPos);

  // Try Y first (gravity)
  testPos->y += velocity->y * dt;
  if (collisionCheck(w, testPos)) {
    testPos->y = originalPos->y;
    *isGrounded = true;
    velocity->y = 0.0f;
  }

  // X: Start from the Y-resolved position
  testPos->x += velocity->x * dt;
  if (collisionCheck(w, testPos)) {
    testPos->x = originalPos->x;
  }

  // Z: Start from Y/X-resolved position
  testPos->z += velocity->z * dt;
  if (collisionCheck(w, testPos)) {
    testPos->z = originalPos->z;
  }

  // Apply position at the end
  setXPosition(cam, testPos->x);
  setYPosition(cam, testPos->y);
  setZPosition(cam, testPos->z);

  free(testPos);
}