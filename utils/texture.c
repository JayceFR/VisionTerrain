#include <stdio.h>
#include <assert.h>
#define STB_IMAGE_IMPLEMENTATION
#include "../libs/stb_image.h"

#include "../glad/include/glad/glad.h"
#include <GLFW/glfw3.h>
#include "texture.h"

static void checkOpenGLError(const char* context) {
  GLenum err;
  while ((err = glGetError()) != GL_NO_ERROR) {
    fprintf(stderr, "OpenGL error in %s: 0x%X\n", context, err);
  }
}

GLuint loadTexture(char *fileName, int width, int height, int nrChannels){
  stbi_set_flip_vertically_on_load(1);
  unsigned char* data = stbi_load(fileName, &width, &height, &nrChannels, 0);
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

  return texture;
}

