#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <time.h>

#include "glad/glad.h"
#include <GLFW/glfw3.h>

#include "chunk.h"
#include "../utils/math.h"
#include "../utils/shader.h"
#include "../utils/perlin.h"

#define STB_PERLIN_IMPLEMENTATION
#include "../libs/stb_perlin.h"

struct chunk{
  uint8_t blocks[CHUNK_SIZE_X][CHUNK_SIZE_Y][CHUNK_SIZE_Z];
  vec3d position; 
  GLuint vao, vbo;
  GLuint waterVao, waterVbo;
  int numOfVertices;
  int numOfWaterVertices; 
  bool dirty; 
};

typedef struct chunk *chunk;

typedef struct {
  float *data;
  int count; 
  int capacity;
} mesh_buffer;

// Vertices of the cube which gets fed into the VBO 
float backVertices[] = {
  -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f,   0.0f, 0.0f,
  0.5f,  0.5f, -0.5f, 0.0f, 0.0f, -1.0f,  1.0f, 1.0f,
  0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f,  1.0f, 0.0f,
  -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f,  0.0f, 0.0f,
  -0.5f,  0.5f, -0.5f, 0.0f, 0.0f, -1.0f,  0.0f, 1.0f,
  0.5f,  0.5f, -0.5f, 0.0f, 0.0f, -1.0f,  1.0f, 1.0f,
};

float frontVertices[] = {
  -0.5f, -0.5f,  0.5f, 0.0f, 0.0f, 1.0f,   0.0f, 0.0f,
  0.5f, -0.5f,  0.5f, 0.0f, 0.0f, 1.0f,  1.0f, 0.0f,
  0.5f,  0.5f,  0.5f, 0.0f, 0.0f, 1.0f,  1.0f, 1.0f,
  -0.5f, -0.5f,  0.5f, 0.0f, 0.0f, 1.0f,  0.0f, 0.0f,
  0.5f,  0.5f,  0.5f, 0.0f, 0.0f, 1.0f,  1.0f, 1.0f,
  -0.5f,  0.5f,  0.5f, 0.0f, 0.0f, 1.0f,  0.0f, 1.0f,
};

float leftVertices[] = {
  -0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
  -0.5f, -0.5f,  0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
  -0.5f,  0.5f,  0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
  -0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
  -0.5f,  0.5f,  0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
  -0.5f,  0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
};

float rightVertices[] = {
  0.5f, -0.5f,  0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
  0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
  0.5f,  0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
  0.5f, -0.5f,  0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
  0.5f,  0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
  0.5f,  0.5f,  0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
};

float bottomVertices[] = {
  -0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f,  0.0f, 1.0f,
  0.5f, -0.5f, -0.5f,  0.0f, -1.0f, 0.0f, 1.0f, 1.0f,
  0.5f, -0.5f,  0.5f,  0.0f, -1.0f, 0.0f, 1.0f, 0.0f,
  -0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f,  0.0f, 1.0f,
  0.5f, -0.5f,  0.5f,  0.0f, -1.0f, 0.0f, 1.0f, 0.0f,
  -0.5f, -0.5f,  0.5f, 0.0f, -1.0f, 0.0f,  0.0f, 0.0f,
};

float topVertices[] = {
  -0.5f,  0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
  0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
  0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
  -0.5f,  0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
  0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
  -0.5f,  0.5f,  0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
};

float *faceVertices[] = {
  backVertices, 
  frontVertices,
  leftVertices,
  rightVertices,
  bottomVertices,
  topVertices
};

#define MAX_SPRITE 8
textureMap spriteSheet[] = {
  {BLOCK_DIRT, BACK,   0},
  {BLOCK_DIRT, FRONT,  0},
  {BLOCK_DIRT, LEFT,   0},
  {BLOCK_DIRT, RIGHT,  0},
  {BLOCK_DIRT, BOTTOM, 0},
  {BLOCK_DIRT, TOP,    0},

  {BLOCK_GRASS, BACK,   1},
  {BLOCK_GRASS, FRONT,  1},
  {BLOCK_GRASS, LEFT,   1},
  {BLOCK_GRASS, RIGHT,  1},
  {BLOCK_GRASS, BOTTOM, 0},
  {BLOCK_GRASS, TOP,    2},

  {BLOCK_WATER, BACK,   0},
  {BLOCK_WATER, FRONT,  0},
  {BLOCK_WATER, LEFT,   0},
  {BLOCK_WATER, RIGHT,  0},
  {BLOCK_WATER, BOTTOM, 0},
  {BLOCK_WATER, TOP,    0},

  {BLOCK_OAK, BACK,     4},
  {BLOCK_OAK, FRONT,    4},
  {BLOCK_OAK, LEFT,     4},
  {BLOCK_OAK, RIGHT,    4},
  {BLOCK_OAK, BOTTOM,   4},
  {BLOCK_OAK, TOP,      4},

  {BLOCK_LEAF, BACK,    6},
  {BLOCK_LEAF, FRONT,   6},
  {BLOCK_LEAF, LEFT,    6},
  {BLOCK_LEAF, RIGHT,   6},
  {BLOCK_LEAF, BOTTOM,  6},
  {BLOCK_LEAF, TOP,     6},
  
  
  {BLOCK_NULL, LEFT, -1},
};

static void initMeshBuffer(mesh_buffer *mesh) {
  mesh->data = malloc(INITIAL_CAPACITY * sizeof(float));
  mesh->count = 0;
  mesh->capacity = INITIAL_CAPACITY;
}

static void freeMeshBuffer(mesh_buffer *mesh) {
  free(mesh->data);
  mesh->data = NULL;
  mesh->count = 0;
  mesh->capacity = 0;
}

static void growMeshBuffer(mesh_buffer *mesh) {
  mesh->capacity *= 2;
  mesh->data = realloc(mesh->data, mesh->capacity * sizeof(float));
}

static int getTexture(BLOCK_TYPE type, FACE face){
  for (int i = 0; spriteSheet[i].type != BLOCK_NULL; i++){
    if (spriteSheet[i].type == type && spriteSheet[i].face == face){
      return spriteSheet[i].value;
    }
  }
  fprintf(stderr, "Texture is not present");
  exit(EXIT_FAILURE);
}

static void addFace(mesh_buffer *mesh, float *face, int bx, int by, int bz, BLOCK_TYPE type, FACE f) {
  for (int i = 0; i < VERTEX_COUNT; i += 8) {
    if (mesh->count + 8 > mesh->capacity) {
        growMeshBuffer(mesh);
    }
    mesh->data[mesh->count++] = face[i + 0] + bx; 
    mesh->data[mesh->count++] = face[i + 1] + by; 
    mesh->data[mesh->count++] = face[i + 2] + bz; 
    
    // normals 
    mesh->data[mesh->count++] = face[i+3];
    mesh->data[mesh->count++] = face[i+4];
    mesh->data[mesh->count++] = face[i+5];

    float uvx = mesh->data[mesh->count];
    uvx = (1.0f / MAX_SPRITE) * (face[i+6] + getTexture(type, f));
    #define offset 0.01f
    uvx += offset;
    uvx -= offset;
    mesh->data[mesh->count++] = uvx; 
    mesh->data[mesh->count++] = face[i + 7]; 
  }
}

static void addAllFaces(mesh_buffer *mesh, int bx, int by, int bz, BLOCK_TYPE type){
  for (int i = 0; i < FACE_COUNT; i++){
    addFace(mesh, faceVertices[i], bx, by, bz, type, i);
  }
}

static void rebuildChunkMesh(chunk c){
  mesh_buffer mesh;
  mesh_buffer waterMesh; 
  initMeshBuffer(&mesh);
  initMeshBuffer(&waterMesh);

  for (int x = 0; x < CHUNK_SIZE_X; x++) {
    for (int y = 0; y < CHUNK_SIZE_Y; y++) {
      for (int z = 0; z < CHUNK_SIZE_Z; z++) {

        BLOCK_TYPE type = c->blocks[x][y][z];

        if (c->blocks[x][y][z] == BLOCK_AIR) continue;

        mesh_buffer *targetMesh = (type == BLOCK_WATER) ? &waterMesh : &mesh;
        // if (type == BLOCK_WATER){
        //   continue;
        // }
        // mesh_buffer *targetMesh = &mesh;

        // LEFT
        if (x == 0 || c->blocks[x-1][y][z] == BLOCK_AIR){
          addFace(targetMesh, faceVertices[LEFT], x, y, z, type, LEFT);
        }

        // RIGHT
        if (x == CHUNK_SIZE_X - 1 || c->blocks[x+1][y][z] == BLOCK_AIR){
          addFace(targetMesh, faceVertices[RIGHT], x, y, z, type, RIGHT);
        }

        // FRONT
        if (z == CHUNK_SIZE_Z - 1 || c->blocks[x][y][z+1] == BLOCK_AIR){
          addFace(targetMesh, faceVertices[FRONT], x, y, z, type, FRONT);
        }

        // BACK
        if (z == 0 || c->blocks[x][y][z-1] == BLOCK_AIR){
          addFace(targetMesh, faceVertices[BACK], x, y, z, type, BACK);
        }

        // TOP
        if (y == CHUNK_SIZE_Y - 1 || c->blocks[x][y+1][z] == BLOCK_AIR){
          addFace(targetMesh, faceVertices[TOP], x, y, z, type, TOP);
        }

        // BOTTOM
        if (y == 0 || c->blocks[x][y-1][z] == BLOCK_AIR){
          addFace(targetMesh, faceVertices[BOTTOM], x, y, z, type, BOTTOM);
        }
      }
    }
  }


  // Load into GPU
  if (c->vao == 0) glGenVertexArrays(1, &c->vao);
  if (c->vbo == 0) glGenBuffers(1, &c->vbo);

  glBindVertexArray(c->vao);
  glBindBuffer(GL_ARRAY_BUFFER, c->vbo);
  glBufferData(GL_ARRAY_BUFFER, mesh.count * sizeof(float), mesh.data, GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);

  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);

  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
  glEnableVertexAttribArray(2);

  c->numOfVertices = mesh.count / 8;
  c->dirty = false;

  if (c->waterVao == 0) glGenVertexArrays(1, &c->waterVao);
  if (c->waterVbo == 0) glGenBuffers(1, &c->waterVbo);

  glBindVertexArray(c->waterVao);
  glBindBuffer(GL_ARRAY_BUFFER, c->waterVbo);
  glBufferData(GL_ARRAY_BUFFER, waterMesh.count * sizeof(float), waterMesh.data, GL_STATIC_DRAW);

  // Setup attributes...
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);

  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);

  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
  glEnableVertexAttribArray(2);

  c->numOfWaterVertices = waterMesh.count / 8;

  freeMeshBuffer(&mesh);
  freeMeshBuffer(&waterMesh);
}

static float octaveNoise(float x, float y, float z) {
  float total = 0.0f;
  float frequency = 1.0f;
  float amplitude = 1.0f;
  float maxValue = 0.0f;
  int octaves = 4;

  for (int i = 0; i < octaves; i++) {
    total += stb_perlin_noise3(x * frequency, y * frequency, z * frequency, 0, 0, 0) * amplitude;
    maxValue += amplitude;
    amplitude *= 0.5f;   // decrease amplitude each octave
    frequency *= 2.0f;   // increase frequency each octave
  }
  return total / maxValue; // normalize result to approx [-1,1]
}

bool chunkBlockIsSolid(chunk c, int x, int y, int z){
  uint8_t block = c->blocks[x][y][z];
  return block == BLOCK_DIRT || block == BLOCK_OAK || block == BLOCK_GRASS || block == BLOCK_LEAF || block == BLOCK_WATER;
}

float islandHeight(int x, int y, int size) {
  // Normalize coordinates to [-1, 1]
  float nx = (2.0f * x) / (size - 1) - 1.0f;
  float ny = (2.0f * y) / (size - 1) - 1.0f;

  // Distance from center (0,0)
  float dist = sqrtf(nx * nx + ny * ny);

  // Basic radial mask to create island shape: more land near center, water near edges
  float edgeFalloff = 1.0f - dist;
  if (edgeFalloff < 0) edgeFalloff = 0;

  // Generate base terrain noise at different frequencies for detail
  float elevation =
      0.6f * noise2D(4.0f * nx, 4.0f * ny) +  // base shape
      0.3f * noise2D(8.0f * nx, 8.0f * ny) +  // detail
      0.1f * noise2D(16.0f * nx, 16.0f * ny); // fine detail

  // Normalize elevation roughly between -1 and 1 (depends on noise function)
  // Multiply by radial mask so edges go down toward water
  elevation *= edgeFalloff;

  // Clamp elevation to [0, 1], so negative values become water (0)
  if (elevation < 0) elevation = 0;
  if (elevation > 1) elevation = 1;

  return elevation;
}

chunk createChunk(float x, float y, float z) {
  chunk new = malloc(sizeof(struct chunk));
  assert(new != NULL);

  new->position = constructVec3d(x, y, z);

  for (int cx = 0; cx < CHUNK_SIZE_X; cx++) {
    for (int cz = 0; cz < CHUNK_SIZE_Z; cz++) {
      // Calculate world coordinates for noise sampling
      float worldX = new->position->x + (float)cx;
      float worldZ = new->position->z + (float)cz;

      float noiseVal = octaveNoise(worldX * 0.1f, 0.0f, worldZ * 0.1f);

      float normalized = noiseVal * 0.5f + 0.5f;

      float heightFloat = powf(normalized, 3.0f) * 20.0f;

      // float heightFloat = islandHeight(cx, cz, 19) * 40.0f;

      int maxHeight = (int) heightFloat;

      for (int cy = 0; cy < CHUNK_SIZE_Y; cy++) {
        if (cy < 3){
          new->blocks[cx][cy][cz] = BLOCK_WATER;
        }
        else if (cy < maxHeight - 1) {
            new->blocks[cx][cy][cz] = BLOCK_DIRT;
        }
        else if (cy == maxHeight - 1) {
            new->blocks[cx][cy][cz] = BLOCK_GRASS;
        }
        else {
            new->blocks[cx][cy][cz] = BLOCK_AIR;
        }
      }

    }
  }

  srand(time(NULL));

  for (int cx = 1; cx < CHUNK_SIZE_X - 1; cx++){
    for (int cz = 1; cz < CHUNK_SIZE_Z - 1; cz++){
      for (int cy = 0; cy < CHUNK_SIZE_Y - 7; cy++){
        if ( rand() % 63 == 0 && new->blocks[cx][cy][cz] == BLOCK_GRASS){

          // add a tree
          new->blocks[cx][cy+1][cz] = BLOCK_OAK;
          new->blocks[cx][cy+2][cz] = BLOCK_OAK;
          new->blocks[cx][cy+3][cz] = BLOCK_OAK;


          // 2 layers of leaves
          #define CURRBLOCK new->blocks[cx + xoffset][cy+yoffset][cz + zoffset]
          for (int yoffset = 4; yoffset < 5; yoffset++){
            for (int xoffset = -1; xoffset < 2; xoffset++){
              for (int zoffset = -1; zoffset < 2; zoffset++){
                if (CURRBLOCK == BLOCK_AIR){
                  CURRBLOCK = BLOCK_LEAF;
                }
              }
            }
          }

          #define CURRBLOCK new->blocks[cx + xoffset][cy + 5][cz]
          for (int xoffset = -1; xoffset < 2; xoffset++){
            if (CURRBLOCK == BLOCK_AIR){
              CURRBLOCK = BLOCK_LEAF;
            }
          }

          #define CURRBLOCK new->blocks[cx][cy + 5][cz + zoffset]
          for (int zoffset = -1; zoffset < 2; zoffset++){
            if (CURRBLOCK == BLOCK_AIR){
              CURRBLOCK = BLOCK_LEAF;
            }
          }

          new->blocks[cx][cy+6][cz] = BLOCK_LEAF;

        }
      }
    }
  }

  new->vao           = 0;
  new->vbo           = 0;
  new->numOfVertices = 0;
  new->waterVao      = 0;
  new->waterVbo      = 0;
  new->dirty         = true;

  return new;
}


void freeChunk(chunk c){
  free(c->position);
  free(c);
}

void renderChunk(
  chunk c,  
  GLuint program, 
  GLuint waterShader,
  mat4x4 view, 
  mat4x4 proj,
  vec3d lightPos,
  vec3d viewPos,
  float time, GLuint texture, 
  bool fake, GLuint reflectedTex, 
  GLuint dudvTex, GLuint normalTex)
{
  if (c->dirty) {
    printf("Calling rebuild chunk mesh");
    rebuildChunkMesh(c);
    printf("Number of vertices present = %d", c->numOfVertices);
    c->dirty = false;
  }

  mat4x4 model = constructTranslationMatrix(c->position->x, c->position->y, c->position->z);
  
  useShader(program, model, view, proj, lightPos, viewPos, time, texture, reflectedTex, dudvTex, normalTex);
  glBindVertexArray(c->vao);
  glDrawArrays(GL_TRIANGLES, 0, c->numOfVertices);

  if (!fake){
    useShader(waterShader, model, view, proj, lightPos, viewPos, time, texture, reflectedTex, dudvTex, normalTex);
    glBindVertexArray(c->waterVao);
    glDrawArrays(GL_TRIANGLES, 0, c->numOfWaterVertices);
  }
}