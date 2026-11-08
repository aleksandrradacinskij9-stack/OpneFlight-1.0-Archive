#include "Mesh.h"
#include <android/log.h>

#define LOG_TAG "OpenFlight"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

Mesh::Mesh() : vbo_(0), vertexCount_(0), floatsPerVertex_(0) {}
Mesh::~Mesh() { destroy(); }

bool Mesh::create(const float* vertices, int vertexCount, int floatsPerVertex) {
    if (vertices == nullptr || vertexCount <= 0 || floatsPerVertex <= 0) return false;
    vertexCount_ = vertexCount; floatsPerVertex_ = floatsPerVertex;
    glGenBuffers(1, &vbo_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertexCount_ * floatsPerVertex_, vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    return true;
}

void Mesh::draw() const {
    if (vbo_ == 0) return;
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);

    if (floatsPerVertex_ == 8) {
        // 3D объекты (Позиция, Текстурные координаты, Нормали)
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 8, nullptr);
        
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 8, (void*)(sizeof(float) * 3));
        
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 8, (void*)(sizeof(float) * 5));

        glDrawArrays(GL_TRIANGLES, 0, vertexCount_);

        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
        glDisableVertexAttribArray(2);
    } else {
        // UI объекты (Только Позиция)
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, floatsPerVertex_, GL_FLOAT, GL_FALSE, sizeof(float) * floatsPerVertex_, nullptr);

        glDrawArrays(GL_TRIANGLES, 0, vertexCount_);

        glDisableVertexAttribArray(0);
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Mesh::destroy() {
    if (vbo_ != 0) { glDeleteBuffers(1, &vbo_); vbo_ = 0; }
    vertexCount_ = 0; floatsPerVertex_ = 0;
}

GLuint Mesh::getVbo() const { return vbo_; }