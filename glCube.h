#ifndef GLCUBE_H_
#define GLCUBE_H_

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include "egl.h"

static const char *vertex_shader_source =
    "uniform mat4 modelviewMatrix;                                \n"
    "uniform mat4 modelviewprojectionMatrix;                      \n"
    "uniform mat3 normalMatrix;                                   \n"
    "                                                             \n"
    "attribute vec4 in_position;                                  \n"
    "attribute vec3 in_normal;                                    \n"
    "attribute vec4 in_color;                                     \n"
    "                                                             \n"
    "vec4 lightSource = vec4(2.0, 2.0, 20.0, 0.0);                \n"
    "                                                             \n"
    "varying vec4 vVaryingColor;                                  \n"
    "                                                             \n"
    "void main() {                                                \n"
    "  gl_Position = modelviewprojectionMatrix * in_position;     \n"
    "  vec3 vEyeNormal = normalMatrix * in_normal;                \n"
    "  vec4 vPosition4 = modelviewMatrix * in_position;           \n"
    "  vec3 vPosition3 = vPosition4.xyz / vPosition4.w;           \n"
    "  vec3 vLightDir = normalize(lightSource.xyz - vPosition3);  \n"
    "  float diff = max(0.0, dot(vEyeNormal, vLightDir));         \n"
    "  vVaryingColor = vec4(diff * in_color.rgb, 1.0);            \n"
    "}                                                            \n";

static const char *fragment_shader_source =
    "precision mediump float;         \n"
    "                                 \n"
    "varying vec4 vVaryingColor;      \n"
    "                                 \n"
    "void main() {                    \n"
    "  gl_FragColor = vVaryingColor;  \n"
    "}                                \n";

static const GLfloat vVertices[] = {
    // front
    -1.0f, -1.0f, +1.0f,
    +1.0f, -1.0f, +1.0f,
    -1.0f, +1.0f, +1.0f,
    +1.0f, +1.0f, +1.0f,
    // back
    -1.0f, -1.0f, -1.0f,
    +1.0f, -1.0f, -1.0f,
    -1.0f, +1.0f, -1.0f,
    +1.0f, +1.0f, -1.0f,
    // right
    +1.0f, -1.0f, -1.0f,
    +1.0f, +1.0f, -1.0f,
    +1.0f, -1.0f, +1.0f,
    +1.0f, +1.0f, +1.0f,
    // left
    -1.0f, -1.0f, -1.0f,
    -1.0f, +1.0f, -1.0f,
    -1.0f, -1.0f, +1.0f,
    -1.0f, +1.0f, +1.0f,
    //top
    -1.0f, +1.0f, -1.0f,
    +1.0f, +1.0f, -1.0f,
    -1.0f, +1.0f, +1.0f,
    +1.0f, +1.0f, +1.0f,
    // bottom
    -1.0f, -1.0f, -1.0f,
    +1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f, +1.0f,
    +1.0f, -1.0f, +1.0f,
};

static const GLfloat vColors[] = {
    // front
    0.5f, 0.6f, 0.5f,
    0.5f, 0.6f, 0.5f,
    0.5f, 0.6f, 0.5f,
    0.5f, 0.6f, 0.5f,
    // back
    0.5f, 0.6f, 0.5f,
    0.5f, 0.6f, 0.5f,
    0.5f, 0.6f, 0.5f,
    0.5f, 0.6f, 0.5f,
    // right
    0.5f, 0.6f, 0.5f,
    0.5f, 0.6f, 0.5f,
    0.5f, 0.5f, 0.5f,
    0.5f, 0.5f, 0.5f,
    // left
    0.5f, 0.5f, 0.5f,
    0.5f, 0.5f, 0.5f,
    0.5f, 0.5f, 0.5f,
    0.5f, 0.5f, 0.5f,
    // top
    0.5f, 0.5f, 0.5f,
    0.5f, 0.5f, 0.5f,
    0.5f, 0.5f, 0.5f,
    0.5f, 0.5f, 0.5f,
    // bottom
    0.5f, 0.5f, 0.5f,
    0.5f, 0.5f, 0.5f,
    0.5f, 0.5f, 0.5f,
    0.5f, 0.5f, 0.5f};

static const GLfloat vNormals[] = {
    // front
    +0.0f, +0.0f, +1.0f, // forward
    +0.0f, +0.0f, +1.0f, // forward
    +0.0f, +0.0f, +1.0f, // forward
    +0.0f, +0.0f, +1.0f, // forward
    // back
    +0.0f, +0.0f, -1.0f, // backward
    +0.0f, +0.0f, -1.0f, // backward
    +0.0f, +0.0f, -1.0f, // backward
    +0.0f, +0.0f, -1.0f, // backward
    // right
    +1.0f, +0.0f, +0.0f, // right
    +1.0f, +0.0f, +0.0f, // right
    +1.0f, +0.0f, +0.0f, // right
    +1.0f, +0.0f, +0.0f, // right
    // left
    -1.0f, +0.0f, +0.0f, // left
    -1.0f, +0.0f, +0.0f, // left
    -1.0f, +0.0f, +0.0f, // left
    -1.0f, +0.0f, +0.0f, // left
    // top
    +0.0f, +1.0f, +0.0f, // up
    +0.0f, +1.0f, +0.0f, // up
    +0.0f, +1.0f, +0.0f, // up
    +0.0f, +1.0f, +0.0f, // up
    // bottom
    +0.0f, -1.0f, +0.0f, // down
    +0.0f, -1.0f, +0.0f, // down
    +0.0f, -1.0f, +0.0f, // down
    +0.0f, -1.0f, +0.0f  // down
};

// Singleton class
class glCube {
private:

  glCube(){};
  glCube(const glCube &other){};
  ~glCube(){};

  GLint width_;
  GLint height_;
  GLfloat aspect_;

  GLuint modelviewmatrix_;
  GLuint modelviewprojectionmatrix_;
  GLuint normalmatrix_;

  GLuint positionoffset_;
  GLuint colorsoffset_;
  GLuint normalsoffset_;

  GLuint vertex_shader_;
  GLuint fragment_shader_;
  GLuint program_;

  GLuint vbo_;

  EGL *egl_;
  GBM *gbm_;

public:

  static glCube *instance();
  GLfloat aspect() { return aspect_; }

  int initGl(EGL *egl, GBM* gbm);
  int initCube();
  int linkProgram();
  int createProgram();
  void draw(unsigned i);
};

#endif // GLCUBE_H_