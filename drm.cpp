#include "drm.h"

#include <fcntl.h>

DRM *DRM::instance() {
    static DRM instance_;
    return &instance_;
}

int DRM::print() {
    drmModeConnector *connector = NULL;
    drmModeEncoder *encoder = NULL;
    drmModeCrtc *crtc = NULL;

    cout << "[drm]" << endl;

    for (int i = 0; i < resources_->count_connectors; i++) {
        connector = drmModeGetConnector(fd_, resources_->connectors[i]);
        cout << "connector " << i
             << " | id : " << connector->connector_id
             << " | type : " << connector->connector_type << endl;
        drmModeFreeConnector(connector);
    }

    for (int i = 0; i < resources_->count_encoders; i++) {
        encoder = drmModeGetEncoder(fd_, resources_->encoders[i]);
        cout << "encoder " << i
             << " | id : " << encoder->encoder_id
             << " | type : " << encoder->encoder_type << endl;
        drmModeFreeEncoder(encoder);
    }

    cout << "[crtcs]" << endl;
    for (int i = 0; i < resources_->count_crtcs; i++) {
        crtc = drmModeGetCrtc(fd_, resources_->crtcs[i]);
        cout << "crtc " << i
             << " | id : " << crtc->crtc_id << endl;
        drmModeFreeCrtc(crtc);
    }
}

int DRM::device_open() {
    fd_ = open("/dev/dri/card0", O_RDWR | O_NONBLOCK);
    if (fd_ == -1) {
        perror("drm open() failed");
        exit(1);
    }

    // Resources
    resources_ = drmModeGetResources(fd_);
    if (!resources_) {
        perror("drmModeGetResources() failed");
        exit(1);
    }
}

int DRM::initialize() {
    drmPipe *pipe = NULL;
    drmModeConnector *connector = NULL;
    drmModeModeInfo *mode = NULL;
    drmModeEncoder *encoder = NULL;
    drmModeCrtc *crtc = NULL;

    // Connectors such as VGA, HDMI, DisplayPort, etc
    for (int i = 0; i < resources_->count_connectors; i++) {
        connector = drmModeGetConnector(fd_, resources_->connectors[i]);
        if (connector->connection == DRM_MODE_CONNECTED) {
            pipe = new drmPipe;
            pipe->connector_ = connector;
            pipes_.push_back(pipe);
        }
        else {
            drmModeFreeConnector(connector);
        }
    }

    // Modes from connector
    for (vector<drmPipe *>::iterator it = pipes_.begin(); it != pipes_.end();
         ++it) {
        pipe = *it;
        connector = pipe->connector_;
        for (int j = 0; j < connector->count_modes; j++) {
            mode = &(connector->modes[j]);
            pipe->modes_.push_back(mode);
        }

        // Encoder for the connector from resource
        for (int j = 0; j < resources_->count_encoders; j++) {
            encoder = drmModeGetEncoder(fd_, resources_->encoders[j]);
            if (connector->encoder_id == encoder->encoder_id) {
                pipe->encoder_ = encoder;

                // Crtc for the encoder
                for (int k = 0; k < resources_->count_crtcs; k++) {
                    crtc = drmModeGetCrtc(fd_, resources_->crtcs[k]);
                    if (encoder->crtc_id == crtc->crtc_id) {
                        pipe->crtc_ = crtc;
                        break;
                    }
                    drmModeFreeCrtc(crtc);
                }
                continue;
            }
            drmModeFreeEncoder(encoder);
            break;
        }
    }

    pipe_ = pipes_.front();
}

DRM::drmPipe *DRM::pipe() {
    if (!pipe_)
        return NULL;
    return pipe_;
}

DRM::drmPipe *DRM::available_pipe() {
    drmPipe *pipe;
    drmModeConnector *connector = NULL;
    drmModeModeInfo *mode = NULL;
    drmModeEncoder *encoder = NULL;
    drmModeCrtc *crtc = NULL;

    for (vector<drmPipe *>::iterator it = pipes_.begin(); it != pipes_.end();
         ++it) {
        pipe = *it;
        connector = pipe->connector_;
        encoder = pipe->encoder_;
        crtc = pipe->crtc_;
        if (pipe->crtc_) {
            cout << "[Available drm pipe]" << endl;
            cout << "connector : "
                 << " | id : " << connector->connector_id
                 << " | type : " << connector->connector_type << endl;
            cout << "encoder   : "
                 << " | id : " << encoder->encoder_id
                 << " | type : " << encoder->encoder_type << endl;
            cout << "crtc      : "
                 << " | id : " << crtc->crtc_id << endl;

            for(vector<drmModeModeInfo *>::iterator it = pipe->modes_.begin(); it != pipe->modes_.end(); ++it) {
                drmModeModeInfo *mode = *it;
                cout << "mode      : "
                    << " | name : " << mode->name 
                    << " | width : " << mode->hdisplay
                    << " | height : " << mode->vdisplay << endl;
            }

            return pipe;
        }
        else {
            perror("No available drm pipe");
        }
    }
}
