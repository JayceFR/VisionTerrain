#include <stdlib.h>
#include <math.h>

#include "../utils/math.h"

struct camera{
  vec3d position;
  float yaw;   // rot around Y axis 
  float pitch; //  rot around X axis 
};
typedef struct camera *camera; 

// Make sure to free after usage
camera constructCamera(float x, float y, float z){
  camera cam = malloc(sizeof(struct camera));
  cam->position = constructVec3d(x,y,z);
  cam->yaw      = -90.0f;
  cam->pitch    = 0.0f;
  return cam;
}

void freeCamera(camera cam){
  free(cam->position);
  free(cam);
}

float getYaw(camera cam){
  return cam->yaw;
}

void setYaw(camera cam, float value){
  cam->yaw = value;
}

float getPitch(camera cam){
  return cam->pitch;
}

void setPitch(camera cam, float value){
  cam->pitch = value;
}

vec3d getPosition(camera cam){
  return cam->position;
}

void setPosition(camera cam, vec3d position){
  free(cam->position);
  cam->position = position;
}

void setXPosition(camera cam, float new_x){
  cam->position->x = new_x; 
}

void setYPosition(camera cam, float new_y){
  cam->position->y = new_y;
}

void setZPosition(camera cam, float new_z){
  cam->position->z = new_z;
}

// Make sure to free it after usage
vec3d getFrontVector(float yaw, float pitch){
  float x = cosf(radians(yaw)) * cosf(radians(pitch));
  float y = sinf(radians(pitch));
  float z = sinf(radians(yaw)) * cosf(radians(pitch));
  vec3d retVec = constructVec3d(x, y, z);
  normalise(retVec);
  return retVec;
}

mat4x4 lookAt(vec3d position, vec3d target, vec3d up){
  vec3d zaxis = subtract(position, target);
  normalise(zaxis);
  vec3d xaxis = cross(up, zaxis);
  normalise(xaxis);
  vec3d yaxis = cross(zaxis, xaxis);

  mat4x4 view = identity();

  view->m[0][0] = xaxis->x;
  view->m[0][1] = xaxis->y;
  view->m[0][2] = xaxis->z;
  view->m[0][3] = -dot(xaxis, position);

  view->m[1][0] = yaxis->x;
  view->m[1][1] = yaxis->y;
  view->m[1][2] = yaxis->z;
  view->m[1][3] = -dot(yaxis, position);

  view->m[2][0] = zaxis->x;
  view->m[2][1] = zaxis->y;
  view->m[2][2] = zaxis->z;
  view->m[2][3] = -dot(zaxis, position);

  view->m[3][0] = 0.0f;
  view->m[3][1] = 0.0f;
  view->m[3][2] = 0.0f;
  view->m[3][3] = 1.0f;

  free(zaxis);
  free(xaxis);
  free(yaxis);
  return view;
}
