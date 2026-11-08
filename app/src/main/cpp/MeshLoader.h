#pragma once

#include <android/asset_manager.h>
#include <GLES2/gl2.h>
#include <vector>

struct LoadedMesh {
    std::vector<float> vertices;
    int vertexCount;
    int floatsPerVertex;
};

class MeshLoader {
public:
    MeshLoader();
    ~MeshLoader();

    bool loadFromAssets(AAssetManager* assetManager,
                        const char* path,
                        LoadedMesh& outMesh);

private:
    bool parseLine(const std::string& line, LoadedMesh& mesh);
};