#include "egl.h"
#include <iostream>
#include <EGL/egl.h>
#include <EGL/eglext.h>

using namespace std;

EGL *EGL::instance() {
    static EGL instance_;
    return &instance_;
}

int EGL::init_egl(GBM *gbm) {
    gbm_ = gbm;

    const string client_exts(eglQueryString(NULL, EGL_EXTENSIONS));

/*
    difference with eglGetPlatformDisplayEXT
    eglGetPlatformDisplayEXT specifies native display platform explicitly
    eglGetDisplay guess the native display plaform which is error prone
    
    difference between eglGetPlatformDisplay and eglGetPlatformDisplayEXT
    "some drivers, like Mali have EGL_KHR_platform_gbm but not
    EGL_MESA_platform_gbm"
    https://phabricator.kde.org/D10346?vs=26663&id=26664

    "To obtain an EGLDisplay from an GBM device, call eglGetPlatformDisplay
    with <platform> set to EGL_PLATFORM_GBM_KHR."
    https://www.khronos.org/registry/EGL/extensions/KHR/EGL_KHR_platform_gbm.txt
    
    "To obtain an EGLDisplay from an GBM device, call
    eglGetPlatformDisplayEXT with <platform> set to EGL_PLATFORM_GBM_MESA."
    https://www.khronos.org/registry/EGL/extensions/MESA/EGL_MESA_platform_gbm.txt 
*/

   // 1. get EGLDisplay
   if (!client_exts.find("EGL_EXT_platform_base")) {
        eglGetPlatformDisplayEXT =
            reinterpret_cast<PFNEGLGETPLATFORMDISPLAYEXTPROC>(
                eglGetProcAddress("eglGetPlatformDisplayEXT"));
        if (eglGetPlatformDisplayEXT)
            display_ =
                eglGetPlatformDisplayEXT(EGL_PLATFORM_GBM_MESA, gbm->device_, NULL);
        else
            cout << "eglGetPlatformDisplayEXT() is not exist" << endl;
    }
    else {
        display_ = eglGetDisplay(gbm->device_);
    }

    if(display_ == EGL_NO_DISPLAY) {
        cout << "failed to eglGetDisplay()" << endl;
        exit(-1);
    }

    // 2. initialize the display
    if(!eglInitialize(display_, &major_, &minor_)) {
        cout << "failed to eglInitialize()" << endl;
        exit(-1);
    }

    // 3. bind API
    if (!eglBindAPI(EGL_OPENGL_ES_API)) {
        cout << "failed to eglBindAPI()" << endl;
        exit(-1);
    }

    // 5, 6, 7.
    chooseConfig();
    createContext();
    createSurface();

    // 8. attach the context to the surface
    eglMakeCurrent(display_, surface_, surface_, context_);

    // 4. summary
    const string display_exts(eglQueryString(display_, EGL_EXTENSIONS));

    cout << "[egl]" << endl;
    cout << "version            : " << eglQueryString(display_, EGL_VERSION) << endl;
    cout << "vendor             : " << eglQueryString(display_, EGL_VENDOR) << endl;
    cout << "client extensions  : " << client_exts << endl;
    cout << "display extensions : " << display_exts << endl;

    return 0;
}

int EGL::chooseConfig() {
    EGLConfig *configs;
    EGLint numConfig = 0;
    EGLint matched = 0;

    const EGLint attribs[] = {EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
                              EGL_RED_SIZE, 1,
                              EGL_GREEN_SIZE, 1,
                              EGL_BLUE_SIZE, 1,
                              EGL_ALPHA_SIZE, 0,
                              EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
                              EGL_SAMPLES, 1,
                              EGL_NONE};

    // ===========
    // get configs

    // 1. get the number of configs
    eglGetConfigs(display_, NULL, 0, &numConfig);
    if (!numConfig) {
        cout << "no available configs" << endl;
        return -1;
    }

    // 2. get a memory for configs
    configs = (EGLConfig *)malloc(numConfig * sizeof(EGLConfig));
    if (!configs) {
        cout << "failed to malloc for EGLConfig" << endl;
        exit(-1);
    }

    // 3. get configs
    if (!eglChooseConfig(display_, attribs, configs, numConfig, &matched) ||
        !matched) {
        cout << "failed to eglChooseConfig()" << endl;
        free(configs);
        exit(-1);
    }

    // ==================
    // get a valid config

    EGLConfig config_out;
    EGLint config_index = -1;

    if (!gbm_->format_) {
        config_ = configs[0];
        free(configs);
        return 0;
    }

    // We should choose one from the configs,
    // and the config should have gbm's format.
    for (int i = 0; i < numConfig; i++) {
        EGLint id;

        // get EGL_NATIVE_VISUAL_ID field from a config.
        if (!eglGetConfigAttrib(display_, configs[i], EGL_NATIVE_VISUAL_ID,
                                &id))
            continue;
        // rememeber the index of the config which has the valid EGL_NATIVE_VISUAL_ID
        else if (id == gbm_->format_) {
            config_index = i;
            break;
        }
    }

    // Get the config which has valid EGL_NATIVE_VISUAL_ID
    if (config_index != -1) {
        config_ = configs[config_index];
        free(configs);
    }
    // It not, we can't use EGL
    else{
        free(configs);
        exit(-1);
    }

    // summary
    cout << "[egl config]" << endl;
    printConfig(config_);
}

int EGL::createContext() {

/*
    difference static const and const
    Answer please?
    
    OpenGL is the OpenGL for embedded devices.
    OpenGL ES 2.0 is released Feb, 2007.
    OpenGL ES 3.0 is released Aug, 2012.

    OpenGL vs Vulkan vs DX12 vs Metal?
    Answer please?
*/

    static const EGLint context_attribs[] = {
        // In my case, a context with the OpenGL ES API < 3.1 is not available.
        EGL_CONTEXT_MAJOR_VERSION, 2,
        //EGL_CONTEXT_MINOR_VERSION, 2,
        EGL_NONE};

    if (!config_) {
        cout << "get a valid egl config first" << endl;
        return -1;
    }

/*
    We can create a conext with a valid EGL config.
    
    eglCreateContext creates a context for API binded by eglBindAPI() in advance.
    Set share_context as EGL_NO_CONTEXT to create a new context,
    If not, all shareable data in the context is shared with a newly created one.
    https://www.khronos.org/registry/EGL/sdk/docs/man/html/eglCreateContext.xhtml
*/
    context_ =
        eglCreateContext(display_, config_, EGL_NO_CONTEXT, context_attribs);
    if (!context_) {
        cout << "failed to eglCreateContext()" << endl;
        exit(-1);
    }
}

int EGL::createSurface() {

    const string client_exts(eglQueryString(NULL, EGL_EXTENSIONS));

    if (!client_exts.find("EGL_EXT_platform_base")) {
        eglCreatePlatformWindowSurfaceEXT =
            reinterpret_cast<PFNEGLCREATEPLATFORMWINDOWSURFACEEXTPROC>(
                eglGetProcAddress("eglCreatePlatformWindowSurfaceEXT"));

        if (eglCreatePlatformWindowSurfaceEXT) {
            surface_ = eglCreatePlatformWindowSurfaceEXT(display_,
                                                         config_,
                                                         gbm_->surface_,
                                                         NULL);
        }
    }

    if (surface_ == EGL_NO_SURFACE) {
        if (eglCreatePlatformWindowSurface) {
            surface_ = eglCreatePlatformWindowSurface(display_,
                                                      config_,
                                                      gbm_->surface(),
                                                      NULL);
        }
        else {
            surface_ = eglCreateWindowSurface(display_,
                                              config_, 
                                              (EGLNativeWindowType)(gbm_->surface()),
                                              NULL);
        }
    }

    if (surface_ == EGL_NO_SURFACE) {
        cout << "failed to eglCreateWindowSurface()" << endl;
        exit(-1);
    }

    return 0;
}

void EGL::printConfig(EGLConfig config) {
    EGLBoolean ret;

#define X(VAL)    \
    {             \
        VAL, #VAL \
    }
    struct
    {
        EGLint attribute;
        const char *name;
    } names[] = {
        X(EGL_BUFFER_SIZE),
        X(EGL_RED_SIZE),
        X(EGL_GREEN_SIZE),
        X(EGL_BLUE_SIZE),
        X(EGL_ALPHA_SIZE),
        X(EGL_CONFIG_CAVEAT),
        X(EGL_CONFIG_ID),
        X(EGL_DEPTH_SIZE),
        X(EGL_LEVEL),
        X(EGL_MAX_PBUFFER_WIDTH),
        X(EGL_MAX_PBUFFER_HEIGHT),
        X(EGL_MAX_PBUFFER_PIXELS),
        X(EGL_NATIVE_RENDERABLE),
        X(EGL_NATIVE_VISUAL_ID),
        X(EGL_NATIVE_VISUAL_TYPE),
        // X(EGL_PRESERVED_RESOURCES),
        X(EGL_SAMPLE_BUFFERS),
        X(EGL_SAMPLES),
        // X(EGL_STENCIL_BITS),
        X(EGL_SURFACE_TYPE),
        X(EGL_TRANSPARENT_TYPE),
        // X(EGL_TRANSPARENT_RED),
        // X(EGL_TRANSPARENT_GREEN),
        // X(EGL_TRANSPARENT_BLUE)
    };
#undef X

    for (int i = 0; i < sizeof(names) / sizeof(names[0]); i++) {
        EGLint value = -1;
        ret = eglGetConfigAttrib(display_, config, names[i].attribute, &value);
        if (ret) {
            printf(" %s: %d (0x%x)\n", names[i].name, value, value);
        }
    }
}

int EGL::printConfigs() {
    EGLint numConfig = 0;
    EGLint matched = 0;
    EGLConfig *configs;
    const EGLint attribs[] = { EGL_NONE };
    EGLint ret;
    
    
    ret = eglGetConfigs(display_, NULL, 0, &numConfig);
    if (!ret) {
        return false;
    }

    cout << "[egl configurations]" << endl;
    cout << "Number of EGL configuration : " << numConfig << endl;

    configs = (EGLConfig *)malloc(sizeof(EGLConfig) * numConfig);
    if (!configs) {
        cout << "Could not allocate memory for egl configs" << endl;
        exit(-1);
    }

    ret = eglChooseConfig(display_, attribs, configs, numConfig, &matched);
    if (!ret) {
        free(configs);
        exit(-1);
    }

    for (int i = 0; i < numConfig; i++) {
        cout << "egl config : " << i << endl;
        EGLConfig config = configs[i];
        printConfig(config);
        cout << endl;
    }

    free(configs);
    return 0;
}

void EGL::error() {
    EGLint err = eglGetError();

    if (err == EGL_SUCCESS) {
        return;
    }

#define X(VAL)      \
    if (err == VAL) \
        cout << "#VAL"
    X(EGL_NOT_INITIALIZED);
    X(EGL_BAD_ACCESS);
    X(EGL_BAD_ALLOC);
    X(EGL_BAD_ATTRIBUTE);
    X(EGL_BAD_CONTEXT);
    X(EGL_BAD_CONFIG);
    X(EGL_BAD_CURRENT_SURFACE);
    X(EGL_BAD_DISPLAY);
    X(EGL_BAD_SURFACE);
    X(EGL_BAD_MATCH);
    X(EGL_BAD_PARAMETER);
    X(EGL_BAD_NATIVE_PIXMAP);
    X(EGL_BAD_NATIVE_WINDOW);
    //X(EGL_BAD_CONTEXT_LOST);
#undef X

    exit(-1);
}