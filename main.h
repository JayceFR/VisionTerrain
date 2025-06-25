#ifndef MAIN_H
#define MAIN_H

#define SCREEN_WIDTH  800
#define SCREEN_HEIGHT 800

#define CENTER_X SCREEN_WIDTH / 2.0; 
#define CENTER_Y SCREEN_HEIGHT / 2.0; 

#define SENSITIVITY 0.0125

#define SOCK_ADD 5005

#define DEPTH_SIZE 24 

// projection matrix stuff
#define FNEAR 0.1f
#define FFAR  1000.0f
#define FFOV  90.0f

#define TARGET_FPS 60.0
#define FRAME_TIME 1.0 / TARGET_FPS

#define CAM_SPEED 5.0f

#define MINI_SCREEN_WIDTH  256
#define MINI_SCREEN_HEIGHT 256
#define BACKGROUND_COLOR 0.1f, 0.1f, 0.1f

#define FIREFLY_COUNT 500
#define TILES (2 * 5 + 1) * (2 * 5 + 1)

#define STB_IMAGE_IMPLEMENTATION

#define MAX_LANDMARKS 32

#define SOCK_RECV_LEN 512

#define NUM_OF_LANDMARKS landmarkCount * 3

#endif