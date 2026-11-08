#include "MeshLoader.h"

#include <android/log.h>
#include <sstream>
#include <string>

#define LOG_TAG "OpenFlight"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

MeshLoader::MeshLoader() {
}

MeshLoader::~MeshLoader() {
}

bool MeshLoader::parseLine(const std::string& line, LoadedMesh& mesh) {
    if (line.empty() || line[0] == '#') {
        return true;
    }

    std::istringstream iss(line);
    std::string token;
    iss >> token;

    if (token == "v") {
        float x, y, z;
        if (iss >> x >> y >> z) {
            mesh.vertices.push_back(x);
            mesh.vertices.push_back(y);
            mesh.vertices.push_back(z);
        }
        return true;
    }

    return true;
}

bool MeshLoader::loadFromAssets(AAssetManager* assetManager,
                                 const char* path,
                                 LoadedMesh& outMesh) {
    if (assetManager == nullptr || path == nullptr) {
        LOGE("MeshLoader: invalid parameters");
        return false;
    }

    AAsset* asset = AAssetManager_open(assetManager, path, AASSET_MODE_BUFFER);
    if (asset == nullptr) {
        LOGE("Failed to open asset: %s", path);
        return false;
    }

    off_t size = AAsset_getLength(asset);
    std::string content;
    content.resize(size);

    int bytesRead = AAsset_read(asset, &content[0], size);
    AAsset_close(asset);

    if (bytesRead <= 0) {
        LOGE("Failed to read asset: %s", path);
        return false;
    }

    content.resize(bytesRead);

    outMesh.vertices.clear();
    outMesh.vertexCount = 0;
    outMesh.floatsPerVertex = 3;

    std::istringstream stream(content);
    std::string line;
    while (std::getline(stream, line)) {
        parseLine(line, outMesh);
    }

    outMesh.vertexCount = static_cast<int>(outMesh.vertices.size() / 3);

    LOGI("Loaded mesh: %s, %d vertices", path, outMesh.vertexCount);
    return outMesh.vertexCount > 0;
}