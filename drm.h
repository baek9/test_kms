#ifndef DRM_H_
#define DRM_H_

#include <iostream>
#include <vector>
#include <xf86drm.h>
#include <xf86drmMode.h>

using namespace std;

// Singleton class
class DRM
{
public:

    typedef struct drmPipe {
        drmModeConnector *connector_;
        vector<drmModeModeInfo *> modes_;
        drmModeEncoder *encoder_;
        drmModeCrtc *crtc_;
    };

    static DRM *instance();
    int print();
    int device_open();
    int initialize();
    drmPipe *pipe();
    drmPipe *available_pipe();

    int fd() { return fd_; }

private:

    DRM(){};
    DRM(const DRM &other){};
    ~DRM(){};

    // Not declare static instance here for security concern.
    // ref : https://wendys.tistory.com/12
    // static DRM instance_;

    int fd_;
    drmModeRes *resources_;
    drmPipe *pipe_;
    vector<drmPipe *> pipes_;
    vector<drmModeConnector *> connectors_;
    vector<drmModeEncoder *> encoders_;
    vector<drmModeCrtc *> crtcs_;

    friend class GBM;
};

#endif // DRM_H_