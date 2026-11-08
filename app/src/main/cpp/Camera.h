#pragma once

enum class CameraMode {
    Free,
    LockOn
};

class Camera {
public:
    Camera();

    void setPerspective(float fovDegrees, float aspect, float nearPlane, float farPlane);

    void setPosition(float x, float y, float z);
    void setTarget(float x, float y, float z);
    void setUp(float x, float y, float z);

    void setYaw(float yawRadians);
    void setPitch(float pitchRadians);
    void addYaw(float deltaRadians);
    void addPitch(float deltaRadians);

    void addDistance(float delta);
    void setDistance(float distance);
    float getDistance() const;

    void setMode(CameraMode mode);
    CameraMode getMode() const;
    void toggleMode();

    void setLockTarget(float x, float y, float z);

    float getYaw() const;
    float getPitch() const;

    void updateViewMatrix();
    void updateProjectionMatrix();

    const float* viewMatrix() const;
    const float* projectionMatrix() const;

private:
    void recalculateTargetFromAngles();

    float fov_;
    float aspect_;
    float near_;
    float far_;

    float position_[3];
    float target_[3];
    float up_[3];

    float yaw_;
    float pitch_;
    float distance_;

    CameraMode mode_;
    float lockTarget_[3];

    float view_[16];
    float projection_[16];
};