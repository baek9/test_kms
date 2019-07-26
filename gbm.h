#ifndef GBM_H_
#define GBM_H_

#include <iostream>
#include "drm.h"
#include <gbm.h>

// Singleton class
class GBM {
private:

  GBM(){};
  GBM(const GBM &other){};
  ~GBM(){};

  DRM *drm_;
  struct gbm_device *device_;
  struct gbm_surface *surface_;
  uint32_t format_;
  uint16_t width_;
  uint16_t height_;

  friend class EGL;
  friend class GL;

public:

  static GBM *instance();

  struct gbm_surface *surface() { 
      if (!surface_) 
        exit(1);
    return surface_;
  }

  uint16_t width() { return width_; }
  uint16_t height() { return height_; }

  void init_gbm(DRM *drm);
 };

#endif // GBM_H_