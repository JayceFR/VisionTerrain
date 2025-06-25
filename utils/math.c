#include <stdlib.h>
#include <assert.h>
#include "math.h"
#include <math.h>

mat4x4 constructMat4x4(float initialValue){
  mat4x4 matrix = malloc(sizeof(struct mat4x4));
  assert(matrix != NULL);
  for (int i = 0; i < 4; i++){
    for (int j = 0; j < 4; j++){
      matrix->m[i][j] = initialValue;
    }
  }
  return matrix;
}

// REMEMBER to free matrix after use 
mat4x4 identity(){
  mat4x4 matrix = constructMat4x4(0.0f);
  assert(matrix != NULL);
  matrix->m[0][0] = 1.0f;
  matrix->m[1][1] = 1.0f;
  matrix->m[2][2] = 1.0f;
  matrix->m[3][3] = 1.0f;
  return matrix;
}


vec3d constructVec3d(float x, float y, float z){
  vec3d vector = malloc(sizeof(struct vec3d));
  assert(vector != NULL);
  vector->x = x;
  vector->y = y; 
  vector->z = z;
  return vector;
}

#define mutliplyOneCol(Y) i->x * m->m[0][Y] + i->y * m->m[1][Y] + i->z * m->m[2][Y] + m->m[3][Y];
void multiplyMatrixVector(vec3d i, vec3d o, mat4x4 m){
  o->x = mutliplyOneCol(0);
  o->y = mutliplyOneCol(1);
  o->z = mutliplyOneCol(2);
}  

mat4x4 constructRotationY(float theta) {
  mat4x4 mat = constructMat4x4(0.0f);  // Zero matrix

  float c = cosf(theta);
  float s = sinf(theta);

  mat->m[0][0] =  c;
  mat->m[0][2] =  s;
  mat->m[1][1] =  1.0f;
  mat->m[2][0] = -s;
  mat->m[2][2] =  c;
  mat->m[3][3] =  1.0f;

  return mat;
}

mat4x4 constructRotationZ(float theta){
  mat4x4 mat = constructMat4x4(0.0f);

  float c = cosf(theta);
  float s = sinf(theta);

  mat->m[0][0] = c;
  mat->m[0][1] = -s;
  mat->m[1][0] = s;
  mat->m[1][1] = c;
  mat->m[2][2] = 1.0f;
  mat->m[3][3] = 1.0f;

  return mat;
}

mat4x4 constructRotationX(float theta){
  mat4x4 mat = constructMat4x4(0.0f);

  float c = cosf(theta);
  float s = sinf(theta);

  mat->m[0][0] = 1.0f;
  mat->m[1][1] = c;
  mat->m[1][2] = -s;
  mat->m[2][1] = s;
  mat->m[2][2] = c;
  mat->m[3][3] = 1.0f;

  return mat;
}

void translate(mat4x4 m, vec3d v){
  m->m[0][3] = v->x;
  m->m[1][3] = v->y;
  m->m[2][3] = v->z;
}

mat4x4 constructTranslationMatrix(float x, float y, float z){
  mat4x4 mat = constructMat4x4(0.0f);

  // identitit 
  for (int i = 0; i < 4; i++){
    mat->m[i][i] = 1.0f;
  }

  // Set translation
  mat->m[0][3] = x;
  mat->m[1][3] = y;
  mat->m[2][3] = z;

  return mat;
}

float dot(vec3d u, vec3d v){
  return (u->x * v->x) + (u->y * v->y) + (u->z * v->z);
}

// REMEMBER to free
vec3d cross(vec3d u, vec3d v){
  float x = u->y * v->z - u->z * v->y;
  float y = u->z * v->x - u->x * v->z;
  float z = u->x * v->y - u->y * v->x;
  return constructVec3d(x, y, z);
}

// does it inplace 
void normalise(vec3d u){
  float abs = sqrtf(u->x * u->x + u->y * u->y + u->z * u->z);
  u->x /= abs;
  u->y /= abs;
  u->z /= abs;
}

float radians(float deg){
  return deg * PI / 180.0f;
}

// Make sure to free after use 
vec3d add(vec3d u, vec3d v){
  float x = u->x + v->x;
  float y = u->y + v->y;
  float z = u->z + v->z;
  return constructVec3d(x, y, z);
}

// Make sure to free after use 
vec3d subtract(vec3d u, vec3d v){
  float x = u->x - v->x;
  float y = u->y - v->y;
  float z = u->z - v->z;
  return constructVec3d(x, y, z);
}

// Make sure to free after use
vec3d multiply(vec3d v, float scalar) {
  return constructVec3d(v->x * scalar, v->y * scalar, v->z * scalar);
}
