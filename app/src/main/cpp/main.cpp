#include <android_native_app_glue.h>
#include <android/log.h>
#include <android/input.h>
#include <android/asset_manager.h>
#include <jni.h>

#include <memory>

#include "EGLManager.h"
#include "Renderer.h"

#define LOG_TAG "OpenFlight"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

struct Engine {
    std::unique_ptr<EGLManager> egl;
    std::unique_ptr<Renderer> renderer;
    bool hasWindow = false;
};

static int32_t handle_input(struct android_app* app, AInputEvent* event) {
    Engine* engine = static_cast<Engine*>(app->userData);
    if (engine == nullptr || !engine->renderer) {
        return 0;
    }

    int32_t type = AInputEvent_getType(event);

    if (type != AINPUT_EVENT_TYPE_MOTION) {
        return 0;
    }

    int32_t action = AMotionEvent_getAction(event);
    int32_t maskedAction = action & AMOTION_EVENT_ACTION_MASK;
    size_t pointerCount = AMotionEvent_getPointerCount(event);

    if (pointerCount >= 2) {
        if (maskedAction == AMOTION_EVENT_ACTION_MOVE
            || maskedAction == AMOTION_EVENT_ACTION_POINTER_DOWN
            || maskedAction == AMOTION_EVENT_ACTION_POINTER_UP) {

            if (pointerCount >= 2) {
                float x0 = AMotionEvent_getX(event, 0);
                float y0 = AMotionEvent_getY(event, 0);
                float x1 = AMotionEvent_getX(event, 1);
                float y1 = AMotionEvent_getY(event, 1);

                engine->renderer->onMultiTouchMove(x0, y0, x1, y1);
                return 1;
            }
        }

        if (maskedAction == AMOTION_EVENT_ACTION_POINTER_UP) {
            if (pointerCount <= 2) {
                engine->renderer->onMultiTouchEnd();
                return 1;
            }
        }
    }

    if (maskedAction == AMOTION_EVENT_ACTION_DOWN) {
        float x = AMotionEvent_getX(event, 0);
        float y = AMotionEvent_getY(event, 0);
        engine->renderer->onTouchDown(x, y);
        return 1;
    }

    if (maskedAction == AMOTION_EVENT_ACTION_MOVE) {
        if (pointerCount == 1) {
            float x = AMotionEvent_getX(event, 0);
            float y = AMotionEvent_getY(event, 0);
            engine->renderer->onTouchMove(x, y);
            return 1;
        }
    }

    if (maskedAction == AMOTION_EVENT_ACTION_UP
        || maskedAction == AMOTION_EVENT_ACTION_CANCEL) {
        float x = AMotionEvent_getX(event, 0);
        float y = AMotionEvent_getY(event, 0);
        engine->renderer->onTouchUp(x, y);
        return 1;
    }

    return 0;
}

static void handle_cmd(struct android_app* app, int32_t cmd) {
    Engine* engine = static_cast<Engine*>(app->userData);
    if (engine == nullptr) {
        return;
    }

    switch (cmd) {
        case APP_CMD_INIT_WINDOW: {
            LOGI("APP_CMD_INIT_WINDOW");
            if (app->window == nullptr) {
                LOGE("APP_CMD_INIT_WINDOW: window is null");
                break;
            }

            if (!engine->egl) {
                engine->egl = std::make_unique<EGLManager>();
            }
            if (!engine->renderer) {
                engine->renderer = std::make_unique<Renderer>();
            }

            if (!engine->egl->init(app->window)) {
                LOGE("EGL init failed");
                engine->egl.reset();
                break;
            }

            if (!engine->renderer->init(app->activity->assetManager)) {
                LOGE("Renderer init failed");
                engine->egl.reset();
                engine->renderer.reset();
                break;
            }

            int32_t w = ANativeWindow_getWidth(app->window);
            int32_t h = ANativeWindow_getHeight(app->window);
            engine->renderer->resize(w, h);

            engine->hasWindow = true;
            break;
        }

        case APP_CMD_TERM_WINDOW: {
            LOGI("APP_CMD_TERM_WINDOW");
            engine->hasWindow = false;
            if (engine->renderer) {
                engine->renderer->shutdown();
                engine->renderer.reset();
            }
            if (engine->egl) {
                engine->egl->shutdown();
                engine->egl.reset();
            }
            break;
        }

        case APP_CMD_WINDOW_RESIZED: {
            LOGI("APP_CMD_WINDOW_RESIZED");
            if (engine->hasWindow && engine->renderer && app->window) {
                int32_t w = ANativeWindow_getWidth(app->window);
                int32_t h = ANativeWindow_getHeight(app->window);
                engine->renderer->resize(w, h);
            }
            break;
        }

        case APP_CMD_GAINED_FOCUS:
            LOGI("APP_CMD_GAINED_FOCUS");
            break;
        case APP_CMD_LOST_FOCUS:
            LOGI("APP_CMD_LOST_FOCUS");
            break;
        case APP_CMD_PAUSE:
            LOGI("APP_CMD_PAUSE");
            break;
        case APP_CMD_RESUME:
            LOGI("APP_CMD_RESUME");
            break;
        case APP_CMD_START:
            LOGI("APP_CMD_START");
            break;
        case APP_CMD_STOP:
            LOGI("APP_CMD_STOP");
            break;
        case APP_CMD_DESTROY:
            LOGI("APP_CMD_DESTROY");
            break;
        default:
            break;
    }
}

void android_main(struct android_app* app) {
    LOGI("OpenFlight 0.09 started");

    Engine engine;
    app->userData = &engine;
    app->onAppCmd = handle_cmd;
    app->onInputEvent = handle_input;

    while (true) {
        int events;
        struct android_poll_source* source;

        int result = ALooper_pollOnce(
            engine.hasWindow ? 0 : -1,
            nullptr,
            &events,
            reinterpret_cast<void**>(&source)
        );

        if (source != nullptr) {
            source->process(app, source);
        }

        if (app->destroyRequested != 0) {
            LOGI("OpenFlight shutting down");
            if (engine.renderer) {
                engine.renderer->shutdown();
                engine.renderer.reset();
            }
            if (engine.egl) {
                engine.egl->shutdown();
                engine.egl.reset();
            }
            return;
        }

        if (engine.hasWindow && engine.renderer && engine.egl) {
            engine.renderer->renderFrame();
            engine.egl->swapBuffers();
        }
    }
}