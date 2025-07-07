#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#ifndef __EMSCRIPTEN__
  #include "glad/glad.h"
#else
  #include <GLES3/gl3.h>
#endif
#include <GLFW/glfw3.h> 

#include "main.h"
#include "utils/shader.h"
#include "utils/math.h"
#include "world/world.h"
#include "world/camera.h"
#include "world/physics.h"
#include "utils/perlin.h"

#include "utils/texture.h"

#ifdef __EMSCRIPTEN__
  #include <emscripten/emscripten.h>
#endif


void framebuffer_size_callback(GLFWwindow* window, int width, int height){
  glViewport(0, 0, width, height);
}


void checkOpenGLError(const char* context) {
  GLenum err;
  while ((err = glGetError()) != GL_NO_ERROR) {
    fprintf(stderr, "OpenGL error in %s: 0x%X\n", context, err);
  }
}

void sleepSeconds(double seconds){
#ifdef __EMSCRIPTEN__
  // No sleep or use emscripten_sleep if async:
  // For synchronous wait, do nothing or busy-wait (not recommended)
#else
  #ifdef _WIN32
    Sleep((DWORD)(seconds * 1000));
  #else
    struct timespec ts;
    ts.tv_sec  = (time_t)seconds;
    ts.tv_nsec = (long)((seconds - ts.tv_sec) * 1e9);
    nanosleep(&ts, NULL);
  #endif
#endif
}


static bool firstMouse = true;
static double lastX = SCREEN_WIDTH / 2.0;
static double lastY = SCREEN_HEIGHT / 2.0;
static camera cam; // required for mouseCallback


void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
  if (firstMouse) {
    lastX = xpos;
    lastY = ypos;
    firstMouse = false;
    return;
  }

  float xoffset = (float)(xpos - lastX);
  float yoffset = (float)(lastY - ypos);

  lastX = xpos;
  lastY = ypos;

  float sensitivity = 0.025f; 
  xoffset *= sensitivity;
  yoffset *= sensitivity;

  // Update yaw and pitch
  float yaw = getYaw(cam) + xoffset;
  float pitch = getPitch(cam) + yoffset;

  // Clamp pitch
  if (pitch > 89.0f)
      pitch = 89.0f;
  if (pitch < -89.0f)
      pitch = -89.0f;

  setYaw(cam, yaw);
  setPitch(cam, pitch);
}

typedef struct { float x, y, z; } Point3D;
#define MAX_LANDMARKS 32

bool getFaceData(Point3D* outPoints, int* outCount, float* yaw, float* pitch) {
  // static int sockfd = -1;
  // static struct sockaddr_in addr;

  // if (sockfd < 0) {
  //     sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  //     addr.sin_family = AF_INET;
  //     addr.sin_port = htons(5005);
  //     addr.sin_addr.s_addr = INADDR_ANY;
  //     bind(sockfd, (struct sockaddr*)&addr, sizeof(addr));
  // }

  // char buffer[512];
  // ssize_t len = recv(sockfd, buffer, sizeof(buffer), MSG_DONTWAIT);
  // if (len <= 0) return false;

  // int floatCount = len / sizeof(float);
  // if (floatCount < 2) return false;

  // float* floats = (float*)buffer;
  // int landmarkCount = (floatCount - 2) / 3;

  // if (landmarkCount > MAX_LANDMARKS) landmarkCount = MAX_LANDMARKS;

  // for (int i = 0; i < landmarkCount; ++i) {
  //     outPoints[i] = (Point3D){ floats[i * 3 + 0], floats[i * 3 + 1], floats[i * 3 + 2] };
  // }

  // *yaw = floats[landmarkCount * 3];
  // *pitch = floats[landmarkCount * 3 + 1];
  // *outCount = landmarkCount;
  return false;
}

void initFBO(int width, int height, GLuint *fbo, GLuint *fboTex, GLuint *fboDepth) {
  glGenFramebuffers(1, fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, *fbo);

  glGenTextures(1, fboTex);
  glBindTexture(GL_TEXTURE_2D, *fboTex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0,
               GL_RGB, GL_UNSIGNED_BYTE, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                         GL_TEXTURE_2D, *fboTex, 0);

  glGenRenderbuffers(1, fboDepth);
  glBindRenderbuffer(GL_RENDERBUFFER, *fboDepth);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                            GL_RENDERBUFFER, *fboDepth);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    printf("ERROR: FBO not complete\n");

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

typedef struct LoopCtx {
  double lastFrameTime;
  double frameTime;
  vec3d front;
  vec3d right; 
  vec3d up;
  GLFWwindow* window;
  vec3d velocity;
  vec3d viewPos;
  vec3d lightPos; 
  world game;
  mat4x4 view;
  GLuint fbo;
  GLuint faceVBO;
  GLuint faceVAO;
  GLuint skyVAO;
  GLuint dubFbo;
  mat4x4 matProj;
  GLuint texture;
  GLuint dubTex;
  GLuint dudvTexture;
  GLuint normalTexture;
  GLuint fireflyVAO; 
  GLuint fboTex; 
  GLuint screenVAO;
  GLuint program;   
  GLuint waterShader;
  GLuint skyShader; 
  GLuint fireflyShader;
  GLuint faceShader;
  GLuint facyShader;
} LoopCtx;

static void main_loop(void *arg) {
  LoopCtx *ctx = (LoopCtx *) arg;
  double now = glfwGetTime();
  double deltaTime = now - ctx->lastFrameTime;

  if (deltaTime < ctx->frameTime) {
      // Sleep remaining time to avoid busy wait
      sleepSeconds(ctx->frameTime - deltaTime);
      return;
  }

  ctx->lastFrameTime = now;

  glfwPollEvents();
  free(ctx->front);
  ctx->front = getFrontVector(getYaw(cam), getPitch(cam));
  free(ctx->right);
  ctx->right = cross(ctx->front, ctx->up);
  normalise(ctx->right);

  vec3d flatFront = constructVec3d(ctx->front->x, 0.0f, ctx->front->z);
  normalise(flatFront);

  float cameraSpeed = 5.0f * (float)deltaTime;

  ctx->velocity->y -= 4.81f;
  ctx->velocity->x = 0.0f;
  ctx->velocity->z = 0.0f; 

  float speed = 4.5f;

  if (glfwGetKey(ctx->window, GLFW_KEY_W) == GLFW_PRESS) {
    vec3d forwardStep = multiply(flatFront, speed);
    ctx->velocity = add(ctx->velocity, forwardStep);
    // setPosition(cam, add(getPosition(cam), multiply(front, cameraSpeed)));
  }
  if (glfwGetKey(ctx->window, GLFW_KEY_S) == GLFW_PRESS) {
    vec3d backStep = multiply(flatFront, -speed);
    ctx->velocity = add(ctx->velocity, backStep);
    // setPosition(cam, subtract(getPosition(cam), multiply(front, cameraSpeed)));
  }
  if (glfwGetKey(ctx->window, GLFW_KEY_A) == GLFW_PRESS) {
    vec3d leftStep = multiply(ctx->right, -speed);
    ctx->velocity = add(ctx->velocity, leftStep);
    // setPosition(cam, subtract(getPosition(cam), multiply(right, cameraSpeed)));
  }
  if (glfwGetKey(ctx->window, GLFW_KEY_D) == GLFW_PRESS) {
    vec3d rightStep = multiply(ctx->right, speed);
    ctx->velocity = add(ctx->velocity, rightStep);
    // setPosition(cam, add(getPosition(cam), multiply(right, cameraSpeed)));
  }

  // velocity->y -= 9.81f;

  bool grounded = false;
  physics(ctx->game, cam, ctx->velocity, &grounded, (float) deltaTime);

  if (grounded){
    ctx->velocity->y = 0.0f;
    if (glfwGetKey(ctx->window, GLFW_KEY_SPACE) == GLFW_PRESS){
      ctx->velocity->y = 40.0f;
    }
  }

  float eye_offset = 1.61f;
  float forward_offset = 0.5f;  // tweak this to avoid clipping

  vec3d camPos = getPosition(cam);  // player base position
  vec3d camForward = getFrontVector(getYaw(cam), getPitch(cam));
  normalise(camForward);

  vec3d eyePos = constructVec3d(
      camPos->x + camForward->x * forward_offset,
      camPos->y + eye_offset,
      camPos->z + camForward->z * forward_offset);

  ctx->view = lookAt(eyePos, add(eyePos, camForward), ctx->up);



  // Rendering
  // Face screen 
  glBindFramebuffer(GL_FRAMEBUFFER, ctx->fbo);
  glViewport(0, 0, 256, 256);
  glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // render to the fbo
  Point3D facePoints[MAX_LANDMARKS];
  int facePointCount = 0;
  float yaw = 0.0f, pitch = 0.0f;

  if (getFaceData(facePoints, &facePointCount, &yaw, &pitch)) {
    for (int i = 0; i < facePointCount; ++i) {
      Point3D p = facePoints[i];
      p.x = p.x * 2.0f - 1.0f;
      p.y = 1.0f - p.y * 2.0f;
      p.z = 0.0f;
      facePoints[i] = p;
    }

    glBindBuffer(GL_ARRAY_BUFFER, ctx->faceVBO);
    glBufferData(GL_ARRAY_BUFFER, facePointCount * sizeof(Point3D), facePoints, GL_DYNAMIC_DRAW);
    setYaw(cam, yaw * 180.0f / 3.14159f);
    setPitch(cam, pitch * 180.0f / 3.14159f);

    glBindVertexArray(ctx->faceVAO);
    glUseProgram(ctx->facyShader);
    glPointSize(10.0f);
    glDrawArrays(GL_POINTS, 0, facePointCount);
    glPointSize(1.0f);
    glBindVertexArray(0);
  }

    // reset stuff
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

  // Main screen 
  // Clear screen 
  glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Render the background
  glDepthMask(GL_FALSE);
  glUseProgram(ctx->skyShader); 
  glBindVertexArray(ctx->skyVAO);
  glDrawArrays(GL_TRIANGLES, 0, 6);
  glDepthMask(GL_TRUE);

  // update view matrix
  ctx->viewPos->x = eyePos->x;
  ctx->viewPos->y = eyePos->y;
  ctx->viewPos->z = eyePos->z;
  float currTime = glfwGetTime();

  // fake screen 
  glBindFramebuffer(GL_FRAMEBUFFER, ctx->dubFbo);
  glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
  glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  // render background to fake screen 
  glDepthMask(GL_FALSE);
  glUseProgram(ctx->skyShader); 
  glBindVertexArray(ctx->skyVAO);
  glDrawArrays(GL_TRIANGLES, 0, 6);
  glDepthMask(GL_TRUE);
  // render world to fake screen 
  float distance = 2 * (eyePos->y - 3.0f);
  vec3d distancePos = constructVec3d(eyePos->x, eyePos->y - distance, eyePos->z);
  setPitch(cam, -getPitch(cam));
  vec3d newUp = constructVec3d(0.0f, 1.0f, 0.0f);
  ctx->view = lookAt(distancePos, add(distancePos, getFrontVector(getYaw(cam), getPitch(cam))), newUp);

  renderWorld(ctx->game, distancePos, ctx->program, ctx->waterShader, ctx->view, ctx->matProj, ctx->lightPos, ctx->viewPos, currTime, ctx->texture, true, ctx->dubTex, ctx->dudvTexture, ctx->normalTexture);
  setPitch(cam, -getPitch(cam));
  // reset to normal frame buffer
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
  ctx->view = lookAt(eyePos, add(eyePos, ctx->front), ctx->up);

  // render the world
  renderWorld(ctx->game, eyePos, ctx->program, ctx->waterShader, ctx->view, ctx->matProj, ctx->lightPos, ctx->viewPos, currTime, ctx->texture, false, ctx->dubTex, ctx->dudvTexture, ctx->normalTexture);

  // render the ui 
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDepthMask(GL_FALSE);
  glUseProgram(ctx->fireflyShader); 
  glBindVertexArray(ctx->fireflyVAO);

  glUniform1f(glGetUniformLocation(ctx->fireflyShader, "time"), glfwGetTime());
  glUniformMatrix4fv(glGetUniformLocation(ctx->fireflyShader, "matProj"), 1, GL_TRUE, (float*)ctx->matProj->m);
  glUniformMatrix4fv(glGetUniformLocation(ctx->fireflyShader, "view"), 1, GL_TRUE, (float*)ctx->view->m);
  glUniform3f(glGetUniformLocation(ctx->fireflyShader, "cameraWorldPos"), eyePos->x, eyePos->y, eyePos->z);

  int fireflyCount = 400;
  int tiles = (2 * 5 + 1) * (2 * 5 + 1);
  glPointSize(2.0f);
  glDrawArrays(GL_POINTS, 0, fireflyCount * tiles);
  glPointSize(1.0f);
  glDepthMask(GL_TRUE);
  glDisable(GL_BLEND);

  glUseProgram(ctx->faceShader);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, ctx->fboTex);
  glUniform1i(glGetUniformLocation(ctx->faceShader, "screenTex"), 0);

  glBindVertexArray(ctx->screenVAO);
  glDrawArrays(GL_TRIANGLES, 0, 6);

  checkOpenGLError("In loop");

  glfwSwapBuffers(ctx->window);
}

int main(){
  if (!glfwInit()){
    fprintf(stderr, "Failed to initilaise GLFW window");
    exit(1);
  }


  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  glfwWindowHint(GLFW_DEPTH_BITS, 24);
  GLFWwindow* window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Hello World", NULL, NULL);
  if (!window){
    fprintf(stderr, "Failed to create GLFW window\n");
    glfwTerminate();
    return EXIT_FAILURE;
  }
  glfwMakeContextCurrent(window);

  #ifndef __EMSCRIPTEN__
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    fprintf(stderr, "Failed to initialize GLAD\n");
    return EXIT_FAILURE;
  }
  #endif

  glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

  // compile shaders into one program
  GLuint program       = compileShader("shader/test.vert", "shader/test.frag");
  GLuint waterShader   = compileShader("shader/water.vert", "shader/water.frag");
  GLuint skyShader     = compileShader("shader/background.vert", "shader/background.frag");
  GLuint fireflyShader = compileShader("shader/firefly.vert", "shader/firefly.frag");
  GLuint faceShader    = compileShader("shader/face.vert", "shader/face.frag");
  GLuint facyShader    = compileShader("shader/facy.vert", "shader/facy.frag");

  GLint isLinked = 0;
  glGetProgramiv(program, GL_LINK_STATUS, &isLinked);
  if (!isLinked) {
    GLint maxLength = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);
    char* infoLog = malloc(maxLength);
    glGetProgramInfoLog(program, maxLength, &maxLength, infoLog);
    printf("\nShader Link Error:\n%s\n", infoLog);
    free(infoLog);
  }

  // projection matrix stuff
  float fNear        = 0.05f;
  float fFar         = 1000.0f;
  float fFov         = 90.0f;
  float fAspectRatio = (float)(SCREEN_WIDTH) / (float)(SCREEN_HEIGHT);
  float fFovRad      = 1.0f / tanf(fFov * 0.5f / 180.0f * 3.14159f);

  // create the projection matrix
  mat4x4 matProj = constructMat4x4(0.0f);
  matProj->m[0][0] = fAspectRatio * fFovRad;
  matProj->m[1][1] = fFovRad;
  matProj->m[2][2] = -(fFar + fNear) / (fFar - fNear);
  matProj->m[3][2] = -1.0f;
  matProj->m[2][3] = -(2.0f * fFar * fNear) / (fFar - fNear);
  matProj->m[3][3] = 0.0f;

  float angle = 0.0f;
  mat4x4 view = constructRotationY(angle);
  view->m[2][3] = -16.0f;

  // load textures 
  GLuint texture = loadTexture("texture/tile.png", 64, 16, 3);
  GLuint dudvTexture = loadTexture("texture/waterDUDV.png", 512, 512, 3);
  GLuint normalTexture = loadTexture("texture/normalMap.png", 512, 512, 3);

  glEnable(GL_DEPTH_TEST);

  glfwSwapInterval(1);

  checkOpenGLError("depth error stuff");

  // The background
  float quadVertices[] = {
    -1.0f,  1.0f,
    -1.0f, -1.0f,
      1.0f, -1.0f,

    -1.0f,  1.0f,
      1.0f, -1.0f,
      1.0f,  1.0f
  };

  GLuint skyVAO, skyVBO;
  glGenVertexArrays(1, &skyVAO);
  glGenBuffers(1, &skyVBO);
  glBindVertexArray(skyVAO);
  glBindBuffer(GL_ARRAY_BUFFER, skyVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

  // the foregroud
  GLuint fireflyVAO;
  glGenVertexArrays(1, &fireflyVAO);
  glBindVertexArray(fireflyVAO);
  glBindBuffer(GL_ARRAY_BUFFER, skyVBO); // Reuse the same VBO
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

  // fake screen
  GLuint dubFbo, dubTex, dubDepth;
  initFBO(SCREEN_WIDTH, SCREEN_HEIGHT, &dubFbo, &dubTex, &dubDepth);
  float dupScreen[] = {
  0.0f,  0.0f,  0.0f, 0.0f,  // top-left of bottom-right quadrant
  0.0f, -1.0f,  0.0f, 1.0f,  // bottom-left of bottom-right quadrant
  1.0f, -1.0f,  1.0f, 1.0f,  // bottom-right of bottom-right quadrant

  0.0f,  0.0f,  0.0f, 0.0f,  // top-left
  1.0f, -1.0f,  1.0f, 1.0f,  // bottom-right
  1.0f,  0.0f,  1.0f, 0.0f   // top-right
};


  GLuint dupScreenVAO, dupScreenVBO;
  glGenVertexArrays(1, &dupScreenVAO);
  glGenBuffers(1, &dupScreenVBO);
  glBindVertexArray(dupScreenVAO);
  glBindBuffer(GL_ARRAY_BUFFER, dupScreenVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(dupScreen), dupScreen, GL_STATIC_DRAW);

  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
  glEnableVertexAttribArray(1);

  // the mini face screen 

  // fbo stuff 
  GLuint fbo, fboTex, fboDepth;
  initFBO(256,256, &fbo, &fboTex, &fboDepth);
  // set up screen 
  float quad[] = {
    // pos         // tex
    -1.0f,  1.0f, 0.0f, 1.0f,  // Top-left
    -1.0f,  0.75f, 0.0f, 0.0f, // Bottom-left
    -0.75f, 0.75f, 1.0f, 0.0f, // Bottom-right

    -1.0f,  1.0f, 0.0f, 1.0f,
    -0.75f, 0.75f, 1.0f, 0.0f,
    -0.75f, 1.0f, 1.0f, 1.0f   // Top-right
  };

  // the face objects 
  GLuint faceVAO, faceVBO;
  glGenVertexArrays(1, &faceVAO);
  glGenBuffers(1, &faceVBO);

  glBindVertexArray(faceVAO);
  glBindBuffer(GL_ARRAY_BUFFER, faceVBO);
  glBufferData(GL_ARRAY_BUFFER, MAX_LANDMARKS * sizeof(Point3D), NULL, GL_DYNAMIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Point3D), (void*)0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  // the mini screen 
  GLuint screenVAO, screenVBO;
  glGenVertexArrays(1, &screenVAO);
  glGenBuffers(1, &screenVBO);
  glBindVertexArray(screenVAO);
  glBindBuffer(GL_ARRAY_BUFFER, screenVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);

  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
  glEnableVertexAttribArray(1);

  // create world
  world game = createWorld(16, 16);
  
  // camera stuff
  cam = constructCamera(65.7f, 23.0f, 32.3f);
  vec3d front = getFrontVector(getYaw(cam), getPitch(cam));
  vec3d up = constructVec3d(0.0f, 1.0f, 0.0f);
  vec3d right = cross(front, up);
  normalise(right);

  initPerlin();

  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // Hide & capture mouse cursor
  if (glfwRawMouseMotionSupported()){
    // glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
  }
  glfwSetCursorPosCallback(window, mouseCallback);

  double targetFPS = 60.0;
  double frameTime = 1.0 / targetFPS;
  double lastFrameTime = glfwGetTime();

  vec3d lightPos = constructVec3d(100.0f, 100.0f, 100.0f);
  vec3d viewPos = constructVec3d(0.0f, 0.0f, 0.0f);

  vec3d velocity = constructVec3d(0.0f, 0.0f, 0.0f);

  glEnable(GL_FRAMEBUFFER_SRGB);

  // GAME loop

  static LoopCtx ctx;           /* lives for the whole run */
  ctx.lastFrameTime  = lastFrameTime;
  ctx.frameTime      = frameTime;
  ctx.window         = window;
  ctx.front          = front;
  ctx.right          = right;
  ctx.up             = up;
  ctx.velocity       = velocity;
  ctx.viewPos        = viewPos;
  ctx.lightPos       = lightPos;
  ctx.game           = game;
  ctx.view           = view;
  ctx.matProj        = matProj;
  ctx.fbo            = fbo;
  ctx.faceVBO        = faceVBO;
  ctx.faceVAO        = faceVAO;
  ctx.skyVAO         = skyVAO;
  ctx.dubFbo         = dubFbo;
  ctx.texture        = texture;
  ctx.dubTex         = dubTex;
  ctx.dudvTexture    = dudvTexture;
  ctx.normalTexture  = normalTexture;
  ctx.fireflyVAO     = fireflyVAO;
  ctx.fboTex         = fboTex;
  ctx.screenVAO      = screenVAO;
  ctx.program        = program;
  ctx.waterShader    = waterShader;
  ctx.skyShader      = skyShader;
  ctx.fireflyShader  = fireflyShader;
  ctx.faceShader     = faceShader;
  ctx.facyShader     = facyShader;

  #ifdef __EMSCRIPTEN__
    emscripten_set_main_loop_arg(main_loop, &ctx, 0, 1);
  #else
    while (!glfwWindowShouldClose(window)) {
      main_loop(&ctx);
    }
  #endif



  freeWorld(game);
  free(front);
  free(up);
  free(right);
  free(matProj);
  free(view);

  glfwDestroyWindow(window);
  glfwTerminate();
  return EXIT_SUCCESS;
}