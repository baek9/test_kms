#include "gbm.h"
#include <gbm.h>
#include <drm_fourcc.h>

using namespace std;

GBM *GBM::instance() {
    static GBM instance_;
    return &instance_;
}

void GBM::init_gbm(DRM *drm) {
    drm_ = drm;
    DRM::drmPipe *pipe = drm->available_pipe();
    uint64_t modifier = DRM_FORMAT_MOD_LINEAR;

    device_ = gbm_create_device(drm_->fd_);
    surface_ = NULL;
    format_ = GBM_FORMAT_XRGB8888;
    width_ = pipe->modes_.front()->hdisplay;
    height_ = pipe->modes_.front()->vdisplay;

    // What is modifier?
    // This causes EGL_BAD_ALLOC from the eglSwapBuffers()
    if (gbm_surface_create_with_modifiers) {
        cout << "FUNCTION : gbm_surface_create_with_modifiers()" << endl;
        surface_ = gbm_surface_create_with_modifiers(device_,
                                                     width_, height_,
                                                     format_, &modifier, 1);
    }

    if (!surface_) {
        cout << "FUNCTION : gbm_surface_create()" << endl;
        surface_ = gbm_surface_create(device_,
                                      width_, height_,
                                      format_, GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
    }

    if (!surface_) {
        cout << "NO GBM SURFACE!" << endl;
        exit(0);
    }

    cout << "[gbm]" << endl;
    cout << "width    : " << width_ << endl;
    cout << "height   : " << height_ << endl;
    cout << "modifier : " << modifier << endl;
}