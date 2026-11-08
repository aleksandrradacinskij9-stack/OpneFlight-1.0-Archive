#include "Renderer.h"
#include "ImageLoader.h"

#include <GLES2/gl2.h>
#include <android/log.h>
#include <cmath>
#include <cstring>
#include <vector>

#define LOG_TAG "OpenFlight"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

static const char* VERTEX_SHADER = R"(
attribute vec3 aPosition;
attribute vec2 aTexCoord;
attribute vec3 aNormal;

uniform mat4 uMvp;

varying vec2 vTexCoord;
varying vec3 vNormal;

void main() {
    gl_Position = uMvp * vec4(aPosition, 1.0);
    vTexCoord = aTexCoord;
    vNormal = aNormal;
}
)";

static const char* FRAGMENT_SHADER = R"(
precision mediump float;

uniform sampler2D uTexture;
uniform vec3 uColor;
uniform vec3 uLightDir;

varying vec2 vTexCoord;
varying vec3 vNormal;

void main() {
    vec4 texColor = texture2D(uTexture, vTexCoord);
    float diff = max(dot(normalize(vNormal), normalize(uLightDir)), 0.4);
    gl_FragColor = texColor * vec4(uColor * diff, 1.0);
}
)";

static const char* UI_VERTEX_SHADER = R"(
attribute vec2 aPosition;
uniform vec2 uScreenSize;
uniform vec2 uButtonPos;
uniform vec2 uButtonSize;
varying vec2 vTexCoord;
void main() {
    vec2 screenPos = uButtonPos + aPosition * uButtonSize;
    vec2 ndc = (screenPos / uScreenSize) * 2.0 - 1.0;
    ndc.y = -ndc.y;
    gl_Position = vec4(ndc, 0.0, 1.0);
    vTexCoord = aPosition;
}
)";

static const char* UI_FRAGMENT_SHADER = R"(
precision mediump float;
uniform sampler2D uTexture;
uniform vec4 uColorTint;
varying vec2 vTexCoord;
void main() {
    gl_FragColor = texture2D(uTexture, vTexCoord) * uColorTint;
}
)";

static void identity(float* m) {
    std::memset(m, 0, sizeof(float) * 16);
    m[0] = 1.0f; m[5] = 1.0f; m[10] = 1.0f; m[15] = 1.0f;
}

static void multiplyMat4(const float* a, const float* b, float* out) {
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            out[i * 4 + j] = a[0 * 4 + j] * b[i * 4 + 0] + a[1 * 4 + j] * b[i * 4 + 1] + a[2 * 4 + j] * b[i * 4 + 2] + a[3 * 4 + j] * b[i * 4 + 3];
        }
    }
}

static void translationMatrix(float* m, float x, float y, float z) {
    identity(m); m[12] = x; m[13] = y; m[14] = z;
}

static void rotationYMatrix(float* m, float angleRadians) {
    float c = std::cos(angleRadians); float s = std::sin(angleRadians);
    identity(m); m[0] = c; m[2] = -s; m[8] = s; m[10] = c;
}

Renderer::Renderer()
    : width_(0), height_(0), initialized_(false)
    , uMvpLocation_(-1), uColorLocation_(-1), uTextureLoc_(-1), uLightDirLoc_(-1)
    , uiPositionAttr_(-1), uiScreenSizeLoc_(-1), uiButtonPosLoc_(-1), uiButtonSizeLoc_(-1), uiTextureLoc_(-1), uiColorTintLoc_(-1)
    , whiteTexture_(0), checkerTexture_(0)
    , camTexture_(0), zoomInTexture_(0), zoomOutTexture_(0), resetTexture_(0), pauseTexture_(0)
    , activeButton_(ButtonId::None), paused_(false)
    , touchActive_(false), lastTouchX_(0.0f), lastTouchY_(0.0f)
    , multiTouchActive_(false), lastPinchDistance_(0.0f), lastAvgY_(0.0f), accumulatedSwipeY_(0.0f), swipeTriggered_(false) {
}

Renderer::~Renderer() { shutdown(); }

// Вспомогательная функция: генерирует плоскость с повторяющейся текстурой
static void pushPlaneVertices(std::vector<float>& v, float w, float l, float y, float nx, float ny, float nz) {
    float hw = w * 0.5f; float hl = l * 0.5f;
    float uScale = w / 20.0f; // 20 метров = 1 квадрат текстуры
    float vScale = l / 20.0f;
    
    // CCW (по часовой, если смотреть сверху)
    v.insert(v.end(), { -hw, y, -hl, 0.0f,    0.0f,    nx, ny, nz });
    v.insert(v.end(), { -hw, y,  hl, 0.0f,    vScale,  nx, ny, nz });
    v.insert(v.end(), {  hw, y,  hl, uScale,  vScale,  nx, ny, nz });
    
    v.insert(v.end(), { -hw, y, -hl, 0.0f,    0.0f,    nx, ny, nz });
    v.insert(v.end(), {  hw, y,  hl, uScale,  vScale,  nx, ny, nz });
    v.insert(v.end(), {  hw, y, -hl, uScale,  0.0f,    nx, ny, nz });
}

bool Renderer::createGround() {
    std::vector<float> v;
    pushPlaneVertices(v, 50000.0f, 50000.0f, -2.0f, 0.0f, 1.0f, 0.0f); // Земля на -2.0
    groundMesh_ = std::make_unique<Mesh>();
    return groundMesh_->create(v.data(), 6, 8);
}

bool Renderer::createRunway() {
    std::vector<float> v;
    pushPlaneVertices(v, 60.0f, 3800.0f, 0.1f, 0.0f, 1.0f, 0.0f); // Полоса на 0.1
    runwayMesh_ = std::make_unique<Mesh>();
    return runwayMesh_->create(v.data(), 6, 8);
}

bool Renderer::createTaxiways() {
    std::vector<float> v;
    pushPlaneVertices(v, 30.0f, 4000.0f, 0.0f, 0.0f, 1.0f, 0.0f); // Рулёжки на 0.0
    taxiwayMesh_ = std::make_unique<Mesh>();
    return taxiwayMesh_->create(v.data(), 6, 8);
}

bool Renderer::createStands() {
    std::vector<float> v;
    pushPlaneVertices(v, 60.0f, 80.0f, 0.2f, 0.0f, 1.0f, 0.0f); // Стоянки на 0.2
    standMesh_ = std::make_unique<Mesh>();
    return standMesh_->create(v.data(), 6, 8);
}

bool Renderer::createBox() {
    // Куб с правильным CCW обходом
    float w = 1.0f, h = 1.0f, d = 1.0f;
    float v[] = {
        // Front Face (Z = +d)
        -w, 0,  d,  0, 0,  0, 0, 1,   w, 0,  d,  1, 0,  0, 0, 1,   w, h,  d,  1, 1,  0, 0, 1,
        -w, 0,  d,  0, 0,  0, 0, 1,   w, h,  d,  1, 1,  0, 0, 1,  -w, h,  d,  0, 1,  0, 0, 1,
        // Back Face (Z = -d)
         w, 0, -d,  0, 0,  0, 0,-1,  -w, 0, -d,  1, 0,  0, 0,-1,  -w, h, -d,  1, 1,  0, 0,-1,
         w, 0, -d,  0, 0,  0, 0,-1,  -w, h, -d,  1, 1,  0, 0,-1,   w, h, -d,  0, 1,  0, 0,-1,
        // Top Face (Y = +h)
        -w, h,  d,  0, 0,  0, 1, 0,   w, h,  d,  1, 0,  0, 1, 0,   w, h, -d,  1, 1,  0, 1, 0,
        -w, h,  d,  0, 0,  0, 1, 0,   w, h, -d,  1, 1,  0, 1, 0,  -w, h, -d,  0, 1,  0, 1, 0,
        // Bottom Face (Y = 0)
        -w, 0, -d,  0, 0,  0,-1, 0,   w, 0, -d,  1, 0,  0,-1, 0,   w, 0,  d,  1, 1,  0,-1, 0,
        -w, 0, -d,  0, 0,  0,-1, 0,   w, 0,  d,  1, 1,  0,-1, 0,  -w, 0,  d,  0, 1,  0,-1, 0,
        // Right Face (X = +w)
         w, 0,  d,  0, 0,  1, 0, 0,   w, 0, -d,  1, 0,  1, 0, 0,   w, h, -d,  1, 1,  1, 0, 0,
         w, 0,  d,  0, 0,  1, 0, 0,   w, h, -d,  1, 1,  1, 0, 0,   w, h,  d,  0, 1,  1, 0, 0,
        // Left Face (X = -w)
        -w, 0, -d,  0, 0, -1, 0, 0,  -w, 0,  d,  1, 0, -1, 0, 0,  -w, h,  d,  1, 1, -1, 0, 0,
        -w, 0, -d,  0, 0, -1, 0, 0,  -w, h,  d,  1, 1, -1, 0, 0,  -w, h, -d,  0, 1, -1, 0, 0
    };
    boxMesh_ = std::make_unique<Mesh>();
    return boxMesh_->create(v, 36, 8);
}

GLuint Renderer::createCheckerboardTexture() {
    GLubyte pixels[64 * 64 * 4];
    for (int y = 0; y < 64; y++) {
        for (int x = 0; x < 64; x++) {
            int c = ((x / 8) + (y / 8)) % 2 == 0 ? 200 : 100; // Чередование
            int i = (y * 64 + x) * 4;
            pixels[i+0] = c; pixels[i+1] = c; pixels[i+2] = c; pixels[i+3] = 255;
        }
    }
    GLuint tex; glGenTextures(1, &tex); glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 64, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // ВАЖНО: Повторение текстуры
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D, 0);
    return tex;
}

bool Renderer::initUI(AAssetManager* assetManager) {
    uiShader_ = std::make_unique<Shader>();
    if (!uiShader_->build(UI_VERTEX_SHADER, UI_FRAGMENT_SHADER)) return false;
    uiPositionAttr_ = uiShader_->getAttributeLocation("aPosition");
    uiScreenSizeLoc_ = uiShader_->getUniformLocation("uScreenSize");
    uiButtonPosLoc_ = uiShader_->getUniformLocation("uButtonPos");
    uiButtonSizeLoc_ = uiShader_->getUniformLocation("uButtonSize");
    uiTextureLoc_ = uiShader_->getUniformLocation("uTexture");
    uiColorTintLoc_ = uiShader_->getUniformLocation("uColorTint");

    const float quadVertices[] = { 0,0, 1,0, 1,1, 0,0, 1,1, 0,1 };
    uiQuadMesh_ = std::make_unique<Mesh>();
    if (!uiQuadMesh_->create(quadVertices, 6, 2)) return false;

    camTexture_ = ImageLoader::loadTextureFromAssets(assetManager, "icons/cam.png");
    zoomInTexture_ = ImageLoader::loadTextureFromAssets(assetManager, "icons/zoom_in.png");
    zoomOutTexture_ = ImageLoader::loadTextureFromAssets(assetManager, "icons/zoom_out.png");
    resetTexture_ = ImageLoader::loadTextureFromAssets(assetManager, "icons/reset.png");
    pauseTexture_ = ImageLoader::loadTextureFromAssets(assetManager, "icons/pause.png");
    return true;
}

bool Renderer::init(AAssetManager* assetManager) {
    glClearColor(0.45f, 0.72f, 0.95f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    
    // ВКЛЮЧАЕМ ОТСЕЧЕНИЕ
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    camera_ = std::make_unique<Camera>();
    camera_->setLockTarget(0.0f, 2.0f, 1500.0f);

    // НОВЫЙ ШЕЙДЕР 3D
    shader_ = std::make_unique<Shader>();
    if (!shader_->build(VERTEX_SHADER, FRAGMENT_SHADER)) return false;
    
    uMvpLocation_ = shader_->getUniformLocation("uMvp");
    uColorLocation_ = shader_->getUniformLocation("uColor");
    uTextureLoc_ = shader_->getUniformLocation("uTexture");
    uLightDirLoc_ = shader_->getUniformLocation("uLightDir");

    // 0.12: Белая текстура для зданий
    GLubyte whitePixel[4] = {255, 255, 255, 255};
    glGenTextures(1, &whiteTexture_); glBindTexture(GL_TEXTURE_2D, whiteTexture_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, whitePixel);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    
    // 0.12: Шахматная текстура для ВПП
    checkerTexture_ = createCheckerboardTexture();

    if (!createGround()) return false;
    if (!createRunway()) return false;
    if (!createTaxiways()) return false;
    if (!createStands()) return false;
    if (!createBox()) return false;
    if (!initUI(assetManager)) return false;

    initialized_ = true;
    LOGI("Renderer initialized (0.13 fix)");
    return true;
}

void Renderer::resize(int width, int height) {
    width_ = width; height_ = height;
    if (width_ > 0 && height_ > 0) {
        glViewport(0, 0, width_, height_);
        if (camera_) {
            float aspect = static_cast<float>(width_) / static_cast<float>(height_);
            camera_->setPerspective(60.0f, aspect, 1.0f, 20000.0f);
        }
        buttons_.clear();
        float bx = BUTTON_MARGIN * 2.0f;
        float by = BUTTON_MARGIN * 2.0f;
        
        UIButton b; b.setup(ButtonId::Camera, bx, by, BUTTON_SIZE, camTexture_); buttons_.push_back(b);
        b.setup(ButtonId::ZoomIn, bx, by + (BUTTON_SIZE + BUTTON_MARGIN), BUTTON_SIZE, zoomInTexture_); buttons_.push_back(b);
        b.setup(ButtonId::ZoomOut, bx, by + 2.0f * (BUTTON_SIZE + BUTTON_MARGIN), BUTTON_SIZE, zoomOutTexture_); buttons_.push_back(b);
        b.setup(ButtonId::Reset, bx, by + 3.0f * (BUTTON_SIZE + BUTTON_MARGIN), BUTTON_SIZE, resetTexture_); buttons_.push_back(b);
        b.setup(ButtonId::Pause, bx, by + 4.0f * (BUTTON_SIZE + BUTTON_MARGIN), BUTTON_SIZE, pauseTexture_); buttons_.push_back(b);
    }
}

ButtonId Renderer::hitTestButton(float x, float y) const {
    for (const auto& b : buttons_) if (b.containsPoint(x, y)) return b.getId();
    return ButtonId::None;
}
UIButton* Renderer::findButton(ButtonId id) {
    for (auto& b : buttons_) if (b.getId() == id) return &b;
    return nullptr;
}

void Renderer::onTouchDown(float x, float y) {
    ButtonId hit = hitTestButton(x, y);
    if (hit != ButtonId::None) {
        activeButton_ = hit;
        UIButton* btn = findButton(hit); if (btn) btn->setPressed(true);
        
        if (hit == ButtonId::Camera) { camera_->toggleMode(); if (btn) btn->setPressed(false); activeButton_ = ButtonId::None; }
        if (hit == ButtonId::Reset) { LOGI("RESET"); if (btn) btn->setPressed(false); activeButton_ = ButtonId::None; }
        if (hit == ButtonId::Pause) { paused_ = !paused_; if (btn) btn->setPressed(false); activeButton_ = ButtonId::None; }
        return;
    }
    activeButton_ = ButtonId::None; touchActive_ = true; multiTouchActive_ = false;
    lastTouchX_ = x; lastTouchY_ = y;
}

void Renderer::onTouchMove(float x, float y) {
    if (activeButton_ == ButtonId::ZoomIn) { camera_->addDistance(-10.0f); return; }
    if (activeButton_ == ButtonId::ZoomOut) { camera_->addDistance(10.0f); return; }
    if (!touchActive_ || multiTouchActive_ || !camera_) return;
    camera_->addYaw(-(x - lastTouchX_) * TOUCH_SENSITIVITY);
    camera_->addPitch(-(y - lastTouchY_) * TOUCH_SENSITIVITY);
    lastTouchX_ = x; lastTouchY_ = y;
}

void Renderer::onTouchUp(float x, float y) {
    if (activeButton_ != ButtonId::None) {
        UIButton* btn = findButton(activeButton_); if (btn) btn->setPressed(false);
        activeButton_ = ButtonId::None;
    }
    touchActive_ = false; multiTouchActive_ = false;
    accumulatedSwipeY_ = 0.0f; swipeTriggered_ = false;
}

void Renderer::onMultiTouchMove(float x0, float y0, float x1, float y1) {
    if (!camera_) return;
    float dx = x1 - x0; float dy = y1 - y0; float dist = std::sqrt(dx*dx + dy*dy);
    float avgY = (y0 + y1) * 0.5f;
    
    if (!multiTouchActive_) {
        multiTouchActive_ = true; lastPinchDistance_ = dist; lastAvgY_ = avgY;
        accumulatedSwipeY_ = 0.0f; swipeTriggered_ = false; return;
    }
    float deltaDist = dist - lastPinchDistance_; float deltaY = avgY - lastAvgY_;
    lastPinchDistance_ = dist; lastAvgY_ = avgY;
    
    if (std::fabs(deltaDist) > PINCH_DOMINANCE) {
        camera_->addDistance(-deltaDist * PINCH_SENSITIVITY); accumulatedSwipeY_ = 0.0f; return;
    }
    if (swipeTriggered_) return;
    accumulatedSwipeY_ += deltaY;
    if (std::fabs(accumulatedSwipeY_) > SWIPE_TRIGGER) { camera_->toggleMode(); swipeTriggered_ = true; accumulatedSwipeY_ = 0.0f; }
}

void Renderer::onMultiTouchEnd() { multiTouchActive_ = false; accumulatedSwipeY_ = 0.0f; swipeTriggered_ = false; }

void Renderer::drawObject(const Mesh& mesh, const float* modelMatrix, GLuint textureId, float r, float g, float b) {
    camera_->updateViewMatrix(); camera_->updateProjectionMatrix();
    float vp[16]; multiplyMat4(camera_->projectionMatrix(), camera_->viewMatrix(), vp);
    float mvp[16]; multiplyMat4(vp, modelMatrix, mvp);

    glUniformMatrix4fv(uMvpLocation_, 1, GL_FALSE, mvp);
    glUniform3f(uColorLocation_, r, g, b);
    glUniform3f(uLightDirLoc_, 0.5f, 1.0f, 0.3f); // Направление света
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glUniform1i(uTextureLoc_, 0);

    mesh.draw();
}

void Renderer::drawAircraft() {
    float runRad = 147.0f * 3.14159265f / 180.0f;
    float rot[16], trans[16], matrix[16];
    rotationYMatrix(rot, runRad); translationMatrix(trans, 0.0f, 0.2f, 1500.0f); // Y=0.2 (на полосе)
    multiplyMat4(trans, rot, matrix);

    auto drawBox = [&](float lx, float ly, float lz, float sx, float sy, float sz, float r, float g, float b) {
        float lT[16], lS[16], lM[16], fM[16];
        identity(lS); lS[0]=sx; lS[5]=sy; lS[10]=sz;
        translationMatrix(lT, lx, ly, lz);
        multiplyMat4(lT, lS, lM); multiplyMat4(matrix, lM, fM);
        drawObject(*boxMesh_, fM, whiteTexture_, r, g, b);
    };

    drawBox(0, 2, 0, 1.5f, 1.5f, 22.0f, 0.88f, 0.88f, 0.90f);
    drawBox(0, 2, 12, 0.7f, 1.0f, 4.0f, 0.88f, 0.88f, 0.90f);
    drawBox(0, 2, 16.5f, 0.3f, 3.5f, 1.0f, 0.30f, 0.30f, 0.32f);
    drawBox(-14, 2, -1, 14.0f, 0.3f, 3.5f, 0.88f, 0.88f, 0.90f);
    drawBox( 14, 2, -1, 14.0f, 0.3f, 3.5f, 0.88f, 0.88f, 0.90f);
    drawBox(-2.5f, 0.8f, -3, 1.2f, 1.2f, 5.0f, 0.55f, 0.55f, 0.58f);
    drawBox( 2.5f, 0.8f, -3, 1.2f, 1.2f, 5.0f, 0.55f, 0.55f, 0.58f);
}

void Renderer::drawGround() { float id[16]; identity(id); drawObject(*groundMesh_, id, whiteTexture_, 0.35f, 0.45f, 0.30f); }

void Renderer::drawAirport() {
    float runRad = 147.0f * 3.14159265f / 180.0f;
    float id[16]; identity(id); float rot[16]; rotationYMatrix(rot, runRad);

    auto drawObj = [&](Mesh* m, float x, float y, float z, GLuint tex, float r, float g, float b) {
        float t[16], model[16]; translationMatrix(t, x, y, z); multiplyMat4(t, rot, model);
        drawObject(*m, model, tex, r, g, b);
    };

    drawObj(runwayMesh_.get(), 0, 0, 1500, checkerTexture_, 1.0f, 1.0f, 1.0f);
    drawObj(taxiwayMesh_.get(), -220, 0, 1500, whiteTexture_, 0.55f, 0.55f, 0.58f);
    drawObj(taxiwayMesh_.get(),  220, 0, 1500, whiteTexture_, 0.55f, 0.55f, 0.58f);
    drawObj(standMesh_.get(), -500, 0, 800, whiteTexture_, 0.5f, 0.5f, 0.5f);
    drawObj(standMesh_.get(), -500, 0, 1000, whiteTexture_, 0.5f, 0.5f, 0.5f);
    drawObj(standMesh_.get(),  500, 0, 800, whiteTexture_, 0.5f, 0.5f, 0.5f);
    drawObj(standMesh_.get(),  500, 0, 1000, whiteTexture_, 0.5f, 0.5f, 0.5f);

    auto drawBuild = [&](float x, float z, float sx, float sy, float sz, float r, float g, float b) {
        float t[16], s[16], m[16], f[16];
        identity(s); s[0]=sx; s[5]=sy; s[10]=sz;
        translationMatrix(t, x, 0, z); multiplyMat4(t, s, m); multiplyMat4(m, id, f);
        drawObject(*boxMesh_, f, whiteTexture_, r, g, b);
    };
    drawBuild(-550, 1200, 100, 15, 50, 0.75f, 0.75f, 0.72f);
    drawBuild(-550, 900, 5, 20, 5, 0.65f, 0.65f, 0.68f);
    drawBuild(550, 1200, 40, 10, 30, 0.70f, 0.70f, 0.72f);
    drawBuild(700, 1200, 40, 10, 30, 0.70f, 0.70f, 0.72f);
}

void Renderer::renderButton(const UIButton& button) {
    if (button.getTexture() == 0) return;
    float tint[4] = {0.35f, 0.35f, 0.55f, 0.85f};
    if (button.isPressed()) { tint[0]=1.0f; tint[1]=0.7f; tint[2]=0.7f; }
    else if (button.getId() == ButtonId::Pause && paused_) { tint[0]=0.9f; tint[1]=0.3f; tint[2]=0.3f; }

    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, button.getTexture());
    glUniform1i(uiTextureLoc_, 0); glUniform4fv(uiColorTintLoc_, 1, tint);
    glUniform2f(uiButtonPosLoc_, button.getX(), button.getY());
    glUniform2f(uiButtonSizeLoc_, button.getSize(), button.getSize());
    
    glBindBuffer(GL_ARRAY_BUFFER, uiQuadMesh_->getVbo());
    glEnableVertexAttribArray(uiPositionAttr_);
    glVertexAttribPointer(uiPositionAttr_, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glDisableVertexAttribArray(uiPositionAttr_);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Renderer::renderUI() {
    if (!uiShader_ || !uiQuadMesh_ || buttons_.empty()) return;
    glDisable(GL_DEPTH_TEST); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    uiShader_->use();
    glUniform2f(uiScreenSizeLoc_, (float)width_, (float)height_);
    for (const auto& b : buttons_) renderButton(b);
    glDisable(GL_BLEND); glEnable(GL_DEPTH_TEST);
}

void Renderer::renderFrame() {
    if (!initialized_) return;
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if (!shader_ || !camera_) return;
    shader_->use();
    drawGround(); drawAirport(); drawAircraft();
    renderUI();
}

void Renderer::shutdown() {
    if (boxMesh_) boxMesh_->destroy(); if (standMesh_) standMesh_->destroy();
    if (taxiwayMesh_) taxiwayMesh_->destroy(); if (runwayMesh_) runwayMesh_->destroy();
    if (groundMesh_) groundMesh_->destroy(); if (uiQuadMesh_) uiQuadMesh_->destroy();
    if (shader_) shader_->destroy(); if (uiShader_) uiShader_->destroy();
    if (camTexture_) glDeleteTextures(1, &camTexture_); if (zoomInTexture_) glDeleteTextures(1, &zoomInTexture_);
    if (zoomOutTexture_) glDeleteTextures(1, &zoomOutTexture_); if (resetTexture_) glDeleteTextures(1, &resetTexture_);
    if (pauseTexture_) glDeleteTextures(1, &pauseTexture_);
    if (whiteTexture_) glDeleteTextures(1, &whiteTexture_); if (checkerTexture_) glDeleteTextures(1, &checkerTexture_);
    camera_.reset(); buttons_.clear(); initialized_ = false;
}