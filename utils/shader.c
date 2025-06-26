#include <stdio.h>
#include <stdlib.h>

#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include "math.h"

char* readFile(char* fileName){
  FILE *file = fopen(fileName, "r");
  
  fseek(file, 0, SEEK_END);
  long len = ftell(file);
  rewind(file);

  char *content = malloc(len+1);
  fread(content, 1, len, file);
  content[len] = '\0';

  fclose(file);

  return content;
}


// Usage - Gluint program = compileShader("shader/vert", "shader/frag");
// Constructs the vertex and fragment shader from their respective sources 
// And compiles and links them into one program and returns it. 
// Use glUseProgram(program) to run the shader code in the main loop.
GLuint compileShader(char *vertexShaderLoc, char *fragmentShaderLoc){
  const char *vertexSrc = readFile(vertexShaderLoc);
  const char *fragementSrc = readFile(fragmentShaderLoc);

  // vertex shader
  GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertexShader, 1, &vertexSrc, NULL);
  glCompileShader(vertexShader);

  // fragment shader
  GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragmentShader, 1, &fragementSrc, NULL);
  glCompileShader(fragmentShader);

  GLuint shaderProgram = glCreateProgram();
  glAttachShader(shaderProgram, vertexShader);
  glAttachShader(shaderProgram, fragmentShader);

  glLinkProgram(shaderProgram);

  // free stuff
  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);

  free((void*)vertexSrc);
  free((void*)fragementSrc);

  return shaderProgram;
}

void useShader(GLuint program, 
               mat4x4 model, mat4x4 view, mat4x4 proj, 
               vec3d lightPos, vec3d viewPos, 
               float time, GLuint texture, 
               GLuint reflectedTex, GLuint dudvTex, GLuint normalTex) 
{
    glUseProgram(program);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);

    GLuint modelLoc = glGetUniformLocation(program, "model");
    if (modelLoc != -1){
      glUniformMatrix4fv(modelLoc, 1, GL_TRUE, (float*)model->m);
    }

    GLuint viewLoc = glGetUniformLocation(program, "view");
    if (viewLoc != -1){
      glUniformMatrix4fv(viewLoc, 1, GL_TRUE, (float*)view->m);
    }

    GLuint projLoc = glGetUniformLocation(program, "projection");
    if (projLoc != -1){
      glUniformMatrix4fv(projLoc, 1, GL_TRUE, (float*)proj->m);
    }

    GLuint lightPosLoc = glGetUniformLocation(program, "lightPos");
    if (lightPosLoc != -1){
      glUniform3f(lightPosLoc, lightPos->x, lightPos->y, lightPos->z);
    }

    GLuint viewPosLoc = glGetUniformLocation(program, "viewPos");
    if (viewPosLoc != -1){
      glUniform3f(viewPosLoc, viewPos->x, viewPos->y, viewPos->z);
    }

    GLuint lightColorLoc = glGetUniformLocation(program, "lightColor");
    if (lightColorLoc != -1){
        glUniform3f(lightColorLoc, 1.0f, 0.68f, 0.79f);
    }

    GLuint timeLoc = glGetUniformLocation(program, "time");
    if (timeLoc != -1){
      glUniform1f(timeLoc, time);
    }

    GLuint reflectedLoc = glGetUniformLocation(program, "reflectedTexture");
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, reflectedTex);
    glUniform1i(reflectedLoc, 1);

    GLuint dudvLoc = glGetUniformLocation(program, "dudvMap");
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, dudvTex);
    glUniform1i(dudvLoc, 2);

    GLuint normalLoc = glGetUniformLocation(program, "normalMap");
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, normalTex);
    glUniform1i(normalLoc, 3);
}
