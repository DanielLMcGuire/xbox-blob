#pragma once

#include "raylib.h"

#include "bloblet.h"
#include "blob_bump.h"

#include <vector>
#include <cstdint>

class Blob
{
public:
    Blob() { Init(); Load(); }
    ~Blob() { Unload(); }

    void Init();
    void Load();
    void Unload();
    void Restart();

    void AdvanceTime(float elapsedTime, float dt);
    void Render(const Camera3D& camera, float pulseIntensity, float blobIntensity,
                float baseBlobIntensity, float elapsedTime);

    inline float GetRadius() const { return radius; }
    inline Vector3 GetCenter() const { return position; }

    void GetLightForPosition(Vector3 queryPosition, Vector3* outLightPos, float* outIntensity) const;
    inline float GetLightIntensity() const { return lightIntensity; }

    inline Texture2D GetGlowTexture() const { return glowTexture; }

    Vector4 color{ 0.25f, 1.0f, 0.15f, 1.0f };
    Vector3 position{ 0, 0, 0 };
    Vector3 scale{ 1, 1, 1 };

private:
    float lightIntensity = 0.0f;

    enum { MAX_BLOB_BUMPS = 32 };
    enum { MAX_BLOBLETS = 8 };

    BlobBump blobBumps[MAX_BLOB_BUMPS];
    int numBlobBumps = 0;

    Bloblet bloblets[MAX_BLOBLETS];
    int numBloblets = 0;

    float radius = 2.3f;

    unsigned int blobVAO = 0;
    unsigned int blobStaticVBO = 0;
    unsigned int blobDynamicVBO = 0;
    unsigned int blobEBO = 0;
    unsigned int blobIndexCount = 0;
    int numVertsPerFace = 0;
    std::vector<Vector3> unitSphereNormals;
    std::vector<Vector4> changingVertices;

    unsigned int blobletVAO = 0;
    unsigned int blobletStaticVBO = 0;
    unsigned int blobletEBO = 0;
    unsigned int blobletIndexCount = 0;

    Shader blobShader{};
    int blobLoc_mvp = -1, blobLoc_eyePos = -1, blobLoc_scaling = -1, blobLoc_ooScaling = -1,
        blobLoc_center = -1, blobLoc_baseColor = -1, blobLoc_ambientColor = -1;

    Shader blobletShader{};
    int bloLoc_mvp = -1, bloLoc_eyePos = -1, bloLoc_center = -1, bloLoc_scaleDir = -1,
        bloLoc_scalePerp = -1, bloLoc_scaleDirPMP = -1, bloLoc_baseColor = -1,
        bloLoc_ambientColor = -1, bloLoc_alphaScale = -1;

    Texture2D glowTexture{};

    void BuildCubeSphere(int resolution, std::vector<Vector3>& outPositions,
                          std::vector<uint16_t>& outIndices);
    void UploadStaticMesh(const std::vector<Vector3>& positions, const std::vector<uint16_t>& indices,
                           unsigned int& outVAO, unsigned int& outVBO, unsigned int& outEBO);

    void ZeroChangingVertices();
    void PrepareChangingVertices();

    void BuildGlowTexture();
};

extern Blob* g_Blob;
