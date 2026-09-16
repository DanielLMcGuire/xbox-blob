#pragma once
#include "raylib.h"
#include "raymath.h"
#include "scene_anim.h"
#include "scene_instances.h"
#include "scene_mesh_gen.h"
#include <array>
#include <vector>

class Blob;

class IntroSceneRenderer
{
public:
    void create();
    void unload();

    void advanceTime(float fpos);

    void updateShadows(const Blob &blob);

    void render(const Camera3D &camera, const Blob &blob, bool withShadows);

    void renderAllSilhouettes(const Camera3D &camera, int mvpLoc, int modelLoc, Shader shader);

private:
    struct GPUMesh
    {
        unsigned int vao = 0, vbo = 0, ebo = 0;
        int indexCount = 0;
    };

    SceneAnimTables animTables;
    std::vector<ScenePrimitiveSet> primSets;

    std::array<std::vector<GPUMesh>, pt_NoTypes> meshes;

    Shader phongShader{}, bumpShader{}, shadowShader{};

    struct LitShaderLocs
    {
        int mvp = -1, model = -1, normalMat = -1, lightSpace = -1;
        int eyePos = -1, lightPos = -1, diffuse = -1, specular = -1, ambient = -1;
        int atten = -1, shadowMap = -1, useShadow = -1;
        int normalMap = -1;
    };
    LitShaderLocs phongLocs, bumpLocs;
    int shadowLoc_mvp = -1;

    Texture2D bumpNormalMap{};

    static constexpr int SB_WIDTH = 512, SB_HEIGHT = 512;
    unsigned int shadowFBOHi = 0, shadowFBOLo = 0;
    unsigned int shadowDepthTexHi = 0, shadowDepthTexLo = 0;
    
    Matrix matWTSHi = MatrixIdentity(), matWTSLo = MatrixIdentity();
    Vector3 shadowLightPosHi{}, shadowLightPosLo{};

    void uploadMesh(GPUMesh &gm, const SceneMesh &mesh);
    void drawMeshRaw(const GPUMesh &gm);
    void computeShadowCamera(const Blob &blob, bool hiZ, Matrix &outWTS, Vector3 &outLightPos);
};
