#include <iostream>
#include <fcntl.h>
#include <vector>
#include <map>
#include <string>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <gbm.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

using namespace std;

// Singleton class
class DRM
{
public:
    typedef struct drmPipe
    {
        drmModeConnector *connector_;
        vector<drmModeModeInfo *> modes_;
        drmModeEncoder *encoder_;
        drmModeCrtc *crtc_;
    };

    static DRM *instance()
    {
        static DRM instance_;
        return &instance_;
    }

    int print()
    {
        drmModeConnector *connector = NULL;
        drmModeEncoder *encoder = NULL;
        drmModeCrtc *crtc = NULL;

        cout << "[connectors]" << endl;
        for (int i = 0; i < resources_->count_connectors; i++)
        {
            connector = drmModeGetConnector(fd_, resources_->connectors[i]);
            cout << i << " | id : " << connector->connector_id
                 << " | type : " << connector->connector_type << endl;
            drmModeFreeConnector(connector);
        }

        cout << "[encoders]" << endl;
        for (int i = 0; i < resources_->count_encoders; i++)
        {
            encoder = drmModeGetEncoder(fd_, resources_->encoders[i]);
            cout << i << " | id : " << encoder->encoder_id
                 << " | type : " << encoder->encoder_type << endl;
            drmModeFreeEncoder(encoder);
        }

        cout << "[crtcs]" << endl;
        for (int i = 0; i < resources_->count_crtcs; i++)
        {
            crtc = drmModeGetCrtc(fd_, resources_->crtcs[i]);
            cout << i << " | id : " << crtc->crtc_id << endl;
            drmModeFreeCrtc(crtc);
        }
    }

    int print_pipes() {}

    int device_open()
    {
        fd_ = open("/dev/dri/card0", O_RDWR | O_NONBLOCK);
        if (fd_ == -1)
        {
            perror("drm open() failed");
            exit(1);
        }

        // Resources
        resources_ = drmModeGetResources(fd_);
        if (!resources_)
        {
            perror("drmModeGetResources() failed");
            exit(1);
        }
    }

    int initialize()
    {
        drmPipe *pipe = NULL;
        drmModeConnector *connector = NULL;
        drmModeModeInfo *mode = NULL;
        drmModeEncoder *encoder = NULL;
        drmModeCrtc *crtc = NULL;

        // Connectors such as VGA, HDMI, DisplayPort, etc
        for (int i = 0; i < resources_->count_connectors; i++)
        {
            connector = drmModeGetConnector(fd_, resources_->connectors[i]);
            if (connector->connection == DRM_MODE_CONNECTED)
            {
                pipe = new drmPipe;
                pipe->connector_ = connector;
                pipes_.push_back(pipe);
            }
            else
            {
                drmModeFreeConnector(connector);
            }
        }

        // Modes from connector
        for (vector<drmPipe *>::iterator it = pipes_.begin(); it != pipes_.end();
             ++it)
        {
            pipe = *it;
            connector = pipe->connector_;
            for (int j = 0; j < connector->count_modes; j++)
            {
                mode = &(connector->modes[j]);
                pipe->modes_.push_back(mode);
            }

            // Encoder for the connector from resource
            for (int j = 0; j < resources_->count_encoders; j++)
            {
                encoder = drmModeGetEncoder(fd_, resources_->encoders[j]);
                if (connector->encoder_id == encoder->encoder_id)
                {
                    pipe->encoder_ = encoder;

                    // Crtc for the encoder
                    for (int k = 0; k < resources_->count_crtcs; k++)
                    {
                        crtc = drmModeGetCrtc(fd_, resources_->crtcs[k]);
                        if (encoder->crtc_id == crtc->crtc_id)
                        {
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
    }

    drmPipe *available_pipe()
    {
        drmPipe *pipe;
        drmModeConnector *connector = NULL;
        drmModeModeInfo *mode = NULL;
        drmModeEncoder *encoder = NULL;
        drmModeCrtc *crtc = NULL;

        for (vector<drmPipe *>::iterator it = pipes_.begin(); it != pipes_.end();
             ++it)
        {
            pipe = *it;
            connector = pipe->connector_;
            encoder = pipe->encoder_;
            crtc = pipe->crtc_;
            if (pipe->crtc_)
            {
                cout << "[Available drm pipe]" << endl;
                cout << "connector : "
                     << " | id : " << connector->connector_id
                     << " | type : " << connector->connector_type << endl;
                cout << "encoder   : "
                     << " | id : " << encoder->encoder_id
                     << " | type : " << encoder->encoder_type << endl;
                cout << "crtc      : "
                     << " | id : " << crtc->crtc_id << endl;
                return pipe;
            }
            else
            {
                cout << "what" << endl;
            }
        }
    }

private:
    DRM(){};
    DRM(const DRM &other){};
    ~DRM(){};

    // Not declare static instance here for security concern.
    // ref : https://wendys.tistory.com/12
    // static DRM instance_;

    int fd_;
    drmModeRes *resources_;
    vector<drmPipe *> pipes_;
    vector<drmModeConnector *> connectors_;
    vector<drmModeEncoder *> encoders_;
    vector<drmModeCrtc *> crtcs_;

    friend class GBM;
};

// Singleton class
class GBM
{
private:
    GBM(){};
    GBM(const GBM &other){};
    ~GBM(){};

    DRM *drm_;
    struct gbm_device *device_;
    struct gbm_surface *surface_;
    uint32_t format_;
    uint16_t width_, height_;

    friend class EGL;

public:
    static GBM *instance()
    {
        static GBM instance_;
        return &instance_;
    }
    void init_gbm(DRM *drm)
    {
        drm_ = drm;
        DRM::drmPipe *pipe = drm->available_pipe();
        uint64_t modifier;

        device_ = gbm_create_device(drm_->fd_);
        surface_ = NULL;
        format_ = GBM_FORMAT_XRGB8888;
        width_ = pipe->modes_.front()->hdisplay;
        height_ = pipe->modes_.front()->vdisplay;

        // What is modifier?
        if (gbm_surface_create_with_modifiers)
        {
            surface_ = gbm_surface_create_with_modifiers(device_, width_, height_,
                                                         format_, &modifier, 1);
            cout << "[gbm]" << endl;
            cout << "width    : " << width_ << endl;
            cout << "height   : " << height_ << endl;
            cout << "modifier : " << modifier << endl;
        }
    }
};

class EGL
{
private:
    EGLint major_, minor_;
    GBM *gbm_;

    EGLDisplay display_;

    PFNEGLGETPLATFORMDISPLAYEXTPROC eglGetPlatformDisplayEXT;

public:
    static EGL *instance()
    {
        static EGL instance_;
        return &instance_;
    }

    int init_egl(GBM *gbm)
    {
        gbm_ = gbm;

        const string client_exts(eglQueryString(NULL, EGL_EXTENSIONS));

        // difference between eglGetPlatformDisplayEXT
        // eglGetPlatformDisplayEXT specifies native display platform explicitly
        // eglGetDisplay guess the native display plaform which is error prone
        if(!client_exts.find("EGL_EXT_platform_base")){
            eglGetPlatformDisplayEXT = reinterpret_cast<PFNEGLGETPLATFORMDISPLAYEXTPROC>(eglGetProcAddress("eglGetPlatformDisplay"));
            if(eglGetPlatformDisplayEXT)
                display_ = eglGetPlatformDisplayEXT(EGL_PLATFORM_GBM_MESA, gbm->device_, NULL);
            else
            {
                    cout << "Cannot retrive function pointer for eglGetPlatformDisplayEXT" << endl;
                    return 1;
            }
        }
        else
        {
            display_ = eglGetDisplay(gbm->device_);
        }

        // initialize the display
        eglInitialize(display_, &major_, &minor_);

        // egl extensions
        const string display_exts(eglQueryString(display_, EGL_EXTENSIONS));
        
        cout << "[egl]" << endl;
        cout << "version : " << eglQueryString(display_, EGL_VERSION) << endl;
        cout << "vendor  : " << eglQueryString(display_, EGL_VENDOR) << endl;
        cout << "client extensions  : " << client_exts << endl;
        cout << "display extensions : " << display_exts << endl;

        return 0;
    }

    int init_gl() {

        const EGLint attribs[] = {
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_RED_SIZE, 1,
            EGL_GREEN_SIZE, 1,
            EGL_BLUE_SIZE, 1,
            EGL_ALPHA_SIZE, 1,
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
            EGL_SAMPLES, 2,
            EGL_NONE
        };

        // bind API
        if(!eglBindAPI(EGL_OPENGL_ES_API)) {
            cout << "Failed to bind api" << endl;
        }

        EGLConfig *configs;
        EGLint count = 0;
        EGLint matched = 0;

        // get counts of configs
        eglGetConfigs(display_, NULL, 0, &count);
        if(!count) {
            cout << "no available configs" << endl;
            return 1;
        }
 
        // setting config
        configs = (EGLConfig*)malloc(count * sizeof(EGLConfig));
        if(!configs) {
            cout << "failed to malloc for EGLConfig" << endl;
            return 1;
        }

        // approval configs and get them 
        if(!eglChooseConfig(display_, attribs, configs, count, &matched) || !matched) {
            cout << "failed to 적용 configs" << endl;

        }
        else {
            

        }


            free(configs);
            return 1;




    }
};

int init_drm()
{
    DRM *drm = DRM::instance();
    GBM *gbm = GBM::instance();
    EGL *egl = EGL::instance();
    DRM::drmPipe *pipe;

    drm->device_open();
    drm->initialize();
    drm->print();

    gbm->init_gbm(drm);

    egl->init_egl(gbm);

    egl->init_gl();

    return 0;
}

int main()
{
    if (init_drm())
    {
        cout << "Failed init_drm()";
        exit(1);
    }
    /*
     if(init_gbm()) {
     cout << "Failed init_gbm()";
     exit(1);
     }
     if(init_egl()) {
     cout << "Failed init_egl()";
     exit(1);
     }
     */
    return 0;
}
