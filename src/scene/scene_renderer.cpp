#include "scene_renderer.h"
#include "scene_data.h"
#include "scene_texgen.h"
#include "../defines.h"
#include "../util/embed.h"
#include "../blob/blob.h"

#include <rlgl.h>
#include "raymath.h"
#include <cmath>

static constexpr Vector3 kMaterialFallback = {0.2079f, 1.0f, 0.100f};
static constexpr Vector3 kLightDiffuse = {0.13f, 0.13f, 0.13f};
static constexpr Vector3 kLightSpecular = {1.0f, 1.0f, 1.0f};
static constexpr float CEIL_Z = 40.0f;
static constexpr float FLOOR_Z = -30.0f;
static constexpr float SHADOW_NEAR = 1.0f;
static constexpr float SHADOW_FAR = 500.0f;
static constexpr float SHADOW_FOVY = PI / 1.5f;

void IntroSceneRenderer::uploadMesh(GPUMesh &gm, const SceneMesh &mesh)
{
    gm.vao = rlLoadVertexArray();
    rlEnableVertexArray(gm.vao);

    gm.vbo = rlLoadVertexBuffer(mesh.vertices.data(), (int)(mesh.vertices.size() * sizeof(SceneVertex)), false);
    rlSetVertexAttribute(0, 3, RL_FLOAT, false, sizeof(SceneVertex), offsetof(SceneVertex, pos));
    rlEnableVertexAttribute(0);
    rlSetVertexAttribute(1, 3, RL_FLOAT, false, sizeof(SceneVertex), offsetof(SceneVertex, tangent));
    rlEnableVertexAttribute(1);
    rlSetVertexAttribute(2, 3, RL_FLOAT, false, sizeof(SceneVertex), offsetof(SceneVertex, bitangent));
    rlEnableVertexAttribute(2);
    rlSetVertexAttribute(3, 3, RL_FLOAT, false, sizeof(SceneVertex), offsetof(SceneVertex, normal));
    rlEnableVertexAttribute(3);
    rlSetVertexAttribute(4, 2, RL_FLOAT, false, sizeof(SceneVertex), offsetof(SceneVertex, u));
    rlEnableVertexAttribute(4);

    gm.ebo = rlLoadVertexBufferElement(mesh.indices.data(), (int)(mesh.indices.size() * sizeof(uint16_t)), false);
    gm.indexCount = (int)mesh.indices.size();

    rlDisableVertexArray();
}

void IntroSceneRenderer::drawMeshRaw(const GPUMesh &gm)
{
    rlEnableVertexArray(gm.vao);
    rlDrawVertexArrayElements(0, gm.indexCount, nullptr);
    rlDisableVertexArray();
}

void IntroSceneRenderer::create()
{
    animTables.create();
    primSets = BuildSceneInstances(animTables);

    Image normalImg = CreateIntensityMapImage(128, true, 1.f / 512.f, 512);
    bumpNormalMap = LoadTextureFromImage(normalImg);
    UnloadImage(normalImg);

#ifdef HAS_EMBED
    #if HAS_EMBED == 2
        #ifdef __EMSCRIPTEN__
            #if !__has_embed("shaders/scene_phong-web.vert") || !__has_embed("shaders/scene_phong-web.frag") || \
                 !__has_embed("shaders/scene_bump-web.vert")  || !__has_embed("shaders/scene_bump-web.frag")  || \
                 !__has_embed("shaders/scene_shadow-web.vert") || !__has_embed("shaders/scene_shadow-web.frag")
                #error FAILED TO FIND WEB SHADERS!
            #endif
        #else
            #if !__has_embed("shaders/scene_phong.vert") || !__has_embed("shaders/scene_phong.frag") || \
                 !__has_embed("shaders/scene_bump.vert")  || !__has_embed("shaders/scene_bump.frag")  || \
                 !__has_embed("shaders/scene_shadow.vert") || !__has_embed("shaders/scene_shadow.frag")
                #error FAILED TO FIND DESKTOP SHADERS!
            #endif
        #endif
    #endif

    #ifdef __clang__
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wc23-extensions"
    #endif

    static constexpr char phongVertData[] = { 
        #ifdef __EMSCRIPTEN__
            #embed "shaders/scene_phong-web.vert"
        #else
            #embed "shaders/scene_phong.vert"
        #endif
        , '\0' 
    };
    static constexpr char phongFragData[] = { 
        #ifdef __EMSCRIPTEN__
            #embed "shaders/scene_phong-web.frag"
        #else
            #embed "shaders/scene_phong.frag"
        #endif
        , '\0' 
    };
    static constexpr char bumpVertData[]  = { 
        #ifdef __EMSCRIPTEN__
            #embed "shaders/scene_bump-web.vert"
        #else
            #embed "shaders/scene_bump.vert"
        #endif
        , '\0' 
    };
    static constexpr char bumpFragData[]  = { 
        #ifdef __EMSCRIPTEN__
            #embed "shaders/scene_bump-web.frag"
        #else
            #embed "shaders/scene_bump.frag"
        #endif
        , '\0' 
    };
    static constexpr char shadowVertData[] = { 
        #ifdef __EMSCRIPTEN__
            #embed "shaders/scene_shadow-web.vert"
        #else
            #embed "shaders/scene_shadow.vert"
        #endif
        , '\0' 
    };
    static constexpr char shadowFragData[] = { 
        #ifdef __EMSCRIPTEN__
            #embed "shaders/scene_shadow-web.frag"
        #else
            #embed "shaders/scene_shadow.frag"
        #endif
        , '\0' 
    };
    #ifdef __clang__
    #pragma clang diagnostic pop
    #endif

    phongShader = LoadShaderFromMemory(phongVertData, phongFragData);
    bumpShader = LoadShaderFromMemory(bumpVertData, bumpFragData);
    shadowShader = LoadShaderFromMemory(shadowVertData, shadowFragData);
#else
    phongShader = LoadShaderFromMemory(
#ifdef __EMSCRIPTEN__
#include "shaders/scene_phong-web.vert.inl"
#else
#include "shaders/scene_phong.vert.inl"
#endif
    ,
#ifdef __EMSCRIPTEN__
#include "shaders/scene_phong-web.frag.inl"
#else
#include "shaders/scene_phong.frag.inl"
#endif
    );
    bumpShader = LoadShaderFromMemory(
#ifdef __EMSCRIPTEN__
#include "shaders/scene_bump-web.vert.inl"
#else
#include "shaders/scene_bump.vert.inl"
#endif
    ,
#ifdef __EMSCRIPTEN__
#include "shaders/scene_bump-web.frag.inl"
#else
#include "shaders/scene_bump.frag.inl"
#endif
    );
    shadowShader = LoadShaderFromMemory(
#ifdef __EMSCRIPTEN__
#include "shaders/scene_shadow-web.vert.inl"
#else
#include "shaders/scene_shadow.vert.inl"
#endif
    ,
#ifdef __EMSCRIPTEN__
#include "shaders/scene_shadow-web.frag.inl"
#else
#include "shaders/scene_shadow.frag.inl"
#endif
    );
#endif

    phongLocs.mvp = GetShaderLocation(phongShader, "mvp");
    phongLocs.model = GetShaderLocation(phongShader, "model");
    phongLocs.normalMat = GetShaderLocation(phongShader, "normalMatrix");
    phongLocs.lightSpace = GetShaderLocation(phongShader, "lightSpaceMatrix");
    phongLocs.eyePos = GetShaderLocation(phongShader, "eyePos");
    phongLocs.lightPos = GetShaderLocation(phongShader, "lightPos");
    phongLocs.diffuse = GetShaderLocation(phongShader, "diffuseColor");
    phongLocs.specular = GetShaderLocation(phongShader, "specularColor");
    phongLocs.ambient = GetShaderLocation(phongShader, "ambientColor");
    phongLocs.atten = GetShaderLocation(phongShader, "attenuation");
    phongLocs.shadowMap = GetShaderLocation(phongShader, "shadowMap");
    phongLocs.useShadow = GetShaderLocation(phongShader, "useShadow");

    bumpLocs.mvp = GetShaderLocation(bumpShader, "mvp");
    bumpLocs.model = GetShaderLocation(bumpShader, "model");
    bumpLocs.normalMat = GetShaderLocation(bumpShader, "normalMatrix");
    bumpLocs.lightSpace = GetShaderLocation(bumpShader, "lightSpaceMatrix");
    bumpLocs.eyePos = GetShaderLocation(bumpShader, "eyePos");
    bumpLocs.lightPos = GetShaderLocation(bumpShader, "lightPos");
    bumpLocs.diffuse = GetShaderLocation(bumpShader, "diffuseColor");
    bumpLocs.specular = GetShaderLocation(bumpShader, "specularColor");
    bumpLocs.ambient = GetShaderLocation(bumpShader, "ambientColor");
    bumpLocs.atten = GetShaderLocation(bumpShader, "attenuation");
    bumpLocs.normalMap = GetShaderLocation(bumpShader, "normalMap");
    bumpLocs.shadowMap = GetShaderLocation(bumpShader, "shadowMap");
    bumpLocs.useShadow = GetShaderLocation(bumpShader, "useShadow");

    shadowLoc_mvp = GetShaderLocation(shadowShader, "mvp");

    meshes[pt_Sphere].resize(std::size(theSphereVers));
    for (size_t i = 0; i < std::size(theSphereVers); i++)
        uploadMesh(meshes[pt_Sphere][i], CreateSphereMesh(theSphereVers[i], true, 0));

    meshes[pt_Cylinder].resize(std::size(theCylinderVers));
    for (size_t i = 0; i < std::size(theCylinderVers); i++)
        uploadMesh(meshes[pt_Cylinder][i], CreateCylinderMesh(theCylinderVers[i], false, 0));

    meshes[pt_Cone].resize(std::size(theConeVers));
    for (size_t i = 0; i < std::size(theConeVers); i++)
        uploadMesh(meshes[pt_Cone][i], CreateConeMesh(theConeVers[i], false, 0));

    meshes[pt_Box].resize(1);
    uploadMesh(meshes[pt_Box][0], CreateBoxMesh(false));

    meshes[pt_Torus].resize(std::size(theTorusVers));
    for (size_t i = 0; i < std::size(theTorusVers); i++)
        uploadMesh(meshes[pt_Torus][i], CreateTorusMesh(theTorusVers[i], false, 0));

    meshes[pt_SurfOfRev].resize(std::size(theSurfOfRevVers));
    for (size_t i = 0; i < std::size(theSurfOfRevVers); i++)
        uploadMesh(meshes[pt_SurfOfRev][i], CreateSurfOfRevMesh(theSurfOfRevVers[i], true, 0));

    auto makeShadowFBO = [](unsigned int &fbo, unsigned int &depthTex)
    {
        depthTex = rlLoadTextureDepth(SB_WIDTH, SB_HEIGHT, false);
        unsigned int colorTex = rlLoadTexture(nullptr, SB_WIDTH, SB_HEIGHT, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, 1);
        fbo = rlLoadFramebuffer();
        rlEnableFramebuffer(fbo);
        rlFramebufferAttach(fbo, colorTex, RL_ATTACHMENT_COLOR_CHANNEL0, RL_ATTACHMENT_TEXTURE2D, 0);
        rlFramebufferAttach(fbo, depthTex, RL_ATTACHMENT_DEPTH, RL_ATTACHMENT_TEXTURE2D, 0);
        rlFramebufferComplete(fbo);
        rlDisableFramebuffer();
    };
    makeShadowFBO(shadowFBOHi, shadowDepthTexHi);
    makeShadowFBO(shadowFBOLo, shadowDepthTexLo);
}

void IntroSceneRenderer::unload()
{
    UnloadShader(phongShader);
    UnloadShader(bumpShader);
    UnloadShader(shadowShader);
    UnloadTexture(bumpNormalMap);
}

void IntroSceneRenderer::advanceTime(float fpos)
{
    animTables.advanceTime(fpos);
}

void IntroSceneRenderer::computeShadowCamera(const Blob &blob, bool hiZ, Matrix &outWTS, Vector3 &outLightPos)
{
    Vector3 queryPos = {0.0f, 0.0f, hiZ ? CEIL_Z : FLOOR_Z};
    Vector3 lightPos;
    float unusedIntensity;
    blob.GetLightForPosition(queryPos, &lightPos, &unusedIntensity);

    Vector3 dir = {0.0f, 0.0f, hiZ ? 1.0f : -1.0f};
    Vector3 target = Vector3Add(lightPos, dir);
    Vector3 up = {0.0f, hiZ ? -1.0f : 1.0f, 0.0f};

    Matrix view = MatrixLookAt(lightPos, target, up);
    Matrix proj = MatrixPerspective(SHADOW_FOVY, 1.0f, SHADOW_NEAR, SHADOW_FAR);
    outWTS = MatrixMultiply(view, proj);
    outLightPos = lightPos;
}

void IntroSceneRenderer::updateShadows(const Blob &blob)
{
    computeShadowCamera(blob, true, matWTSHi, shadowLightPosHi);
    computeShadowCamera(blob, false, matWTSLo, shadowLightPosLo);

    static constexpr PrimitiveTypes kCasterTypes[] = {pt_Torus, pt_Cone, pt_Box, pt_Cylinder};

    struct Pass { unsigned int fbo; const Matrix *wts; bool hiZ; };
    Pass passes[2] = {
        {shadowFBOHi, &matWTSHi, true},
        {shadowFBOLo, &matWTSLo, false},
    };

    for (auto &pass : passes)
    {
        rlEnableFramebuffer(pass.fbo);
        rlViewport(0, 0, SB_WIDTH, SB_HEIGHT);
        rlClearScreenBuffers();

        BeginShaderMode(shadowShader);
        for (PrimitiveTypes type : kCasterTypes)
        {
            for (const auto &inst : primSets[type].instances)
            {
                if (inst.bHiZ != pass.hiZ) continue;
                Matrix model = inst.worldMatrix(animTables);
                Matrix mvp = MatrixMultiply(model, *pass.wts);
                SetShaderValueMatrix(shadowShader, shadowLoc_mvp, mvp);
                drawMeshRaw(meshes[type][inst.idxVersion]);
            }
        }
        EndShaderMode();

        rlDisableFramebuffer();
    }
    rlViewport(0, 0, GetScreenWidth(), GetScreenHeight());
}

void IntroSceneRenderer::renderAllSilhouettes(const Camera3D &camera, int mvpLoc, int modelLoc, Shader shader)
{
    Matrix view = GetCameraMatrix(camera);
    float aspect = (float)GetScreenWidth() / (float)GetScreenHeight();
    Matrix proj = MatrixPerspective(camera.fovy * DEG2RAD, aspect, 0.4f, 800.0f);
    Matrix viewProj = MatrixMultiply(view, proj);

    for (PrimitiveTypes type = (PrimitiveTypes)0; type < pt_NoTypes; type = (PrimitiveTypes)(type + 1))
    {
        for (const auto &inst : primSets[type].instances)
        {
            Matrix model = inst.worldMatrix(animTables);
            Matrix mvp = MatrixMultiply(model, viewProj);
            SetShaderValueMatrix(shader, mvpLoc, mvp);
            if (modelLoc >= 0) SetShaderValueMatrix(shader, modelLoc, model);
            drawMeshRaw(meshes[type][inst.idxVersion]);
        }
    }
}

void IntroSceneRenderer::renderFixedLight(const Matrix &view, const Matrix &proj, Vector3 eyePos, Vector3 lightPos,
                                          Vector3 tint)
{
    Matrix viewProj = MatrixMultiply(view, proj);

    Vector3 material = kMaterialFallback;
    if (Vector3Length(tint) > 0.0001f) material = tint;

    Vector3 diffuseColor = Vector3Multiply(material, kLightDiffuse);
    Vector3 specularColor = Vector3Multiply(material, kLightSpecular);
    Vector3 ambientColor = {0, 0, 0};
    Vector3 atten = {1.0f, 0.001f, 0.001f};

    for (PrimitiveTypes type = (PrimitiveTypes)0; type < pt_NoTypes; type = (PrimitiveTypes)(type + 1))
    {
        bool bump = (type == pt_Sphere || type == pt_SurfOfRev);
        Shader &shader = bump ? bumpShader : phongShader;
        const LitShaderLocs &locs = bump ? bumpLocs : phongLocs;

        BeginShaderMode(shader);
        SetShaderValue(shader, locs.eyePos, &eyePos, SHADER_UNIFORM_VEC3);
        SetShaderValue(shader, locs.diffuse, &diffuseColor, SHADER_UNIFORM_VEC3);
        SetShaderValue(shader, locs.specular, &specularColor, SHADER_UNIFORM_VEC3);
        SetShaderValue(shader, locs.ambient, &ambientColor, SHADER_UNIFORM_VEC3);
        SetShaderValue(shader, locs.atten, &atten, SHADER_UNIFORM_VEC3);
        
        int useShadowFlag = 0;
        SetShaderValue(shader, locs.useShadow, &useShadowFlag, SHADER_UNIFORM_INT);
        SetShaderValue(shader, locs.lightPos, &lightPos, SHADER_UNIFORM_VEC3);
        
        if (bump)
            SetShaderValueTexture(shader, locs.normalMap, bumpNormalMap);

        for (const auto &inst : primSets[type].instances)
        {
            Matrix model = inst.worldMatrix(animTables);
            Matrix mvp = MatrixMultiply(model, viewProj);
            Matrix normalMat = MatrixTranspose(MatrixInvert(model));

            SetShaderValueMatrix(shader, locs.mvp, mvp);
            SetShaderValueMatrix(shader, locs.model, model);
            SetShaderValueMatrix(shader, locs.normalMat, normalMat);

            drawMeshRaw(meshes[type][inst.idxVersion]);
        }
        EndShaderMode();
    }
}

void IntroSceneRenderer::render(const Camera3D &camera, const Blob &blob, bool withShadows)
{
    Matrix view = GetCameraMatrix(camera);
    float aspect = (float)GetScreenWidth() / (float)GetScreenHeight();
    Matrix proj = MatrixPerspective(camera.fovy * DEG2RAD, aspect, 0.4f, 800.0f);
    Matrix viewProj = MatrixMultiply(view, proj);

    Vector3 material = kMaterialFallback;
    Vector3 blobTint = {blob.color.x, blob.color.y, blob.color.z};
    if (Vector3Length(blobTint) > 0.0001f) material = blobTint;

    float intensity = std::max(0.0f, blob.GetLightIntensity()) * 2.0f;
    Vector3 diffuseColor = Vector3Scale(Vector3Multiply(material, kLightDiffuse), intensity);
    Vector3 specularColor = Vector3Scale(Vector3Multiply(material, kLightSpecular), intensity);
    Vector3 ambientColor = {0, 0, 0};
    float ooIntensity = 1.0f / std::max(intensity, 0.0001f);
    Vector3 atten = {1.0f, 0.001f * ooIntensity, 0.001f * ooIntensity};

    for (PrimitiveTypes type = (PrimitiveTypes)0; type < pt_NoTypes; type = (PrimitiveTypes)(type + 1))
    {
        bool bump = (type == pt_Sphere || type == pt_SurfOfRev);
        Shader &shader = bump ? bumpShader : phongShader;
        const LitShaderLocs &locs = bump ? bumpLocs : phongLocs;

        BeginShaderMode(shader);
        SetShaderValue(shader, locs.eyePos, &camera.position, SHADER_UNIFORM_VEC3);
        SetShaderValue(shader, locs.diffuse, &diffuseColor, SHADER_UNIFORM_VEC3);
        SetShaderValue(shader, locs.specular, &specularColor, SHADER_UNIFORM_VEC3);
        SetShaderValue(shader, locs.ambient, &ambientColor, SHADER_UNIFORM_VEC3);
        SetShaderValue(shader, locs.atten, &atten, SHADER_UNIFORM_VEC3);
        int useShadowFlag = withShadows ? 1 : 0;
        SetShaderValue(shader, locs.useShadow, &useShadowFlag, SHADER_UNIFORM_INT);
        if (bump)
            SetShaderValueTexture(shader, locs.normalMap, bumpNormalMap);

        for (const auto &inst : primSets[type].instances)
        {
            Matrix model = inst.worldMatrix(animTables);
            Matrix mvp = MatrixMultiply(model, viewProj);
            Matrix normalMat = MatrixTranspose(MatrixInvert(model));

            Vector3 instWorldPos = {model.m12, model.m13, model.m14};
            Vector3 lightPos;
            float unusedIntensity;
            blob.GetLightForPosition(instWorldPos, &lightPos, &unusedIntensity);

            SetShaderValueMatrix(shader, locs.mvp, mvp);
            SetShaderValueMatrix(shader, locs.model, model);
            SetShaderValueMatrix(shader, locs.normalMat, normalMat);
            SetShaderValue(shader, locs.lightPos, &lightPos, SHADER_UNIFORM_VEC3);

            if (withShadows)
            {
                const Matrix &wts = inst.bHiZ ? matWTSHi : matWTSLo;
                Texture2D shadowTex{};
                shadowTex.id = inst.bHiZ ? shadowDepthTexHi : shadowDepthTexLo;
                shadowTex.width = SB_WIDTH;
                shadowTex.height = SB_HEIGHT;
                shadowTex.mipmaps = 1;
                shadowTex.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
                SetShaderValueMatrix(shader, locs.lightSpace, wts);
                SetShaderValueTexture(shader, locs.shadowMap, shadowTex);
            }

            drawMeshRaw(meshes[type][inst.idxVersion]);
        }
        EndShaderMode();
    }
}
