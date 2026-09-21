#include "shield_manager.h"

#include "shield_mesh.h"
#include "../blob/blob.h"
#include "../util/embed.h"

#include <rlgl.h>
#include "raymath.h"

#include <algorithm>
#include <cstddef>
#include <cmath>
#include <vector>

void ShieldManager::GpuMesh::upload(const void *vertices, int vertexBytes, const uint16_t *indices, int count)
{
    indexCount = count;

    vao = rlLoadVertexArray();
    rlEnableVertexArray(vao);

    vbo = rlLoadVertexBuffer(vertices, vertexBytes, false);
    rlSetVertexAttribute(0, 3, RL_FLOAT, false, sizeof(ShieldVertex), (int)offsetof(ShieldVertex, position));
    rlEnableVertexAttribute(0);
    rlSetVertexAttribute(1, 3, RL_FLOAT, false, sizeof(ShieldVertex), (int)offsetof(ShieldVertex, normal));
    rlEnableVertexAttribute(1);

    ebo = rlLoadVertexBufferElement(indices, count * (int)sizeof(uint16_t), false);

    rlDisableVertexArray();
}

void ShieldManager::GpuMesh::bind() const
{
#if defined(__EMSCRIPTEN__) || defined(PLATFORM_WEB)
    rlEnableVertexBuffer(vbo);
    rlSetVertexAttribute(0, 3, RL_FLOAT, false, sizeof(ShieldVertex), (int)offsetof(ShieldVertex, position));
    rlEnableVertexAttribute(0);
    rlSetVertexAttribute(1, 3, RL_FLOAT, false, sizeof(ShieldVertex), (int)offsetof(ShieldVertex, normal));
    rlEnableVertexAttribute(1);
    rlEnableVertexBufferElement(ebo);
#else
    rlEnableVertexArray(vao);
#endif
}

void ShieldManager::GpuMesh::unbind() const
{
#if defined(__EMSCRIPTEN__) || defined(PLATFORM_WEB)
    rlDisableVertexBufferElement();
    rlDisableVertexAttribute(0);
    rlDisableVertexAttribute(1);
    rlDisableVertexBuffer();
#else
    rlDisableVertexArray();
#endif
}

void ShieldManager::GpuMesh::unload()
{
    if (vao)
    {
        rlUnloadVertexArray(vao);
        rlUnloadVertexBuffer(vbo);
        rlUnloadVertexBuffer(ebo);
    }
    *this = GpuMesh{};
}

void ShieldManager::create(int newSeed)
{
    using namespace ShieldConfig;

    if (ready) return;
    seed = newSeed;

    ShieldMesh cap = BuildShieldCap(GRID_WIDTH, GRID_HEIGHT, CAP_INSIDE_RADIUS, CAP_OUTSIDE_RADIUS,
                                    CAP_HORIZ_DIM, CAP_VERT_DIM);
    solidMesh.upload(cap.vertices.data(), (int)(cap.vertices.size() * sizeof(ShieldVertex)),
                     cap.indices.data(), (int)cap.indices.size());


    std::vector<ShieldVertex> vertices;
    std::vector<uint16_t> indices;

    const float step = (BAND_MAX_LATITUDE - BAND_MIN_LATITUDE) / (float)BAND_SHIELD_COUNT;
    for (int k = 0; k < BAND_SHIELD_COUNT; k++)
    {
        const float start = BAND_MIN_LATITUDE + step * (float)k;
        ShieldMesh band = BuildShieldBand(GRID_WIDTH, GRID_HEIGHT, BAND_OUTSIDE_RADIUS - BAND_THICKNESS,
                                          BAND_OUTSIDE_RADIUS, BAND_HORIZ_RADIANS, start, start + step);

        const uint16_t base = (uint16_t)vertices.size();
        bandIndexOffset[k] = (int)indices.size();
        bandIndexCount[k] = (int)band.indices.size();

        vertices.insert(vertices.end(), band.vertices.begin(), band.vertices.end());
        for (uint16_t i : band.indices) indices.push_back((uint16_t)(i + base));
    }

    bandMesh.upload(vertices.data(), (int)(vertices.size() * sizeof(ShieldVertex)), indices.data(),
                     (int)indices.size());

#ifdef HAS_EMBED
    #if HAS_EMBED == 2
        #ifdef __EMSCRIPTEN__
            #if !__has_embed("shaders/shield-web.vert") || !__has_embed("shaders/shield-web.frag")
                #error FAILED TO FIND WEB SHIELD SHADERS!
            #endif
        #else
            #if !__has_embed("shaders/shield.vert") || !__has_embed("shaders/shield.frag")
                #error FAILED TO FIND DESKTOP SHIELD SHADERS!
            #endif
        #endif
    #endif

    #ifdef __clang__
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wc23-extensions"
    #endif

    static constexpr char shieldVertData[] = {
        #ifdef __EMSCRIPTEN__
            #embed "shaders/shield-web.vert"
        #else
            #embed "shaders/shield.vert"
        #endif
        , '\0'
    };
    static constexpr char shieldFragData[] = {
        #ifdef __EMSCRIPTEN__
            #embed "shaders/shield-web.frag"
        #else
            #embed "shaders/shield.frag"
        #endif
        , '\0'
    };

    #ifdef __clang__
    #pragma clang diagnostic pop
    #endif

    shader = LoadShaderFromMemory(shieldVertData, shieldFragData);
#else
    shader = LoadShaderFromMemory(
#ifdef __EMSCRIPTEN__
#include "shaders/shield-web.vert.inl"
#else
#include "shaders/shield.vert.inl"
#endif
    ,
#ifdef __EMSCRIPTEN__
#include "shaders/shield-web.frag.inl"
#else
#include "shaders/shield.frag.inl"
#endif
    );
#endif

    if (shader.id == 0 || shader.id == rlGetShaderIdDefault())
    {
        TraceLog(LOG_ERROR, "SHIELDS: shader failed to build, shields disabled");
        shader = Shader{};
        solidMesh.unload();
        bandMesh.unload();
        return;
    }

    loc_mvp = GetShaderLocation(shader, "mvp");
    loc_model = GetShaderLocation(shader, "model");
    loc_eyePos = GetShaderLocation(shader, "eyePos");
    loc_blobLightPos = GetShaderLocation(shader, "blobLightPos");
    loc_moodLightPos = GetShaderLocation(shader, "moodLightPos");
    loc_shading = GetShaderLocation(shader, "shading");
    loc_blobIntensity = GetShaderLocation(shader, "blobIntensity");
    loc_blobSpecColor = GetShaderLocation(shader, "blobSpecColor");

    restart();
    ready = true;
}

void ShieldManager::unload()
{
    solidMesh.unload();
    bandMesh.unload();
    if (shader.id) UnloadShader(shader);
    shader = Shader{};
    ready = false;
}

void ShieldManager::restart()
{
    using namespace ShieldConfig;

    QRand rng(seed);

    float scale = 1.0f;
    for (auto &s : solids)
    {
        s.restart(rng, scale);
        scale *= CAP_RADIUS_SCALE;
    }
    for (auto &b : bands) b.restart(rng);
}

float ShieldManager::ShadingAt(float t)
{
    using namespace ShieldConfig;

    float s = MAX_SHADING;
    if (t < FADE_IN_START_TIME + FADE_IN_DELTA)
        s *= (t - FADE_IN_START_TIME) / FADE_IN_DELTA;
    else if (t > FADE_OUT_START_TIME)
        s *= (FADE_OUT_START_TIME + FADE_OUT_DELTA - t) / FADE_OUT_DELTA;

    return std::clamp(s, 0.0f, 1.0f);
}

void ShieldManager::render(const Camera3D &camera, const Blob &blob, float blobIntensity, float elapsedTime,
                           ShieldPass pass)
{
    using namespace ShieldConfig;

    if (!ready) return;

    const float shading = ShadingAt(elapsedTime);
    if (shading <= 0.0f) return;

    const Vector3 center = blob.GetCenter();
    const Vector3 look = Vector3Subtract(camera.target, camera.position);
    const float blobDot = Vector3DotProduct(center, look);

    Matrix solidModel[SOLID_SHIELD_COUNT];
    int order[SOLID_SHIELD_COUNT];
    int drawCount = 0;

    for (int i = 0; i < SOLID_SHIELD_COUNT; i++)
    {
        const Matrix pose = solids[i].matrixAt(elapsedTime);
        const Vector3 c = Vector3Add(solids[i].centerFor(pose), center);

        solidModel[i] = pose;
        solidModel[i].m12 += center.x;
        solidModel[i].m13 += center.y;
        solidModel[i].m14 += center.z;

        const bool farSide = Vector3DotProduct(c, look) >= blobDot;
        if (farSide != (pass == ShieldPass::FarSide)) continue;

        order[drawCount++] = i;
    }

    if (pass == ShieldPass::FarSide)
        std::sort(order, order + drawCount);
    else
        std::sort(order, order + drawCount, [](int a, int b) { return a > b; });

    const bool drawBands = (pass == ShieldPass::NearSide);
    if (drawCount == 0 && !drawBands) return;

    const Matrix view = MatrixLookAt(camera.position, camera.target, camera.up);
    const Matrix proj = MatrixPerspective(camera.fovy * DEG2RAD, (float)GetScreenWidth() / (float)GetScreenHeight(),
                                          0.4f, 800.0f);
    const Matrix viewProj = MatrixMultiply(view, proj);

    const float ramp = std::clamp((elapsedTime - PUSHOUT_START_TIME) / PUSHOUT_DELTA, 0.0f, 1.0f);
    const float lightIntensity = blobIntensity * BLOB_INTENSITY_SCALE * ramp * ramp;

    const Vector3 moodPos = MOOD_LIGHT_POSITION;
    const Vector3 blobSpec = BLOB_SPEC_COLOR;

    rlDrawRenderBatchActive();

    BeginBlendMode(BLEND_ALPHA);
    rlDisableDepthMask();
    rlEnableBackfaceCulling();

    BeginShaderMode(shader);
    SetShaderValueMatrix(shader, loc_mvp, viewProj);
    SetShaderValue(shader, loc_eyePos, &camera.position, SHADER_UNIFORM_VEC3);
    SetShaderValue(shader, loc_blobLightPos, &center, SHADER_UNIFORM_VEC3);
    SetShaderValue(shader, loc_moodLightPos, &moodPos, SHADER_UNIFORM_VEC3);
    SetShaderValue(shader, loc_shading, &shading, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shader, loc_blobIntensity, &lightIntensity, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shader, loc_blobSpecColor, &blobSpec, SHADER_UNIFORM_VEC3);

    if (drawCount > 0)
    {
        solidMesh.bind();
        for (int n = 0; n < drawCount; n++)
        {
            SetShaderValueMatrix(shader, loc_model, solidModel[order[n]]);
            rlDrawVertexArrayElements(0, solidMesh.indexCount, nullptr);
        }
        solidMesh.unbind();
    }

    if (drawBands)
    {
        bandMesh.bind();
        for (int k = BAND_SHIELD_COUNT - 1; k >= 0; k--)
        {
            Matrix model = bands[k].matrixAt(elapsedTime);
            model.m12 += center.x;
            model.m13 += center.y;
            model.m14 += center.z;

            SetShaderValueMatrix(shader, loc_model, model);
            rlDrawVertexArrayElements(bandIndexOffset[k], bandIndexCount[k], nullptr);
        }
        bandMesh.unbind();
    }

    EndShaderMode();

    rlDisableBackfaceCulling();
    rlEnableDepthMask();
    EndBlendMode();
}
