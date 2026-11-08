#pragma once

#include <GLES2/gl2.h>

class Mesh {
public:
    Mesh();
    ~Mesh();

    bool create(const float* vertices, int vertexCount, int floatsPerVertex);
    void draw() const;
    void destroy();

    GLuint getVbo() const;

private:
    GLuint vbo_;
    int vertexCount_;
    int floatsPerVertex_;
};