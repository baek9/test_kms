#include <iostream>
#include <cstring>
#include <inttypes.h>
#include <gbm.h>
//#include <GLES3/gl3.h>
//#include <GLES3/gl3ext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
//#include <GLFW/glfw3.h>

#include "esUtil.h"
#include "drm.h"
#include "gbm.h"
#include "egl.h"
#include "glCube.h"

using namespace std;

static void page_flip_handler(int fd, unsigned int frame,
                              unsigned int sec, unsigned int usec, void *data)
{
  int *waiting_for_flip = (int *)data;
  *waiting_for_flip = 0;
}

struct drm_fb
{
  struct gbm_bo *bo;
  uint32_t fb_id;
};

static void drm_fb_destroy_callback(struct gbm_bo *bo, void *data)
{
  int drm_fd = gbm_device_get_fd(gbm_bo_get_device(bo));
  struct drm_fb *fb = (drm_fb *)data;

  if (fb->fb_id)
    drmModeRmFB(drm_fd, fb->fb_id);

  free(fb);
}

DRM *drm = DRM::instance();
GBM *gbm = GBM::instance();
EGL *egl = EGL::instance();
glCube *gl = glCube::instance();

int init()
{
  DRM::drmPipe *pipe;

  drm->device_open();
  drm->initialize();
  drm->print();

  gbm->init_gbm(drm);
  egl->init_egl(gbm);
  gl->initGl(egl, gbm);

  cout << "ALL INITIALIZATION ENDED!!!" << endl;

  // =================

  struct gbm_bo *bo = NULL;

/*
  drmEventContext evctx = {
    version : 2,
    page_flip_handler : page_flip_handler,
  };
*/

  drmEventContext evctx;
  memset(&evctx, 0, sizeof(drmEventContext));
  evctx.version = 2;
  evctx.page_flip_handler = page_flip_handler;

  gl->initCube();

  // drm legacy run

  cout << "1" << endl;

  cout << "egl->display is exist" << eglQueryString(egl->display(), EGL_VERSION) << endl;

  if (egl->surface() != EGL_NO_SURFACE)
  {
    cout << "egl->surface is exist" << endl;
  }

  int a = eglSwapBuffers(egl->display(), egl->surface());

  egl->error();

  bo = gbm_surface_lock_front_buffer(gbm->surface());

  if (!bo)
  {
    cout << "what?" << endl;
  }

  cout << "2" << endl;

  // =======================
  // get information from bo

  uint32_t width, height, format;

  uint32_t strides[4] = {0},
           handles[4] = {0},
           offsets[4] = {0};

  uint32_t flags = 0;

  int ret = -1;
  struct drm_fb *fb;

  //fb = drm_fb_get_from_bo
  int drm_fd = gbm_device_get_fd(gbm_bo_get_device(bo));
  fb = (drm_fb *)gbm_bo_get_user_data(bo);
  if (fb)
  {
    //return fb;
    goto fb_exist;
  }

  //  uint32_t width, height, format;

  fb = (drm_fb *)calloc(1, sizeof *fb);
  fb->bo = bo;

  width = gbm_bo_get_width(bo);
  height = gbm_bo_get_height(bo);
  format = gbm_bo_get_format(bo);

  if (gbm_bo_get_modifier &&
      gbm_bo_get_plane_count &&
      gbm_bo_get_stride_for_plane &&
      gbm_bo_get_offset)
  {
    uint64_t modifiers[4] = {0};
    modifiers[0] = gbm_bo_get_modifier(bo);

    const int num_planes = gbm_bo_get_plane_count(bo);
    for (int i = 0; i < num_planes; i++)
    {
      strides[i] = gbm_bo_get_stride_for_plane(bo, i);
      handles[i] = gbm_bo_get_handle(bo).u32;
      offsets[i] = gbm_bo_get_offset(bo, i);
      modifiers[i] = modifiers[0];
    }

    if (modifiers[0])
    {
      flags = DRM_MODE_FB_MODIFIERS;
      printf("Using modifier %" PRIx64 "\n", modifiers[0]);
    }

    ret = drmModeAddFB2WithModifiers(drm_fd, width, height,
                                     format, handles, strides, offsets,
                                     modifiers, &fb->fb_id, flags);
  }

  if (!ret)
  {
    if (flags)
    {
      perror("Modifiers failed!\n");

      uint32_t temp1[4] = {gbm_bo_get_handle(bo).u32, 0, 0, 0};
      uint32_t temp2[4] = {gbm_bo_get_stride(bo), 0, 0, 0};
      memcpy(handles, temp1, 16);
      memcpy(strides, temp2, 16);
      memcpy(offsets, 0, 16);
      ret = drmModeAddFB2(drm_fd, width, height,
                          format, handles, strides, offsets,
                          &fb->fb_id, 0);
    }

    if (ret)
    {
      printf("failed to create fb: %s\n", strerror(errno));
      free(fb);
      //return NULL;
      return 1;
    }

    gbm_bo_set_user_data(bo, fb, drm_fb_destroy_callback);

    // here we get fb
  fb_exist:

    // 목적은 fb를 얻는 것.
    if (!fb)
    {
      perror("Failed to get a new framebuffer bo");
      return 1;
    }

    // fb를 얻는 것은 drmModeSetCrtc를 부르기 위함.
    DRM::drmPipe *pipe = drm->pipe();
    ret = drmModeSetCrtc(drm->fd(), pipe->crtc_->crtc_id, fb->fb_id, 0, 0,
                         &(pipe->connector_->connector_id), 1, pipe->modes_.front());

    if (ret)
    {
      perror("failed to drmModeSetCrtc()");
      return 1;
    }

    int i = 0;
    fd_set fds;

    while (1)
    {

      struct gbm_bo *next_bo;
      int waiting_for_flip = 1;

      gl->draw(i++);

      eglSwapBuffers(egl->display(), egl->surface());
      egl->error();
      next_bo = gbm_surface_lock_front_buffer(gbm->surface());

      // fb = drm_fb_get_from_bo, but, already have fb use it
      if (!fb)
      {
        return -1;
      }

      if (drmModePageFlip(drm->fd(), pipe->crtc_->crtc_id, fb->fb_id,
                          DRM_MODE_PAGE_FLIP_EVENT, &waiting_for_flip))
      {
        cout << "failed to drmModePageFlip()" << endl;
      }

      while (waiting_for_flip)
      {
        FD_ZERO(&fds);
        FD_SET(0, &fds);
        FD_SET(drm->fd(), &fds);

        ret = select(drm->fd() + 1, &fds, NULL, NULL, NULL);

        if (ret < 0)
        {
          printf("select err: %s\n", strerror(errno));
          return ret;
        }
        else if (ret == 0)
        {
          printf("select timeout!\n");
          return -1;
        }
        else if (FD_ISSET(0, &fds))
        {
          printf("user interrupted!\n");
          return 0;
        }

        drmHandleEvent(drm->fd(), &evctx);
      }

      gbm_surface_release_buffer(gbm->surface(), bo);
      bo = next_bo;
    }

    return 0;
  }

  return 0;
}

/*
static void error_callback(int error, const char* description) {
  perror(description);
}

static void key_callback(GLFWwindow* window, int key, int action, int mods) {
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
    glfwSetWindowShouldClose(window, GL_TRUE);
  }
}
*/

int main()
{

  if (init())
  {
    perror("Failed init()");
    exit(1);
  }

  /* 
  GLFWwindow* window;

  glfwSetErrorCallback(error_callback);
  
  if(!glfwInit()) {
    perror("1");
    exit(EXIT_FAILURE);
  }

  window = glfwCreateWindow(640, 480, "Simple example", NULL, NULL);
  if (!window) {
    printf("2\n");
    glfwTerminate();
    exit(EXIT_FAILURE);
  }

  glfwMakeContextCurrent(window);

  glfwSetKeyCallback(window, (GLFWkeyfun)key_callback);

  while (!glfwWindowShouldClose(window)) {
    float ratio;
    int width, height;

    glfwGetFramebufferSize(window, &width, &height);
    ratio = width / (float) height;

    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-ratio, ratio, -1.f, 1.f, 1.f, -1.f);
    glMatrixMode(GL_MODELVIEW);

    glLoadIdentity();
    glRotatef((float) glfwGetTime() * 50.f, 0.f, 0.f, 1.f);

    glBegin(GL_TRIANGLES);
    glColor3f(1.f, 0.f, 0.f);
    glVertex3f(-0.6f, -0.4f, 0.f);
    glColor3f(0.f, 1.f, 0.f);
    glVertex3f(0.6f, -0.4f, 0.f);
    glColor3f(0.f, 0.f, 1.f);
    glVertex3f(0.f, 0.6f, 0.f);
    glEnd();

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwDestroyWindow(window);
  
  glfwTerminate();
  exit(EXIT_SUCCESS);
  */

  return 0;
}
