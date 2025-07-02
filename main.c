#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>

#include "glad/glad.h"
#include <GLFW/glfw3.h>

#include "main.h"
#include "utils/shader.h"
#include "utils/math.h"
// #include "world/chunk.h"
#include "world/world.h"
#include "world/camera.h"

#define STB_IMAGE_IMPLEMENTATION
#include "libs/stb_image.h"

#include <arpa/inet.h>

void framebuffer_size_callback(GLFWwindow* window, int width, int height){
  glViewport(0, 0, width, height);
}


void checkOpenGLError(const char* context) {
  GLenum err;
  while ((err = glGetError()) != GL_NO_ERROR) {
    fprintf(stderr, "OpenGL error in %s: 0x%X\n", context, err);
  }
}

void sleepSeconds(double seconds) {
  #ifdef _WIN32
      Sleep((DWORD)(seconds * 1000));
  #else
      struct timespec ts;
      ts.tv_sec = (time_t)seconds;
      ts.tv_nsec = (long)((seconds - ts.tv_sec) * 1e9);
      nanosleep(&ts, NULL);
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
  }

  float xoffset = (float)(xpos - lastX);
  float yoffset = (float)(lastY - ypos); // Y is inverted

  lastX = xpos;
  lastY = ypos;

  float sensitivity = 0.0125f;
  xoffset *= sensitivity;
  yoffset *= sensitivity;

  setYaw(cam, getYaw(cam) + xoffset);
  
  float newPitch = getPitch(cam) + yoffset;

  // Clamp pitch
  if (getPitch(cam) > 89.0f) newPitch = 89.0f;
  if (getPitch(cam) < -89.0f) newPitch = -89.0f;

  setPitch(cam, newPitch);

}

typedef struct { float x, y, z; } Point3D;
#define MAX_LANDMARKS 32

bool getFaceData(Point3D* outPoints, int* outCount, float* yaw, float* pitch) {
  static int sockfd = -1;
  static struct sockaddr_in addr;

  if (sockfd < 0) {
      sockfd = socket(AF_INET, SOCK_DGRAM, 0);
      addr.sin_family = AF_INET;
      addr.sin_port = htons(5005);
      addr.sin_addr.s_addr = INADDR_ANY;
      bind(sockfd, (struct sockaddr*)&addr, sizeof(addr));
  }

  char buffer[512];
  ssize_t len = recv(sockfd, buffer, sizeof(buffer), MSG_DONTWAIT);
  if (len <= 0) return false;

  int floatCount = len / sizeof(float);
  if (floatCount < 2) return false;

  float* floats = (float*)buffer;
  int landmarkCount = (floatCount - 2) / 3;

  if (landmarkCount > MAX_LANDMARKS) landmarkCount = MAX_LANDMARKS;

  for (int i = 0; i < landmarkCount; ++i) {
      outPoints[i] = (Point3D){ floats[i * 3 + 0], floats[i * 3 + 1], floats[i * 3 + 2] };
  }

  *yaw = floats[landmarkCount * 3];
  *pitch = floats[landmarkCount * 3 + 1];
  *outCount = landmarkCount;
  return true;
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

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    fprintf(stderr, "Failed to initialize GLAD\n");
    return EXIT_FAILURE;
  }

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
  float fNear        = 0.1f;
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

  // load the texture 
  int width = 64; int height = 16; int nrChannels = 3; 
  stbi_set_flip_vertically_on_load(1);
  unsigned char *data = stbi_load("texture/tile.png", &width, &height, &nrChannels, 0);
  assert(data != NULL);

  GLuint texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);

  checkOpenGLError("Loading textures");

  // set properties of the texture
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  GLenum format = (nrChannels == 3) ? GL_RGB : GL_RGBA;
  glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
  glGenerateMipmap(GL_TEXTURE_2D);
  stbi_image_free(data);

  checkOpenGLError("depth error stuff");


  width = 512; height = 512; nrChannels = 3; 
  stbi_set_flip_vertically_on_load(1);
  unsigned char *data2 = stbi_load("texture/waterDUDV.png", &width, &height, &nrChannels, 0);
  assert(data2 != NULL);

  GLuint dudvTexture;
  glGenTextures(1, &dudvTexture);
  glBindTexture(GL_TEXTURE_2D, dudvTexture);

  checkOpenGLError("Loading textures");

  // set properties of the texture
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  format = (nrChannels == 3) ? GL_RGB : GL_RGBA;
  glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data2);
  glGenerateMipmap(GL_TEXTURE_2D);
  stbi_image_free(data2);

  width = 512; height = 512; nrChannels = 3; 
  stbi_set_flip_vertically_on_load(1);
  unsigned char *data3 = stbi_load("texture/normalMap.png", &width, &height, &nrChannels, 0);
  assert(data3 != NULL);

  GLuint normalTexture;
  glGenTextures(1, &normalTexture);
  glBindTexture(GL_TEXTURE_2D, normalTexture);

  checkOpenGLError("Loading textures");

  // set properties of the texture
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  format = (nrChannels == 3) ? GL_RGB : GL_RGBA;
  glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data3);
  glGenerateMipmap(GL_TEXTURE_2D);
  stbi_image_free(data3);

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
  cam = constructCamera(65.7f, 13.0f, 32.3f);
  vec3d front = getFrontVector(getYaw(cam), getPitch(cam));
  vec3d up = constructVec3d(0.0f, 1.0f, 0.0f);
  vec3d right = cross(front, up);
  normalise(right);

  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // Hide & capture mouse cursor
  glfwSetCursorPosCallback(window, mouseCallback);

  double targetFPS = 60.0;
  double frameTime = 1.0 / targetFPS;
  double lastFrameTime = glfwGetTime();

  vec3d lightPos = constructVec3d(100.0f, 100.0f, 100.0f);
  vec3d viewPos = constructVec3d(0.0f, 0.0f, 0.0f);

  glEnable(GL_FRAMEBUFFER_SRGB);

  // GAME loop
  while (!glfwWindowShouldClose(window)) {
    double now = glfwGetTime();
    double deltaTime = now - lastFrameTime;

    if (deltaTime < frameTime) {
        // Sleep remaining time to avoid busy wait
        sleepSeconds(frameTime - deltaTime);
        continue;
    }

    lastFrameTime = now;

    glfwPollEvents();
    free(front);
    front = getFrontVector(getYaw(cam), getPitch(cam));
    free(right);
    right = cross(front, up);
    normalise(right);

    float cameraSpeed = 5.0f * (float)deltaTime;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        setPosition(cam, add(getPosition(cam), multiply(front, cameraSpeed)));
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        setPosition(cam, subtract(getPosition(cam), multiply(front, cameraSpeed)));
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        setPosition(cam, subtract(getPosition(cam), multiply(right, cameraSpeed)));
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        setPosition(cam, add(getPosition(cam), multiply(right, cameraSpeed)));
    }

    view = lookAt(getPosition(cam), add(getPosition(cam), front), up);

    // Rendering
    // Face screen 
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
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

      glBindBuffer(GL_ARRAY_BUFFER, faceVBO);
      glBufferData(GL_ARRAY_BUFFER, facePointCount * sizeof(Point3D), facePoints, GL_DYNAMIC_DRAW);
      setYaw(cam, yaw * 180.0f / 3.14159f);
      setPitch(cam, pitch * 180.0f / 3.14159f);

      glBindVertexArray(faceVAO);
      glUseProgram(facyShader);
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
    glUseProgram(skyShader); 
    glBindVertexArray(skyVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glDepthMask(GL_TRUE);

    // update view matrix
    viewPos->x = getPosition(cam)->x;
    viewPos->y = getPosition(cam)->y;
    viewPos->z = getPosition(cam)->z;

    float currTime = glfwGetTime();

    // fake screen 
    glBindFramebuffer(GL_FRAMEBUFFER, dubFbo);
    glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // render background to fake screen 
    // glDepthMask(GL_FALSE);
    // glUseProgram(skyShader); 
    // glBindVertexArray(skyVAO);
    // glDrawArrays(GL_TRIANGLES, 0, 6);
    // glDepthMask(GL_TRUE);
    // render world to fake screen 
    float distance = 2 * (getPosition(cam)->y - 3.0f);
    setYPosition(cam, getPosition(cam)->y - distance);
    setPitch(cam, -getPitch(cam));
    vec3d newUp = constructVec3d(0.0f, 1.0f, 0.0f);
    view = lookAt(getPosition(cam), add(getPosition(cam), getFrontVector(getYaw(cam), getPitch(cam))), newUp);
    renderWorld(game, getPosition(cam), program, waterShader, view, matProj, lightPos, viewPos, currTime, texture, true, dubTex, dudvTexture, normalTexture);
    setYPosition(cam, getPosition(cam)->y + distance);
    setPitch(cam, -getPitch(cam));
    // reset to normal frame buffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    view = lookAt(getPosition(cam), add(getPosition(cam), front), up);

    // render the world
    renderWorld(game, getPosition(cam), program, waterShader, view, matProj, lightPos, viewPos, currTime, texture, false, dubTex, dudvTexture, normalTexture);

    // render the ui 
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glUseProgram(fireflyShader); 
    glBindVertexArray(fireflyVAO);

    glUniform1f(glGetUniformLocation(fireflyShader, "time"), glfwGetTime());
    glUniformMatrix4fv(glGetUniformLocation(fireflyShader, "matProj"), 1, GL_TRUE, (float*)matProj->m);
    glUniformMatrix4fv(glGetUniformLocation(fireflyShader, "view"), 1, GL_TRUE, (float*)view->m);
    glUniform3f(glGetUniformLocation(fireflyShader, "cameraWorldPos"), getPosition(cam)->x,getPosition(cam)->y, getPosition(cam)->z);

    int fireflyCount = 400;
    int tiles = (2 * 5 + 1) * (2 * 5 + 1);
    glPointSize(2.0f);
    glDrawArrays(GL_POINTS, 0, fireflyCount * tiles);
    glPointSize(1.0f);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    glUseProgram(faceShader);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fboTex);
    glUniform1i(glGetUniformLocation(faceShader, "screenTex"), 0);

    glBindVertexArray(screenVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    checkOpenGLError("In loop");

    glfwSwapBuffers(window);
  }


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