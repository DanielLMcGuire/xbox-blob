#pragma once

#include "raylib.h"
#include "shield_config.h"
#include "shield_motion.h"

class Blob;

enum class ShieldPass
{
    FarSide,
    NearSide,
};

class ShieldManager
{
public:
    ShieldManager() = default;
    ~ShieldManager() { unload(); }
    ShieldManager(const ShieldManager &) = delete;
    ShieldManager &operator=(const ShieldManager &) = delete;

    void create(int seed);
    void unload();

    bool isReady() const { return ready; }

    void restart();

    void render(const Camera3D &camera, const Blob &blob, float blobIntensity, float elapsedTime, ShieldPass pass);

    static float ShadingAt(float elapsedTime);

private:
    struct GpuMesh
    {
        unsigned int vao = 0, vbo = 0, ebo = 0;
        int indexCount = 0;

        void upload(const void *vertices, int vertexBytes, const uint16_t *indices, int count);
        void bind() const;
        void unbind() const;
        void unload();
    };

    bool ready = false;
    int seed = defSeed;

    GpuMesh solidMesh;
    GpuMesh bandMesh;
    int bandIndexOffset[ShieldConfig::BAND_SHIELD_COUNT] = {};
    int bandIndexCount[ShieldConfig::BAND_SHIELD_COUNT] = {};

    SolidShieldMotion solids[ShieldConfig::SOLID_SHIELD_COUNT];
    BandShieldMotion bands[ShieldConfig::BAND_SHIELD_COUNT];

    Shader shader{};
    int loc_mvp = -1, loc_model = -1, loc_eyePos = -1, loc_blobLightPos = -1, loc_moodLightPos = -1;
    int loc_shading = -1, loc_blobIntensity = -1, loc_blobSpecColor = -1;
};
