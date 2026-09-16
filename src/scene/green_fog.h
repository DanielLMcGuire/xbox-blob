#pragma once
#include "raylib.h"
#include "raymath.h"
#include "../util/qrand.h"

class Blob;

class GreenFog
{
public:
    void create();
    void unload();

    void restart();

    void render(const Camera3D &camera, const Blob &blob, float blobIntensity, float fElapsedTime);

private:
    unsigned int quadVAO = 0, quadVBO = 0;
    Texture2D plasmaTex[3]{};

    Shader fogShader{};
    int loc_plasmaOffset = -1, loc_plasmaScale = -1;
    int loc_intensityMap = -1, loc_plasmaMap[3] = {-1, -1, -1};
    int loc_useIntensityMap = -1, loc_intensityTint = -1, loc_glowColor = -1;

    unsigned int glowVAO = 0, glowVBO = 0;
    Shader glowShader{};
    int glowLoc_mvp = -1, glowLoc_tex0 = -1, glowLoc_tint = -1;

    QRand rng;
};
