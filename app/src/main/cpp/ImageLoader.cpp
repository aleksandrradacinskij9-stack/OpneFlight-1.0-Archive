#include "ImageLoader.h"

#include <android/log.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define LOG_TAG "OpenFlight"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

GLuint ImageLoader::loadTextureFromAssets(AAssetManager* assetManager,
                                           const char* path) {
    if (assetManager == nullptr || path == nullptr) {
        LOGE("ImageLoader: invalid parameters");
        return 0;
    }

    AAsset* asset = AAssetManager_open(assetManager, path, AASSET_MODE_BUFFER);
    if (asset == nullptr) {
        LOGE("Failed to open image: %s", path);
        return 0;
    }

    off_t size = AAsset_getLength(asset);
    unsigned char* fileData = new unsigned char[size];

    int bytesRead = AAsset_read(asset, fileData, size);
    AAsset_close(asset);

    if (bytesRead <= 0) {
        LOGE("Failed to read image: %s", path);
        delete[] fileData;
        return 0;
    }

    int width = 0;
    int height = 0;
    int channels = 0;

    unsigned char* pixels = stbi_load_from_memory(
        fileData, bytesRead,
        &width, &height, &channels,
        4
    );

    delete[] fileData;

    if (pixels == nullptr) {
        LOGE("Failed to decode image: %s", path);
        return 0;
    }

    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA,
        width,
        height,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        pixels
    );

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D, 0);

    stbi_image_free(pixels);

    LOGI("Loaded texture: %s (%dx%d)", path, width, height);
    return texture;
}