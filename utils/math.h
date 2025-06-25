#ifndef MATH_H
#define MATH_H

struct vec3d{
  float x;
  float y;
  float z;
}; 
typedef struct vec3d *vec3d; 

struct mat4x4{
  //row - column
  float m[4][4];
};
typedef struct mat4x4 *mat4x4; 

#define PI 3.141593

extern mat4x4 constructMat4x4(float initialValue);
extern vec3d constructVec3d(float x, float y, float z);
extern mat4x4 constructRotationY(float theta);
extern mat4x4 constructRotationZ(float theta);
extern mat4x4 constructRotationX(float theta);
extern mat4x4 identity();
extern void translate(mat4x4 m, vec3d v);
extern mat4x4 constructTranslationMatrix(float x, float y, float z);
extern float dot(vec3d u, vec3d v);
// REMEMBER to free
extern vec3d cross(vec3d u, vec3d v);
extern vec3d add(vec3d u, vec3d v);
extern vec3d subtract(vec3d u, vec3d v);
extern vec3d multiply(vec3d v, float scalar);
// does it inplace 
extern void normalise(vec3d u);
extern float radians(float deg);

#endif