#include "Camera.h"
#include <cmath>
#include <cstring>
#include <android/log.h>

#define LOG_TAG "OpenFlight"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

static void normalize3(float* v) {
    float len = std::sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
    if (len > 0.0001f) {
        v[0] /= len; v[1] /= len; v[2] /= len;
    }
}

static void cross3(const float* a, const float* b, float* out) {
    out[0] = a[1]*b[2] - a[2]*b[1];
    out[1] = a[2]*b[0] - a[0]*b[2];
    out[2] = a[0]*b[1] - a[1]*b[0];
}

static float dot3(const float* a, const float* b) {
    return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
}

Camera::Camera()
    : fov_(60.0f)
    , aspect_(1.0f)
    , near_(1.0f)        // БЫЛО 0.5f
    , far_(20000.0f)     // БЫЛО 100000.0f
    , yaw_(0.35f)
    , pitch_(-0.3f)
    , distance_(300.0f)
    , mode_(CameraMode::Free) {
    position_[0] = -120.0f; position_[1] = 60.0f; position_[2] = 1150.0f;
    target_[0]   = 0.0f;    target_[1]   = 0.0f;  target_[2]   = 1500.0f;
    up_[0]       = 0.0f;    up_[1]       = 1.0f;  up_[2]       = 0.0f;

    lockTarget_[0] = 0.0f;
    lockTarget_[1] = 2.0f;
    lockTarget_[2] = 1500.0f;

    std::memset(view_, 0, sizeof(view_));
    std::memset(projection_, 0, sizeof(projection_));
}

void Camera::setPerspective(float fovDegrees, float aspect, float nearPlane, float farPlane) {
    fov_ = fovDegrees; aspect_ = aspect; near_ = nearPlane; far_ = farPlane;
    updateProjectionMatrix();
}
void Camera::setPosition(float x, float y, float z) { position_[0] = x; position_[1] = y; position_[2] = z; }
void Camera::setTarget(float x, float y, float z) { target_[0] = x; target_[1] = y; target_[2] = z; }
void Camera::setUp(float x, float y, float z) { up_[0] = x; up_[1] = y; up_[2] = z; }

void Camera::setYaw(float yawRadians) { yaw_ = yawRadians; recalculateTargetFromAngles(); }
void Camera::setPitch(float pitchRadians) {
    const float MIN_PITCH = -1.4f; const float MAX_PITCH = 1.4f;
    if (pitchRadians < MIN_PITCH) pitchRadians = MIN_PITCH;
    if (pitchRadians > MAX_PITCH) pitchRadians = MAX_PITCH;
    pitch_ = pitchRadians; recalculateTargetFromAngles();
}
void Camera::addYaw(float deltaRadians) { setYaw(yaw_ + deltaRadians); }
void Camera::addPitch(float deltaRadians) { setPitch(pitch_ + deltaRadians); }

void Camera::addDistance(float delta) { setDistance(distance_ + delta); }
void Camera::setDistance(float distance) {
    const float MIN_DIST = 30.0f; const float MAX_DIST = 10000.0f;
    if (distance < MIN_DIST) distance = MIN_DIST;
    if (distance > MAX_DIST) distance = MAX_DIST;
    distance_ = distance; recalculateTargetFromAngles();
}
float Camera::getDistance() const { return distance_; }

void Camera::setMode(CameraMode mode) {
    mode_ = mode;
    if (mode_ == CameraMode::LockOn) { yaw_ = 0.0f; pitch_ = -0.2f; distance_ = 150.0f; }
}
CameraMode Camera::getMode() const { return mode_; }
void Camera::toggleMode() { setMode(mode_ == CameraMode::Free ? CameraMode::LockOn : CameraMode::Free); }
void Camera::setLockTarget(float x, float y, float z) { lockTarget_[0] = x; lockTarget_[1] = y; lockTarget_[2] = z; }
float Camera::getYaw() const { return yaw_; }
float Camera::getPitch() const { return pitch_; }

void Camera::recalculateTargetFromAngles() {
    float cosPitch = std::cos(pitch_); float sinPitch = std::sin(pitch_);
    float cosYaw   = std::cos(yaw_);   float sinYaw   = std::sin(yaw_);
    float dirX = sinYaw * cosPitch; float dirY = sinPitch; float dirZ = cosYaw * cosPitch;

    if (mode_ == CameraMode::LockOn) {
        position_[0] = lockTarget_[0] - dirX * distance_;
        position_[1] = lockTarget_[1] - dirY * distance_;
        position_[2] = lockTarget_[2] - dirZ * distance_;
        target_[0] = lockTarget_[0]; target_[1] = lockTarget_[1]; target_[2] = lockTarget_[2];
    } else {
        target_[0] = position_[0] + dirX * distance_;
        target_[1] = position_[1] + dirY * distance_;
        target_[2] = position_[2] + dirZ * distance_;
    }
}

void Camera::updateViewMatrix() {
    if (mode_ == CameraMode::LockOn) recalculateTargetFromAngles();
    float forward[3] = { target_[0]-position_[0], target_[1]-position_[1], target_[2]-position_[2] };
    normalize3(forward);
    float right[3]; cross3(forward, up_, right); normalize3(right);
    float trueUp[3]; cross3(right, forward, trueUp);

    view_[0]=right[0]; view_[4]=right[1]; view_[8]=right[2]; view_[12]=-dot3(right, position_);
    view_[1]=trueUp[0]; view_[5]=trueUp[1]; view_[9]=trueUp[2]; view_[13]=-dot3(trueUp, position_);
    view_[2]=-forward[0]; view_[6]=-forward[1]; view_[10]=-forward[2]; view_[14]=dot3(forward, position_);
    view_[3]=0.0f; view_[7]=0.0f; view_[11]=0.0f; view_[15]=1.0f;
}

void Camera::updateProjectionMatrix() {
    float fovRadians = fov_ * 3.14159265f / 180.0f;
    float f = 1.0f / std::tan(fovRadians / 2.0f);
    std::memset(projection_, 0, sizeof(projection_));
    projection_[0] = f / aspect_; projection_[5] = f;
    projection_[10] = (far_ + near_) / (near_ - far_);
    projection_[11] = -1.0f; projection_[14] = (2.0f * far_ * near_) / (near_ - far_);
}

const float* Camera::viewMatrix() const { return view_; }
const float* Camera::projectionMatrix() const { return projection_; }