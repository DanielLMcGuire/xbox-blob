#pragma once

#include "raylib.h"
#include "raymath.h"

#include "logo_mesh_gen.h"

class LogoRenderer
{
public:
    void create();
    void unload();

    void advanceTime(float fElapsedTime);

    void render(const Matrix &matOtw, const Camera3D &camera, float fElapsedTime);

private:
    struct GPUMesh
    {
        unsigned int vao = 0, vbo = 0, ebo = 0;
        int indexCount = 0;
    };

    GPUMesh lipMesh, surfaceMesh, surfaceTopMesh, interiorMesh, tmSlashMesh, tmWordmarkMesh, textMesh;
    Texture2D lipTex{}, surfaceTex{}, surfaceTopTex{}, tmTex{};

    Shader unlitShader{}, interiorShader{}, tmShader{}, textShader{};
    int unlitLoc_mvp = -1, unlitLoc_tex0 = -1;
    int interiorLoc_mvp = -1, interiorLoc_gradStart = -1, interiorLoc_gradEnd = -1;
    int interiorLoc_flatStart = -1, interiorLoc_flatEnd = -1, interiorLoc_blendW = -1;
    int tmLoc_mvp = -1, tmLoc_tex0 = -1, tmLoc_fadeAlpha = -1;
    int textLoc_mvp = -1, textLoc_flatColor = -1;

    bool renderText = false;
    float tmAlpha = 0.f;
    Matrix matTextAnim = MatrixIdentity();

    void uploadLogoMesh(GPUMesh &gm, const LogoMesh &mesh);
    void uploadTextMesh(GPUMesh &gm, const TextMesh &mesh);
    void drawMeshRaw(const GPUMesh &gm);
};
