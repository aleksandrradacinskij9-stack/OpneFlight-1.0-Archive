#include "UIButton.h"

UIButton::UIButton()
    : id_(ButtonId::None)
    , x_(0.0f)
    , y_(0.0f)
    , size_(0.0f)
    , texture_(0)
    , pressed_(false) {
}

void UIButton::setup(ButtonId id,
                     float x, float y,
                     float size,
                     GLuint texture) {
    id_ = id;
    x_ = x;
    y_ = y;
    size_ = size;
    texture_ = texture;
    pressed_ = false;
}

void UIButton::setTexture(GLuint texture) {
    texture_ = texture;
}

bool UIButton::containsPoint(float x, float y) const {
    return x >= x_ && x <= x_ + size_
        && y >= y_ && y <= y_ + size_;
}

void UIButton::setPressed(bool pressed) {
    pressed_ = pressed;
}

bool UIButton::isPressed() const {
    return pressed_;
}

ButtonId UIButton::getId() const {
    return id_;
}

GLuint UIButton::getTexture() const {
    return texture_;
}

float UIButton::getX() const {
    return x_;
}

float UIButton::getY() const {
    return y_;
}

float UIButton::getSize() const {
    return size_;
}