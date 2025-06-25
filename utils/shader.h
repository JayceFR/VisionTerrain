#ifndef SHADER_H
#define SHADER_H

#include "../include/glad/glad.h"
#include "./math.h"

// Usage - char *data = readFile(loc);
// Reads the entire file and returns it as a string. 
extern char* readFile(char*);


// Usage - Gluint program = compileShader("shader/vert", "shader/frag");
// Constructs the vertex and fragment shader from their respective sources 
// And compiles and links them into one program and returns it. 
// Use glUseProgram(program) to run the shader code in the main loop.
extern GLuint compileShader(char *vertexShaderLoc, char *fragmentShaderLoc);

extern void useShader(GLuint program, 
              mat4x4 model, mat4x4 view, mat4x4 proj, 
               vec3d lightPos, vec3d viewPos, 
               float time, GLuint texture, 
               GLuint reflectedTex, GLuint dudvTex);

#endif