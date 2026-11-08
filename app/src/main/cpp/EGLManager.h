#pragma once

#include <EGL/egl.h>
#include <android/native_window.h>

class EGLManager {
public:
    EGLManager();
    ~EGLManager();

    bool init(ANativeWindow* window);
    void shutdown();

    bool isValid() const;
    void swapBuffers();

    EGLDisplay getDisplay() const;
    EGLSurface getSurface() const;
    EGLContext getContext() const;

private:
    EGLDisplay display_;
    EGLSurface surface_;
    EGLContext context_;
    EGLConfig config_;
    bool valid_;
};