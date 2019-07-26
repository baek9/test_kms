#include "glCube.h"
#include "egl.h"
#include "esUtil.h"

// Singleton class
glCube *glCube::instance() {
    static glCube instance_;
    return &instance_;
}

// should move to EGL
int glCube::initGl(EGL *egl, GBM* gbm) {
    egl_ = egl;
    gbm_ = gbm;

    width_ = gbm->width();
    height_ = gbm->height();
    aspect_ = (GLfloat)width_ / (GLfloat)height_;

    // summary
    const string gl_exts((char *)glGetString(GL_EXTENSIONS));

    cout << "[gl]" << endl;
    cout << "version      : " << glGetString(GL_VERSION) << endl;
    cout << "glsl version : " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
    cout << "vendor       : " << glGetString(GL_VENDOR) << endl;
    cout << "renderer     : " << glGetString(GL_RENDERER) << endl;
    cout << "extensions   : " << gl_exts << endl;

    return 0;
}


int glCube::initCube() {
    
    int ret;
    
    createProgram();

    glBindAttribLocation(program_, 0, "in_position");
    glBindAttribLocation(program_, 1, "in_normal");
    glBindAttribLocation(program_, 2, "in_color");

    linkProgram();

    glUseProgram(program_);

    modelviewmatrix_ = glGetUniformLocation(program_, "modelviewMatrix");
    modelviewprojectionmatrix_ = glGetUniformLocation(program_, "modelviewprojectionMatrix");
    normalmatrix_ = glGetUniformLocation(program_, "normalMatrix");

    glViewport(0, 0, width_, height_);
    glEnable(GL_CULL_FACE);

    positionoffset_ = 0;
    colorsoffset_ = sizeof(vVertices);
    normalsoffset_ = sizeof(vVertices) + sizeof(vColors);

    glGenBuffers(1, &vbo_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER,
                 sizeof(vVertices) + sizeof(vColors) + sizeof(vNormals),
                 0, GL_STATIC_DRAW);

    glBufferSubData(GL_ARRAY_BUFFER, positionoffset_, sizeof(vVertices), &vVertices[0]);
    glBufferSubData(GL_ARRAY_BUFFER, colorsoffset_, sizeof(vColors), &vColors[0]);
    glBufferSubData(GL_ARRAY_BUFFER, normalsoffset_, sizeof(vNormals), &vNormals[0]);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (const GLvoid *)(intptr_t)positionoffset_);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (const GLvoid *)(intptr_t)normalsoffset_);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (const GLvoid *)(intptr_t)colorsoffset_);
    glEnableVertexAttribArray(2);
}

int glCube::linkProgram() {

    GLint ret;

    glLinkProgram(program_);
    glGetProgramiv(program_, GL_LINK_STATUS, &ret);

    if (!ret) {
        cout << "failed to link program" << endl;
        exit(-1);
    }
}

int glCube::createProgram() {

    GLint ret;

    // compile vertex shader
    vertex_shader_ = glCreateShader(GL_VERTEX_SHADER);

    glShaderSource(vertex_shader_, 1, &vertex_shader_source, NULL);
    glCompileShader(vertex_shader_);

    glGetShaderiv(vertex_shader_, GL_COMPILE_STATUS, &ret);

    if (!ret) {
        char *log;

        cout << "failed to compile vertex shader" << endl;
        glGetShaderiv(vertex_shader_, GL_INFO_LOG_LENGTH, &ret);
        if (ret > 1) {
            log = (char*)malloc(ret);
            glGetShaderInfoLog(vertex_shader_, ret, NULL, log);
            cout << log << endl;
            free(log);
        }

        exit(-1);
    }

    // compile fragment shader
    fragment_shader_ = glCreateShader(GL_FRAGMENT_SHADER);

    glShaderSource(fragment_shader_, 1, &fragment_shader_source, NULL);
    glCompileShader(fragment_shader_);

    if (!ret) {
        char *log;

        cout << "failed to compile fragment shader" << endl;
        glGetShaderiv(fragment_shader_, GL_INFO_LOG_LENGTH, &ret);
        if (ret > 1) {
            log = (char*)malloc(ret);
            glGetShaderInfoLog(fragment_shader_, ret, NULL, log);
            cout << log << endl;
            free(log);
        }

        exit(-1);
    }

    // create gl program
    program_ = glCreateProgram();

    // attach shaders to the program
    glAttachShader(program_, vertex_shader_);
    glAttachShader(program_, fragment_shader_);

   }

void glCube::draw(unsigned i)
{
    ESMatrix modelview;

    glClearColor(0.5, 0.5, 0.5, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    esMatrixLoadIdentity(&modelview);
    esTranslate(&modelview, 0.0f, 0.0f, -8.0f);
    esRotate(&modelview, 45.0f + (0.25f * i), 1.0f, 0.0f, 0.0f);
    esRotate(&modelview, 45.0f + (0.5f * i), 0.0f, 1.0f, 0.0f);
    esRotate(&modelview, 10.0f + (0.15f * i), 0.0f, 0.0f, 1.0f);

    ESMatrix projection;
    esMatrixLoadIdentity(&projection);
    esFrustum(&projection,
              -2.8f, +2.8f,
              -2.8f * aspect_, +2.8f * aspect_,
              6.0f, 10.0f);

    ESMatrix modelviewprojection;
    esMatrixLoadIdentity(&modelviewprojection);
    esMatrixMultiply(&modelviewprojection, &modelview, &projection);

    float normal[9];
    normal[0] = modelview.m[0][0];
    normal[1] = modelview.m[0][1];
    normal[2] = modelview.m[0][2];
    normal[3] = modelview.m[1][0];
    normal[4] = modelview.m[1][1];
    normal[5] = modelview.m[1][2];
    normal[6] = modelview.m[2][0];
    normal[7] = modelview.m[2][1];
    normal[8] = modelview.m[2][2];

    glUniformMatrix4fv(modelviewmatrix_, 1, GL_FALSE, &modelview.m[0][0]);
    glUniformMatrix4fv(modelviewprojectionmatrix_, 1, GL_FALSE, &modelviewprojection.m[0][0]);
    glUniformMatrix3fv(normalmatrix_, 1, GL_FALSE, normal);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDrawArrays(GL_TRIANGLE_STRIP, 4, 4);
    glDrawArrays(GL_TRIANGLE_STRIP, 8, 4);
    glDrawArrays(GL_TRIANGLE_STRIP, 12, 4);
    glDrawArrays(GL_TRIANGLE_STRIP, 16, 4);
    glDrawArrays(GL_TRIANGLE_STRIP, 20, 4);
}
