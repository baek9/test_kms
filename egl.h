#ifndef EGL_H_
#define EGL_H_

#include "gbm.h"
#include <EGL/egl.h>
#include <EGL/eglext.h>

class EGL {
private:

  GBM *gbm_;

  EGLint major_;
  EGLint minor_;

  EGLDisplay display_;
  EGLConfig config_;
  EGLContext context_;
  EGLSurface surface_;

  PFNEGLGETPLATFORMDISPLAYEXTPROC eglGetPlatformDisplayEXT;
  PFNEGLCREATEPLATFORMWINDOWSURFACEEXTPROC eglCreatePlatformWindowSurfaceEXT;

public:

  static EGL *instance();

  GBM *gbm() { return gbm_; }
  EGLDisplay display() { return display_; }
  EGLSurface surface() { return surface_; }

  int init_egl(GBM *gbm);
  int chooseConfig();
  int createContext();
  int createSurface();
  void printConfig(EGLConfig config);
  int printConfigs();
  void error();
};

#endif // EGL_H_