#include "Shader.h"
#include <android/log.h>

#define LOG_TAG "OpenFlight"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

Shader::Shader()
    : program_(0), vertexShader_(0), fragmentShader_(0) {}

Shader::~Shader() { destroy(); }

GLuint Shader::compileStage(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    if (shader == 0) return 0;

    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint compiled = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_FALSE) {
        GLint logLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
        if (logLength > 0) {
            std::string log;
            log.resize(logLength);
            glGetShaderInfoLog(shader, logLength, nullptr, log.data());
            LOGE("Shader compile error: %s", log.c_str());
        }
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

bool Shader::build(const char* vertexSource, const char* fragmentSource) {
    vertexShader_ = compileStage(GL_VERTEX_SHADER, vertexSource);
    if (vertexShader_ == 0) return false;

    fragmentShader_ = compileStage(GL_FRAGMENT_SHADER, fragmentSource);
    if (fragmentShader_ == 0) {
        glDeleteShader(vertexShader_);
        vertexShader_ = 0;
        return false;
    }

    program_ = glCreateProgram();
    if (program_ == 0) return false;

    glAttachShader(program_, vertexShader_);
    glAttachShader(program_, fragmentShader_);

    // ВАЖНО: Привязываем все возможные атрибуты
    glBindAttribLocation(program_, 0, "aPosition");
    glBindAttribLocation(program_, 1, "aTexCoord");
    glBindAttribLocation(program_, 2, "aNormal");

    glLinkProgram(program_);

    GLint linked = 0;
    glGetProgramiv(program_, GL_LINK_STATUS, &linked);
    if (linked == GL_FALSE) {
        GLint logLength = 0;
        glGetProgramiv(program_, GL_INFO_LOG_LENGTH, &logLength);
        if (logLength > 0) {
            std::string log;
            log.resize(logLength);
            glGetProgramInfoLog(program_, logLength, nullptr, log.data());
            LOGE("Program link error: %s", log.c_str());
        }
        glDeleteProgram(program_);
        program_ = 0;
        return false;
    }

    return true;
}

void Shader::use() const { if (program_ != 0) glUseProgram(program_); }

void Shader::destroy() {
    if (program_ != 0) { glDeleteProgram(program_); program_ = 0; }
    if (vertexShader_ != 0) { glDeleteShader(vertexShader_); vertexShader_ = 0; }
    if (fragmentShader_ != 0) { glDeleteShader(fragmentShader_); fragmentShader_ = 0; }
}

GLuint Shader::program() const { return program_; }
GLint Shader::getUniformLocation(const char* name) const { return glGetUniformLocation(program_, name); }
GLint Shader::getAttributeLocation(const char* name) const { return glGetAttribLocation(program_, name); }