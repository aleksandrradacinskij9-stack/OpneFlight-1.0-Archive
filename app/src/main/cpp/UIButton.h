#pragma once

#include <GLES2/gl2.h>

enum class ButtonId {
    None,
    Camera,
    ZoomIn,
    ZoomOut,
    Reset,
    Pause
};

class UIButton {
public:
    UIButton();

    void setup(ButtonId id,
               float x, float y,
               float size,
               GLuint texture);

    void setTexture(GLuint texture);

    bool containsPoint(float x, float y) const;

    void setPressed(bool pressed);
    bool isPressed() const;

    ButtonId getId() const;
    GLuint getTexture() const;

    float getX() const;
    float getY() const;
    float getSize() const;

private:
    ButtonId id_;
    float x_;
    float y_;
    float size_;
    GLuint texture_;
    bool pressed_;
};