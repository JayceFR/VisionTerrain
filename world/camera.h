#ifndef CAMERA_H
#define CAMERA_H

#include "../utils/math.h"

struct camera{};
typedef struct camera *camera;

extern camera constructCamera(float x, float y, float z);
extern void freeCamera(camera cam);
extern vec3d getFrontVector(float yaw, float pitch);
mat4x4 lookAt(vec3d position, vec3d target, vec3d up);
// getters
extern float getYaw(camera cam);
extern float getPitch(camera cam);
extern vec3d getPosition(camera cam);
// setters
extern void setPosition(camera cam, vec3d position);
extern void setYaw(camera cam, float value);
extern void setPitch(camera cam, float value);
extern void setYPosition(camera cam, float new_y);

#endif