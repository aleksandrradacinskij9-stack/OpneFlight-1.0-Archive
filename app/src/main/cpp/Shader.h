#pragma once

#include <GLES2/gl2.h>
#include <string>

class Shader {
public:
    Shader();
    ~Shader();

    bool build(const char* vertexSource, const char* fragmentSource);
    void use() const;
    void destroy();

    GLuint program() const;

    GLint getUniformLocation(const char* name) const;
    GLint getAttributeLocation(const char* name) const;

private:
    GLuint compileStage(GLenum type, const char* source);

    GLuint program_;
    GLuint vertexShader_;
    GLuint fragmentShader_;
};