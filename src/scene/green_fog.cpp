#include "green_fog.h"
#include "scene_texgen.h"
#include "../defines.h"
#include "../util/embed.h"
#include "../blob/blob.h"

#include <rlgl.h>
#include <algorithm>
#include <cmath>
#include <cstddef>


// NOT YET DONE! Needs scene depth buffer logic to be added

namespace
{
constexpr int PLASMA_SIZE = 256;
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

void GreenFog::create()
{
    QuadVertex verts[4];
    verts[0].pos = {-1, -1, 1};
    verts[1].pos = {-1, +1, 1};
    verts[2].pos = {+1, +1, 1};
    verts[3].pos = {+1, -1, 1};
    verts[0].u0 = 0; verts[0].v0 = 1;
    verts[1].u0 = 0; verts[1].v0 = 0;
    verts[2].u0 = 1; verts[2].v0 = 0;
    verts[3].u0 = 1; verts[3].v0 = 1;
    for (auto &v : verts)
    {
        v.v1 = -(2.0f * v.u0 - 1.0f) * 640.0f / (float)PLASMA_SIZE;
        v.u1 = -(2.0f * v.v0 - 1.0f) * 480.0f / (float)PLASMA_SIZE;
    }
    static const uint16_t quadIndices[6] = {0, 1, 2, 0, 2, 3};

    quadVAO = rlLoadVertexArray();
    rlEnableVertexArray(quadVAO);
    quadVBO = rlLoadVertexBuffer(verts, sizeof(verts), false);
    rlSetVertexAttribute(0, 3, RL_FLOAT, false, sizeof(QuadVertex), offsetof(QuadVertex, pos));
    rlEnableVertexAttribute(0);
    rlSetVertexAttribute(1, 2, RL_FLOAT, false, sizeof(QuadVertex), offsetof(QuadVertex, u0));
    rlEnableVertexAttribute(1);
    rlLoadVertexBufferElement(quadIndices, sizeof(quadIndices), false);
    rlDisableVertexArray();

    GlowVertex gverts[4] = {
        {{-1, -1, 0}, 0, 1}, {{-1, +1, 0}, 0, 0}, {{+1, +1, 0}, 1, 0}, {{+1, -1, 0}, 1, 1}};
    glowVAO = rlLoadVertexArray();
    rlEnableVertexArray(glowVAO);
    glowVBO = rlLoadVertexBuffer(gverts, sizeof(gverts), false);
    rlSetVertexAttribute(0, 3, RL_FLOAT, false, sizeof(GlowVertex), offsetof(GlowVertex, pos));
    rlEnableVertexAttribute(0);
    rlSetVertexAttribute(1, 2, RL_FLOAT, false, sizeof(GlowVertex), offsetof(GlowVertex, u));
    rlEnableVertexAttribute(1);
    rlLoadVertexBufferElement(quadIndices, sizeof(quadIndices), false);
    rlDisableVertexArray();

#ifdef HAS_EMBED
    #if HAS_EMBED == 2
        #if !__has_embed("shaders/greenfog.vert")
            #error FAILED TO FIND greenfog.vert!
        #endif
        #if !__has_embed("shaders/greenfog.frag")
            #error FAILED TO FIND greenfog.frag!
        #endif
        #if !__has_embed("shaders/glow.vert")
            #error FAILED TO FIND glow.vert!
        #endif
        #if !__has_embed("shaders/glow.frag")
            #error FAILED TO FIND glow.frag!
        #endif
    #endif
    #ifdef __clang__
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wc23-extensions"
    #endif
    static constexpr char fogVertData[] = { 
        #embed "shaders/greenfog.vert" 
        , '\0' 
    };
    static constexpr char fogFragData[] = { 
        #embed "shaders/greenfog.frag" 
        , '\0' 
    };
    static constexpr char glowVertData[] = { 
        #embed "shaders/glow.vert" 
        , '\0' 
    };
    static constexpr char glowFragData[] = { 
        #embed "shaders/glow.frag" 
        , '\0' 
    };
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

    restart();
}

void GreenFog::unload()
{
    UnloadShader(fogShader);
    UnloadShader(glowShader);
    for (auto &t : plasmaTex) UnloadTexture(t);
}

void GreenFog::restart()
{
    for (auto &t : plasmaTex) UnloadTexture(t);
    int tffonp = 255 / 3;
    int seed = rng.Rand();
    auto images = CreatePlasmaMapImages(3, PLASMA_SIZE, 5 * tffonp, seed, 0, 255 / 3);
    for (int i = 0; i < 3; i++)
    {
        plasmaTex[i] = LoadTextureFromImage(images[i]);
        UnloadImage(images[i]);
    }
}

void GreenFog::render(const Camera3D &camera, const Blob &blob, float blobIntensity, float fElapsedTime)
{
    Matrix view = GetCameraMatrix(camera);
    float aspect = (float)GetScreenWidth() / (float)GetScreenHeight();
    Matrix proj = MatrixPerspective(camera.fovy * DEG2RAD, aspect, 0.4f, 800.0f);
    Matrix viewProj = MatrixMultiply(view, proj);

    float radiusFromBlob = Vector3Distance(camera.position, blob.position);
    Vector3 rel = Vector3Subtract(camera.position, blob.position);
    float theta = atan2f(rel.y, rel.x);
    float phi = (radiusFromBlob > 0.0001f) ? asinf(std::max(-1.0f, std::min(1.0f, rel.z / radiusFromBlob))) : 0.0f;

    Vector3 originScr = ProjectToNDC(blob.position, viewProj);

    float intensity = std::max(0.0f, blobIntensity * 0.7f - 0.1f);
    Vector3 intensityTint = {0.0f, intensity, 0.0f};
    float glow = 0.75f * std::max(0.0f, std::min(1.0f, (fElapsedTime - GLOW_FADE_SCREEN_START) * GLOW_FADE_SCREEN_MUL));
    Vector3 glowColor = {glow * 0.625f, glow, glow * 0.4f};

    Vector2 plasmaOffset[3], plasmaScale[3];
    for (int i = 0; i < 3; i++)
    {
        float rad = 0.6f * (((float)(3 - i - 1)) / 3.0f - 0.2f);
        float x_mul = 0.5f * radiusFromBlob * MUL_SCALE;
        float y_mul = 1.0f * aspect * radiusFromBlob * MUL_SCALE;
        float x_add = rad * theta - originScr.x * x_mul * 640.0f / (float)PLASMA_SIZE;
        float y_add = -rad * phi + originScr.y * y_mul * 480.0f / (float)PLASMA_SIZE;
        plasmaOffset[i] = {y_add, x_add};
        plasmaScale[i] = {-y_mul, -x_mul};
    }

    rlDisableDepthMask();
    rlDisableDepthTest();
    rlSetBlendFactors(RL_SRC_ALPHA, RL_ONE, RL_FUNC_ADD);
    rlEnableColorBlend();

    BeginShaderMode(fogShader);
    SetShaderValueV(fogShader, loc_plasmaOffset, plasmaOffset, SHADER_UNIFORM_VEC2, 3);
    SetShaderValueV(fogShader, loc_plasmaScale, plasmaScale, SHADER_UNIFORM_VEC2, 3);
    for (int i = 0; i < 3; i++)
        SetShaderValueTexture(fogShader, loc_plasmaMap[i], plasmaTex[i]);
    int useIntensityMap = 0;
    SetShaderValue(fogShader, loc_useIntensityMap, &useIntensityMap, SHADER_UNIFORM_INT);
    SetShaderValue(fogShader, loc_intensityTint, &intensityTint, SHADER_UNIFORM_VEC3);
    SetShaderValue(fogShader, loc_glowColor, &glowColor, SHADER_UNIFORM_VEC3);
    rlEnableVertexArray(quadVAO);
    rlDrawVertexArrayElements(0, 6, nullptr);
    rlDisableVertexArray();
    EndShaderMode();

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

    if (alpha > 0)
    {
        Vector4 tint = {0xA0 / 255.f, 0xFF / 255.f, 0x60 / 255.f, alpha / 255.f};
        Matrix mvp = MatrixIdentity();
        BeginShaderMode(glowShader);
        SetShaderValueMatrix(glowShader, glowLoc_mvp, mvp);
        SetShaderValueTexture(glowShader, glowLoc_tex0, blob.GetGlowTexture());
        SetShaderValue(glowShader, glowLoc_tint, &tint, SHADER_UNIFORM_VEC4);
        rlEnableVertexArray(glowVAO);
        rlDrawVertexArrayElements(0, 6, nullptr);
        rlDisableVertexArray();
        EndShaderMode();
    }

    rlEnableDepthMask();
    rlEnableDepthTest();
}
