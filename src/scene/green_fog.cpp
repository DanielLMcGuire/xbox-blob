#include "green_fog.h"

#include <rlgl.h>

#include "../defines.h"

#include "scene_texgen.h"
#include "scene_renderer.h"
#include "../blob/blob.h"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace
{
constexpr int PLASMA_SIZE = 512;
constexpr float MUL_SCALE = 0.005f;

struct QuadVertex
{
    Vector3 pos;
    float u0, v0;
    float u1, v1;
};

struct GlowVertex
{
    Vector3 pos;
    float u, v;
};

Vector3 ProjectToNDC(Vector3 p, const Matrix &m)
{
    float x = m.m0 * p.x + m.m4 * p.y + m.m8 * p.z + m.m12;
    float y = m.m1 * p.x + m.m5 * p.y + m.m9 * p.z + m.m13;
    float z = m.m2 * p.x + m.m6 * p.y + m.m10 * p.z + m.m14;
    float w = m.m3 * p.x + m.m7 * p.y + m.m11 * p.z + m.m15;
    if (fabsf(w) < 0.0001f) w = 0.0001f;
    return {x / w, y / w, z / w};
}
} // namespace

void GreenFog::create(int seed)
{
    QuadVertex verts[4];
    verts[0].pos = {-1.0f, -1.0f, 0.0f};
    verts[1].pos = {-1.0f, +1.0f, 0.0f};
    verts[2].pos = {+1.0f, +1.0f, 0.0f};
    verts[3].pos = {+1.0f, -1.0f, 0.0f};

    verts[0].u0 = 0.0f; verts[0].v0 = 0.0f;
    verts[1].u0 = 0.0f; verts[1].v0 = 1.0f;
    verts[2].u0 = 1.0f; verts[2].v0 = 1.0f;
    verts[3].u0 = 1.0f; verts[3].v0 = 0.0f;

    for (auto &v : verts)
    {
        v.v1 = -v.pos.x * 640.0f / (float)PLASMA_SIZE;
        v.u1 =  v.pos.y * 480.0f / (float)PLASMA_SIZE;
    }
    static const uint16_t quadIndices[6] = {0, 1, 2, 0, 2, 3};

    quadVAO = rlLoadVertexArray();
    rlEnableVertexArray(quadVAO);
    quadVBO = rlLoadVertexBuffer(verts, sizeof(verts), false);
    rlSetVertexAttribute(0, 3, RL_FLOAT, false, sizeof(QuadVertex), offsetof(QuadVertex, pos));
    rlEnableVertexAttribute(0);
    rlSetVertexAttribute(1, 2, RL_FLOAT, false, sizeof(QuadVertex), offsetof(QuadVertex, u0));
    rlEnableVertexAttribute(1);
    rlSetVertexAttribute(2, 2, RL_FLOAT, false, sizeof(QuadVertex), offsetof(QuadVertex, u1));
    rlEnableVertexAttribute(2);
    rlLoadVertexBufferElement(quadIndices, sizeof(quadIndices), false);
    rlDisableVertexArray();

    GlowVertex gverts[4] = {
        {{-1.0f, -1.0f, 0.0f}, 0.0f, 0.0f},
        {{-1.0f, +1.0f, 0.0f}, 0.0f, 1.0f},
        {{+1.0f, +1.0f, 0.0f}, 1.0f, 1.0f},
        {{+1.0f, -1.0f, 0.0f}, 1.0f, 0.0f}
    };
    glowVAO = rlLoadVertexArray();
    rlEnableVertexArray(glowVAO);
    glowVBO = rlLoadVertexBuffer(gverts, sizeof(gverts), false);
    rlSetVertexAttribute(0, 3, RL_FLOAT, false, sizeof(GlowVertex), offsetof(GlowVertex, pos));
    rlEnableVertexAttribute(0);
    rlSetVertexAttribute(1, 2, RL_FLOAT, false, sizeof(GlowVertex), offsetof(GlowVertex, u));
    rlEnableVertexAttribute(1);
    rlLoadVertexBufferElement(quadIndices, sizeof(quadIndices), false);
    rlDisableVertexArray();

    GlowVertex iverts[4] = {
        {{-1.0f, -1.0f, 0.0f}, 0.0f, 0.0f},
        {{-1.0f, +1.0f, 0.0f}, 0.0f, 1.0f},
        {{+1.0f, +1.0f, 0.0f}, 1.0f, 1.0f},
        {{+1.0f, -1.0f, 0.0f}, 1.0f, 0.0f}
    };
    intensityQuadVAO = rlLoadVertexArray();
    rlEnableVertexArray(intensityQuadVAO);
    intensityQuadVBO = rlLoadVertexBuffer(iverts, sizeof(iverts), false);
    rlSetVertexAttribute(0, 3, RL_FLOAT, false, sizeof(GlowVertex), offsetof(GlowVertex, pos));
    rlEnableVertexAttribute(0);
    rlSetVertexAttribute(1, 2, RL_FLOAT, false, sizeof(GlowVertex), offsetof(GlowVertex, u));
    rlEnableVertexAttribute(1);
    rlLoadVertexBufferElement(quadIndices, sizeof(quadIndices), false);
    rlDisableVertexArray();

#ifdef HAS_EMBED
    #ifdef __clang__
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wc23-extensions"
    #endif
    static constexpr char fogVertData[] = { 
        #embed "shaders/greenfog.vert" 
        , '\0' };
    static constexpr char fogFragData[] = { 
        #embed "shaders/greenfog.frag" 
        , '\0' };
    static constexpr char glowVertData[] = { 
        #embed "shaders/glow.vert" 
        , '\0' };
    static constexpr char glowFragData[] = { 
        #embed "shaders/glow.frag" 
        , '\0' };
    #ifdef __clang__
    #pragma clang diagnostic pop
    #endif
    fogShader = LoadShaderFromMemory(fogVertData, fogFragData);
    glowShader = LoadShaderFromMemory(glowVertData, glowFragData);
#else
    fogShader = LoadShaderFromMemory(
#include "shaders/greenfog.vert.inl"
    ,
#include "shaders/greenfog.frag.inl"
    );
    glowShader = LoadShaderFromMemory(
#include "shaders/glow.vert.inl"
    ,
#include "shaders/glow.frag.inl"
    );
#endif

#ifdef HAS_EMBED
    #ifdef __clang__
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wc23-extensions"
    #endif
    static constexpr char occVertData[] = { 
        #embed "shaders/occlusion.vert" 
        , '\0' };
    static constexpr char occFragData[] = { 
        #embed "shaders/occlusion.frag" 
        , '\0' };
    #ifdef __clang__
    #pragma clang diagnostic pop
    #endif
    occlusionShader = LoadShaderFromMemory(occVertData, occFragData);
#else
    occlusionShader = LoadShaderFromMemory(
#include "shaders/occlusion.vert.inl"
    ,
#include "shaders/occlusion.frag.inl"
    );
#endif

    loc_plasmaOffset = GetShaderLocation(fogShader, "plasmaOffset");
    loc_plasmaScale = GetShaderLocation(fogShader, "plasmaScale");
    loc_intensityMap = GetShaderLocation(fogShader, "intensityMap");
    loc_plasmaMap[0] = GetShaderLocation(fogShader, "plasmaMap0");
    loc_plasmaMap[1] = GetShaderLocation(fogShader, "plasmaMap1");
    loc_plasmaMap[2] = GetShaderLocation(fogShader, "plasmaMap2");
    loc_useIntensityMap = GetShaderLocation(fogShader, "useIntensityMap");
    loc_intensityTint = GetShaderLocation(fogShader, "intensityTint");
    loc_glowColor = GetShaderLocation(fogShader, "glowColor");

    glowLoc_mvp = GetShaderLocation(glowShader, "mvp");
    glowLoc_tex0 = GetShaderLocation(glowShader, "tex0");
    glowLoc_tint = GetShaderLocation(glowShader, "tint");

    occLoc_mvp = GetShaderLocation(occlusionShader, "mvp");
    occLoc_model = GetShaderLocation(occlusionShader, "model");
    occLoc_cameraPos = GetShaderLocation(occlusionShader, "cameraPos");
    occLoc_fogRadius = GetShaderLocation(occlusionShader, "fogRadius");
    occLoc_cameraRadiusFromBlob = GetShaderLocation(occlusionShader, "cameraRadiusFromBlob");
    occLoc_targetSize = GetShaderLocation(occlusionShader, "targetSize");
    occLoc_blobNDC = GetShaderLocation(occlusionShader, "blobNDC");
    occLoc_posMul = GetShaderLocation(occlusionShader, "posMul");
    occLoc_isBackdrop = GetShaderLocation(occlusionShader, "isBackdrop");

    intensityColorTex = rlLoadTexture(nullptr, INTENSITY_TEX_W, INTENSITY_TEX_H, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, 1);
    rlTextureParameters(intensityColorTex, RL_TEXTURE_WRAP_S, RL_TEXTURE_WRAP_CLAMP);
    rlTextureParameters(intensityColorTex, RL_TEXTURE_WRAP_T, RL_TEXTURE_WRAP_CLAMP);
    rlTextureParameters(intensityColorTex, RL_TEXTURE_MAG_FILTER, RL_TEXTURE_FILTER_LINEAR);
    rlTextureParameters(intensityColorTex, RL_TEXTURE_MIN_FILTER, RL_TEXTURE_FILTER_LINEAR);

    intensityDepthTex = rlLoadTextureDepth(INTENSITY_TEX_W, INTENSITY_TEX_H, true);
    intensityFBO = rlLoadFramebuffer();
    rlEnableFramebuffer(intensityFBO);
    rlFramebufferAttach(intensityFBO, intensityColorTex, RL_ATTACHMENT_COLOR_CHANNEL0, RL_ATTACHMENT_TEXTURE2D, 0);
    rlFramebufferAttach(intensityFBO, intensityDepthTex, RL_ATTACHMENT_DEPTH, RL_ATTACHMENT_RENDERBUFFER, 0);
    rlFramebufferComplete(intensityFBO);
    rlDisableFramebuffer();

    rng.SetSeed(seed);
    camTheta = 3.14159265359f;
    camPhi = 0.0f;
    camRad = 90.0f;

    for (auto &t : plasmaTex) UnloadTexture(t);
    int tffonp = 255 / 3;
    int s = rng.Rand();
    auto images = CreatePlasmaMapImages(3, PLASMA_SIZE, 5 * tffonp, s, 0, 255 / 3);
    for (int i = 0; i < 3; i++)
    {
        plasmaTex[i] = LoadTextureFromImage(images[i]);
        SetTextureWrap(plasmaTex[i], TEXTURE_WRAP_REPEAT);
        rlTextureParameters(plasmaTex[i].id, RL_TEXTURE_MAG_FILTER, RL_TEXTURE_FILTER_LINEAR);
        rlTextureParameters(plasmaTex[i].id, RL_TEXTURE_MIN_FILTER, RL_TEXTURE_FILTER_LINEAR);
        UnloadImage(images[i]);
    }
}

void GreenFog::unload()
{
    UnloadShader(fogShader);
    UnloadShader(glowShader);
    UnloadShader(occlusionShader);
    for (auto &t : plasmaTex) UnloadTexture(t);
    rlUnloadVertexArray(quadVAO);
    rlUnloadVertexBuffer(quadVBO);
    rlUnloadVertexArray(glowVAO);
    rlUnloadVertexBuffer(glowVBO);
    rlUnloadVertexArray(intensityQuadVAO);
    rlUnloadVertexBuffer(intensityQuadVBO);
}

void GreenFog::renderIntensityTexture(const Camera3D &camera, const Blob &blob, IntroSceneRenderer &sceneRenderer)
{
    constexpr float MAIN_FOG_RAD = 40.0f;
    float cameraRadiusFromBlob = Vector3Distance(camera.position, blob.position);
    
    float camAspect = (float)GetScreenHeight() / (float)GetScreenWidth();
    float projAspect = (float)GetScreenWidth() / (float)GetScreenHeight();

    Matrix view = GetCameraMatrix(camera);
    Matrix proj = MatrixPerspective(camera.fovy * DEG2RAD, projAspect, 0.4f, 800.0f);
    Matrix viewProj = MatrixMultiply(view, proj);
    Vector3 blobNDC3 = ProjectToNDC(blob.position, viewProj);

    Vector2 blobNDC = {blobNDC3.x, blobNDC3.y};
    Vector2 targetSize = {(float)INTENSITY_TEX_W, (float)INTENSITY_TEX_H};
    Vector2 posMul = {
        cameraRadiusFromBlob * MUL_SCALE,
        cameraRadiusFromBlob * MUL_SCALE * camAspect
    };

    rlEnableFramebuffer(intensityFBO);
    rlViewport(0, 0, INTENSITY_TEX_W, INTENSITY_TEX_H);
    
    rlClearColor(255, 0, 0, 255); 
    rlClearScreenBuffers();

    BeginShaderMode(occlusionShader);
    SetShaderValue(occlusionShader, occLoc_cameraPos, &camera.position, SHADER_UNIFORM_VEC3);
    SetShaderValue(occlusionShader, occLoc_fogRadius, &MAIN_FOG_RAD, SHADER_UNIFORM_FLOAT);
    SetShaderValue(occlusionShader, occLoc_cameraRadiusFromBlob, &cameraRadiusFromBlob, SHADER_UNIFORM_FLOAT);
    SetShaderValue(occlusionShader, occLoc_targetSize, &targetSize, SHADER_UNIFORM_VEC2);
    SetShaderValue(occlusionShader, occLoc_blobNDC, &blobNDC, SHADER_UNIFORM_VEC2);
    SetShaderValue(occlusionShader, occLoc_posMul, &posMul, SHADER_UNIFORM_VEC2);

    int isBackdrop = 1;
    SetShaderValue(occlusionShader, occLoc_isBackdrop, &isBackdrop, SHADER_UNIFORM_INT);
    rlDisableDepthTest();
    rlDisableDepthMask();
    rlEnableVertexArray(intensityQuadVAO);
    rlDrawVertexArrayElements(0, 6, nullptr);
    rlDisableVertexArray();

    isBackdrop = 0;
    SetShaderValue(occlusionShader, occLoc_isBackdrop, &isBackdrop, SHADER_UNIFORM_INT);
    rlEnableDepthTest();
    rlEnableDepthMask();
    
    EndShaderMode();

    rlDisableFramebuffer();
    rlViewport(0, 0, GetScreenWidth(), GetScreenHeight());
}

void GreenFog::render(const Camera3D &camera, const Blob &blob, IntroSceneRenderer &sceneRenderer,
                       float blobIntensity, float fElapsedTime)
{
    renderIntensityTexture(camera, blob, sceneRenderer);

    float camAspect = (float)GetScreenHeight() / (float)GetScreenWidth();
    float projAspect = (float)GetScreenWidth() / (float)GetScreenHeight();

    Matrix view = GetCameraMatrix(camera);
    Matrix proj = MatrixPerspective(camera.fovy * DEG2RAD, projAspect, 0.4f, 800.0f);
    Matrix viewProj = MatrixMultiply(view, proj);

    Vector3 rel = Vector3Subtract(camera.position, blob.position);
    float r_xyz = Vector3Length(rel);
    if (r_xyz < 0.0001f) r_xyz = 0.0001f;

    camRad = r_xyz;
    camPhi = asinf(std::max(-1.0f, std::min(1.0f, rel.z / r_xyz)));

    float oldTheta = camTheta;
    camTheta = atan2f(rel.y, rel.x);

    if (fabsf(oldTheta) < 1000.0f && fabsf(camTheta - oldTheta) > 3.14159265359f)
    {
        while (oldTheta - 3.14159265359f > camTheta) camTheta += 2.0f * 3.14159265359f;
        while (oldTheta + 3.14159265359f < camTheta) camTheta -= 2.0f * 3.14159265359f;
    }

    Vector3 originScr = ProjectToNDC(blob.position, viewProj);

    float intensity = std::max(0.0f, blobIntensity * 0.7f - 0.1f);
    Vector3 intensityTint = {0.0f, intensity, 0.0f};
    float glow = 0.75f * std::max(0.0f, std::min(1.0f, (fElapsedTime - GLOW_FADE_SCREEN_START) * GLOW_FADE_SCREEN_MUL));
    Vector3 glowColor = {glow * 0.625f, glow, glow * 0.4f};

    Vector2 plasmaOffset[3], plasmaScale[3];
    for (int i = 0; i < 3; i++)
    {
        float rad = 0.6f * (((float)(3 - i - 1)) / 3.0f - 0.2f);
        float x_mul = 0.5f * camRad * MUL_SCALE;
        float y_mul = 1.0f * camAspect * camRad * MUL_SCALE;
        
        float x_add =  rad * camTheta - originScr.x * x_mul * 640.0f / (float)PLASMA_SIZE;
        float y_add = -rad * camPhi   + originScr.y * y_mul * 480.0f / (float)PLASMA_SIZE;
        
        plasmaOffset[i] = {y_add, x_add};
        plasmaScale[i] = {-y_mul, -x_mul};
    }

    rlDisableDepthMask();
    rlDisableDepthTest();
    BeginBlendMode(BLEND_ADDITIVE);

    BeginShaderMode(fogShader);
    SetShaderValueV(fogShader, loc_plasmaOffset, plasmaOffset, SHADER_UNIFORM_VEC2, 3);
    SetShaderValueV(fogShader, loc_plasmaScale, plasmaScale, SHADER_UNIFORM_VEC2, 3);

    int useIntensityMap = 1;
    SetShaderValue(fogShader, loc_useIntensityMap, &useIntensityMap, SHADER_UNIFORM_INT);
    SetShaderValue(fogShader, loc_intensityTint, &intensityTint, SHADER_UNIFORM_VEC3);
    SetShaderValue(fogShader, loc_glowColor, &glowColor, SHADER_UNIFORM_VEC3);

    rlActiveTextureSlot(0);
    rlEnableTexture(intensityColorTex);
    int unit0 = 0;
    SetShaderValue(fogShader, loc_intensityMap, &unit0, SHADER_UNIFORM_INT);

    for (int i = 0; i < 3; i++)
    {
        rlActiveTextureSlot(i + 1);
        rlEnableTexture(plasmaTex[i].id);
        int unit = i + 1;
        SetShaderValue(fogShader, loc_plasmaMap[i], &unit, SHADER_UNIFORM_INT);
    }

    rlEnableVertexArray(quadVAO);
    rlDrawVertexArrayElements(0, 6, nullptr);
    rlDisableVertexArray();
    EndShaderMode();

    for (int i = 3; i >= 0; i--)
    {
        rlActiveTextureSlot(i);
        rlDisableTexture();
    }
    rlActiveTextureSlot(0);

    float glowCircle = std::max(0.0f, std::min(1.0f, (fElapsedTime - GLOW_FADE_CIRCLE_START) * GLOW_FADE_CIRCLE_MUL));
    if (fElapsedTime < BLOB_STATIC_END_TIME)
    {
        float t = fElapsedTime;
        glowCircle = (t < BLOB_STATIC_END_TIME * 0.2f)
                         ? (t / (BLOB_STATIC_END_TIME * 0.2f))
                         : (1.0f - (t - BLOB_STATIC_END_TIME * 0.2f) / BLOB_STATIC_END_TIME);
    }
    int alpha = std::max(0, std::min(255, (int)(255.0f * glowCircle)));
    alpha = std::min(196, alpha * 2);

    if (alpha > 0 && glowCircle > 0.0001f)
    {
        float mul = 0.33f / glowCircle;
        if (fElapsedTime < BLOB_STATIC_END_TIME && fElapsedTime > 0.0001f)
        {
            mul = camRad * BLOB_STATIC_END_TIME / (fElapsedTime * 8.0f);
        }
        if (mul < 0.001f) mul = 0.001f;

        float flareScale = 1.0f / mul;
        Matrix mvp = MatrixMultiply(MatrixScale(flareScale, flareScale, 1.0f),
                                    MatrixTranslate(originScr.x, originScr.y, 0.0f));

        Vector4 tint = {0xA0 / 255.f, 0xFF / 255.f, 0x60 / 255.f, alpha / 255.f};
        BeginShaderMode(glowShader);
        SetShaderValueMatrix(glowShader, glowLoc_mvp, mvp);
        
        rlActiveTextureSlot(0);
        rlEnableTexture(blob.GetGlowTexture().id);
        rlTextureParameters(blob.GetGlowTexture().id, RL_TEXTURE_MAG_FILTER, RL_TEXTURE_FILTER_LINEAR);
        rlTextureParameters(blob.GetGlowTexture().id, RL_TEXTURE_MIN_FILTER, RL_TEXTURE_FILTER_LINEAR);
        int gUnit = 0;
        SetShaderValue(glowShader, glowLoc_tex0, &gUnit, SHADER_UNIFORM_INT);

        SetShaderValue(glowShader, glowLoc_tint, &tint, SHADER_UNIFORM_VEC4);
        rlEnableVertexArray(glowVAO);
        rlDrawVertexArrayElements(0, 6, nullptr);
        rlDisableVertexArray();
        EndShaderMode();

        rlActiveTextureSlot(0);
        rlDisableTexture();
    }

    EndBlendMode();
    rlEnableDepthMask();
    rlEnableDepthTest();
}