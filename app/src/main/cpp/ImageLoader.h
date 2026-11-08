#pragma once

#include <android/asset_manager.h>
#include <GLES2/gl2.h>

class ImageLoader {
public:
    static GLuint loadTextureFromAssets(AAssetManager* assetManager,
                                        const char* path);
};