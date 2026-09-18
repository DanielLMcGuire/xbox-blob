#pragma once
#include "raylib.h"
#include "raymath.h"
#include "../util/qrand.h"

class Blob;
class IntroSceneRenderer;

class GreenFog
{
public:
    void create(int seed);
    void unload();

    void restart(int seed);

    void render(const Camera3D &camera, const Blob &blob, IntroSceneRenderer &sceneRenderer,
                float blobIntensity, float fElapsedTime);

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

    static constexpr int INTENSITY_TEX_W = 4096, INTENSITY_TEX_H = 2048;
    unsigned int intensityFBO = 0, intensityColorTex = 0, intensityDepthTex = 0;
    
    unsigned int intensityQuadVAO = 0, intensityQuadVBO = 0;

    Shader occlusionShader{};
    int occLoc_mvp = -1;
    int occLoc_model = -1;
    int occLoc_cameraPos = -1;
    int occLoc_fogRadius = -1;
    int occLoc_cameraRadiusFromBlob = -1;
    int occLoc_targetSize = -1;
    int occLoc_blobNDC = -1;
    int occLoc_posMul = -1;
    int occLoc_isBackdrop = -1;

    float camTheta = 3.14159265359f;
    float camPhi = 0.0f;
    float camRad = 90.0f;

    void renderIntensityTexture(const Camera3D &camera, const Blob &blob, IntroSceneRenderer &sceneRenderer);

    QRand rng;
    int rngSeed = defSeed;
};
