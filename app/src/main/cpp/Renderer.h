#pragma once

#include <memory>
#include <vector>
#include <android/asset_manager.h>

#include "Shader.h"
#include "Mesh.h"
#include "Camera.h"
#include "UIButton.h"

class Renderer {
public:
    Renderer();
    ~Renderer();

    bool init(AAssetManager* assetManager);
    void resize(int width, int height);
    void renderFrame();
    void shutdown();

    void onTouchDown(float x, float y);
    void onTouchMove(float x, float y);
    void onTouchUp(float x, float y);
    void onMultiTouchMove(float x0, float y0, float x1, float y1);
    void onMultiTouchEnd();

private:
    bool createGround();
    bool createRunway();
    bool createTaxiways();
    bool createStands();
    bool createBox();
    GLuint createCheckerboardTexture();

    bool initUI(AAssetManager* assetManager);
    void renderUI();
    void renderButton(const UIButton& button);

    ButtonId hitTestButton(float x, float y) const;
    UIButton* findButton(ButtonId id);

    void drawObject(const Mesh& mesh, const float* modelMatrix, GLuint textureId, float r, float g, float b);
    void drawAircraft();
    void drawAirport();
    void drawGround();

    int width_;
    int height_;
    bool initialized_;

    std::unique_ptr<Shader> shader_;
    std::unique_ptr<Shader> uiShader_;
    std::unique_ptr<Camera> camera_;

    std::unique_ptr<Mesh> groundMesh_;
    std::unique_ptr<Mesh> runwayMesh_;
    std::unique_ptr<Mesh> taxiwayMesh_;
    std::unique_ptr<Mesh> standMesh_;
    std::unique_ptr<Mesh> boxMesh_;
    std::unique_ptr<Mesh> uiQuadMesh_;

    GLuint whiteTexture_;
    GLuint checkerTexture_;

    GLint uMvpLocation_;
    GLint uColorLocation_;
    GLint uTextureLoc_;
    GLint uLightDirLoc_;

    GLint uiPositionAttr_;
    GLint uiScreenSizeLoc_;
    GLint uiButtonPosLoc_;
    GLint uiButtonSizeLoc_;
    GLint uiTextureLoc_;
    GLint uiColorTintLoc_;

    std::vector<UIButton> buttons_;

    GLuint camTexture_;
    GLuint zoomInTexture_;
    GLuint zoomOutTexture_;
    GLuint resetTexture_;
    GLuint pauseTexture_;

    ButtonId activeButton_;
    bool paused_;

    bool touchActive_;
    float lastTouchX_;
    float lastTouchY_;

    bool multiTouchActive_;
    float lastPinchDistance_;
    float lastAvgY_;
    float accumulatedSwipeY_;
    bool swipeTriggered_;

    static constexpr float TOUCH_SENSITIVITY = 0.005f;
    static constexpr float PINCH_SENSITIVITY = 0.5f;
    static constexpr float SWIPE_TRIGGER = 300.0f;
    static constexpr float PINCH_DOMINANCE = 3.0f;
    static constexpr float BUTTON_SIZE = 140.0f;
    static constexpr float BUTTON_MARGIN = 20.0f;
};