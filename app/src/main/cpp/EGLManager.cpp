#include "EGLManager.h"
#include <android/log.h>

#define LOG_TAG "OpenFlight"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

EGLManager::EGLManager()
    : display_(EGL_NO_DISPLAY)
    , surface_(EGL_NO_SURFACE)
    , context_(EGL_NO_CONTEXT)
    , config_(nullptr)
    , valid_(false) {
}

EGLManager::~EGLManager() {
    shutdown();
}

bool EGLManager::init(ANativeWindow* window) {
    if (window == nullptr) {
        LOGE("EGLManager::init: window is null");
        return false;
    }

    display_ = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (display_ == EGL_NO_DISPLAY) {
        LOGE("eglGetDisplay failed");
        return false;
    }

    EGLint majorVersion = 0;
    EGLint minorVersion = 0;
    if (!eglInitialize(display_, &majorVersion, &minorVersion)) {
        LOGE("eglInitialize failed");
        return false;
    }
    LOGI("EGL initialized: version %d.%d", majorVersion, minorVersion);

    const EGLint configAttribs[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_SURFACE_TYPE,    EGL_WINDOW_BIT,
        EGL_RED_SIZE,        8,
        EGL_GREEN_SIZE,      8,
        EGL_BLUE_SIZE,       8,
        EGL_ALPHA_SIZE,      8,
        EGL_DEPTH_SIZE,      24,
        EGL_NONE
    };

    EGLint numConfigs = 0;
    if (!eglChooseConfig(display_, configAttribs, &config_, 1, &numConfigs)
        || numConfigs < 1) {
        LOGE("eglChooseConfig failed with 24-bit depth, trying 16-bit");

        const EGLint configAttribs16[] = {
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
            EGL_SURFACE_TYPE,    EGL_WINDOW_BIT,
            EGL_RED_SIZE,        8,
            EGL_GREEN_SIZE,      8,
            EGL_BLUE_SIZE,       8,
            EGL_ALPHA_SIZE,      8,
            EGL_DEPTH_SIZE,      16,
            EGL_NONE
        };

        if (!eglChooseConfig(display_, configAttribs16, &config_, 1, &numConfigs)
            || numConfigs < 1) {
            LOGE("eglChooseConfig failed with 16-bit depth too");
            return false;
        }
        LOGI("Using 16-bit depth buffer (24 not supported)");
    } else {
        LOGI("Using 24-bit depth buffer");
    }

    EGLint depthSize = 0;
    eglGetConfigAttrib(display_, config_, EGL_DEPTH_SIZE, &depthSize);
    LOGI("Actual depth buffer size: %d bits", depthSize);

    const EGLint contextAttribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };

    context_ = eglCreateContext(display_, config_, EGL_NO_CONTEXT, contextAttribs);
    if (context_ == EGL_NO_CONTEXT) {
        LOGE("eglCreateContext failed");
        return false;
    }

    surface_ = eglCreateWindowSurface(display_, config_, window, nullptr);
    if (surface_ == EGL_NO_SURFACE) {
        LOGE("eglCreateWindowSurface failed");
        return false;
    }

    if (!eglMakeCurrent(display_, surface_, surface_, context_)) {
        LOGE("eglMakeCurrent failed");
        return false;
    }

    valid_ = true;
    LOGI("EGL context and surface ready");
    return true;
}

void EGLManager::shutdown() {
    if (display_ == EGL_NO_DISPLAY) {
        return;
    }

    eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

    if (context_ != EGL_NO_CONTEXT) {
        eglDestroyContext(display_, context_);
        context_ = EGL_NO_CONTEXT;
    }

    if (surface_ != EGL_NO_SURFACE) {
        eglDestroySurface(display_, surface_);
        surface_ = EGL_NO_SURFACE;
    }

    eglTerminate(display_);
    display_ = EGL_NO_DISPLAY;
    valid_ = false;

    LOGI("EGL shutdown complete");
}

bool EGLManager::isValid() const {
    return valid_;
}

void EGLManager::swapBuffers() {
    if (valid_ && display_ != EGL_NO_DISPLAY && surface_ != EGL_NO_SURFACE) {
        eglSwapBuffers(display_, surface_);
    }
}

EGLDisplay EGLManager::getDisplay() const { return display_; }
EGLSurface EGLManager::getSurface() const { return surface_; }
EGLContext EGLManager::getContext() const { return context_; }