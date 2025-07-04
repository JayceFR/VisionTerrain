#ifndef PHYSICS_H
#define PHYSICS_H

#include "world.h"
#include "camera.h"

extern void physics(world w, camera cam, vec3d velocity, bool *grounded, float dt);

#endif