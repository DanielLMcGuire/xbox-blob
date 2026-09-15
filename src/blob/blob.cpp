#include "blob.h"
#include "../defines.h"
#include "blob_math.h"
#include "../util/embed.h"

#include <rlgl.h>
#include "raymath.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>

Blob* g_Blob = nullptr;

void Blob::Init()
{
    for (auto& b : blobBumps) b.Init();
    for (auto& b : bloblets) b.Init();

    numBlobBumps = 0;
    numBloblets = 0;

    color = { 0.25f, 1.0f, 0.15f, 1.0f };
    position = { 0, 0, 0 };
    scale = { 1, 1, 1 };
    radius = 2.3f;

    g_Blob = this;
}

void Blob::BuildCubeSphere(int resolution, std::vector<Vector3>& outPositions,
                           std::vector<uint16_t>& outIndices)
{
    int subdiv = std::max(1, resolution / 2);
    float step = 2.0f / static_cast<float>(subdiv);
    int stride = subdiv + 1;

    outPositions.clear();
    outPositions.resize(static_cast<size_t>(6) * stride * stride);

    for (int k = 0; k < 6; k++)
    {
        for (int j = 0; j <= subdiv; j++)
        {
            for (int i = 0; i <= subdiv; i++)
            {
                float fu = (i == subdiv) ? 1.0f : (-1.0f + step * (float)i);
                float fv = (j == subdiv) ? 1.0f : (-1.0f + step * (float)j);

                Vector3 pos{};
                switch (k)
                {
                    case 0: pos = { -1.0f, -fu, +fv }; break;
                    case 1: pos = { +fv, -1.0f, -fu }; break;
                    case 2: pos = { -fu, +fv, -1.0f }; break;
                    case 3: pos = { +1.0f, +fu, +fv }; break;
                    case 4: pos = { +fv, +1.0f, +fu }; break;
                    case 5: pos = { +fu, +fv, +1.0f }; break;
                }
                pos = Vector3Normalize(pos);

                int idx = k * stride * stride + j * stride + i;
                outPositions[idx] = pos;
            }
        }
    }

    outIndices.clear();
    outIndices.reserve((size_t)6 * subdiv * subdiv * 6);
    for (int k = 0; k < 6; k++)
    {
        uint16_t faceBase = (uint16_t)(k * stride * stride);
        for (int j = 0; j < subdiv; j++)
        {
            for (int i = 0; i < subdiv; i++)
            {
                uint16_t a = faceBase + (uint16_t)(j * stride + i);
                uint16_t b = faceBase + (uint16_t)(j * stride + i + 1);
                uint16_t c = faceBase + (uint16_t)((j + 1) * stride + i + 1);
                uint16_t d = faceBase + (uint16_t)((j + 1) * stride + i);

                outIndices.push_back(a);
                outIndices.push_back(b);
                outIndices.push_back(d);

                outIndices.push_back(b);
                outIndices.push_back(c);
                outIndices.push_back(d);
            }
        }
    }
}

void Blob::UploadStaticMesh(const std::vector<Vector3>& positions, const std::vector<uint16_t>& indices,
                             unsigned int& outVAO, unsigned int& outVBO, unsigned int& outEBO)
{
    outVAO = rlLoadVertexArray();
    rlEnableVertexArray(outVAO);

    outVBO = rlLoadVertexBuffer(positions.data(), (int)(positions.size() * sizeof(Vector3)), false);
    rlSetVertexAttribute(0, 3, RL_FLOAT, false, sizeof(Vector3), 0);
    rlEnableVertexAttribute(0);

    outEBO = rlLoadVertexBufferElement(indices.data(), (int)(indices.size() * sizeof(uint16_t)), false);

    rlDisableVertexArray();
}

void Blob::Load()
{
    g_Blob = this;

    constexpr int BLOBLET_DIM = 8;
    constexpr int BLOB_DIM = 32;

    std::vector<uint16_t> blobIndices;
    BuildCubeSphere(BLOB_DIM, unitSphereNormals, blobIndices);
    UploadStaticMesh(unitSphereNormals, blobIndices, blobVAO, blobStaticVBO, blobEBO);
    blobIndexCount = (unsigned int)blobIndices.size();
    numVertsPerFace = (int)(unitSphereNormals.size() / 6);

    changingVertices.resize(unitSphereNormals.size());

    rlEnableVertexArray(blobVAO);
    blobDynamicVBO = rlLoadVertexBuffer(nullptr, (int)(changingVertices.size() * sizeof(Vector4)), true);
    rlSetVertexAttribute(1, 4, RL_FLOAT, false, sizeof(Vector4), 0);
    rlEnableVertexAttribute(1);
    rlDisableVertexArray();

    std::vector<Vector3> blobletPositions;
    std::vector<uint16_t> blobletIndices;
    BuildCubeSphere(BLOBLET_DIM, blobletPositions, blobletIndices);
    UploadStaticMesh(blobletPositions, blobletIndices, blobletVAO, blobletStaticVBO, blobletEBO);
    blobletIndexCount = (unsigned int)blobletIndices.size();

    Restart();

#ifdef HAS_EMBED
    #if HAS_EMBED == 2
        #ifdef __EMSCRIPTEN__
            #if !__has_embed("shaders/blob-web.vert")    || !__has_embed("shaders/blob-web.frag") || \
                !__has_embed("shaders/bloblet-web.vert") || !__has_embed("shaders/bloblet-web.frag")
                #error FAILED TO FIND WEB SHADERS!
            #endif
        #else
            #if !__has_embed("shaders/blob.vert")    || !__has_embed("shaders/blob.frag") || \
                !__has_embed("shaders/bloblet.vert") || !__has_embed("shaders/bloblet.frag")
                #error FAILED TO FIND DESKTOP SHADERS!
            #endif
        #endif
    #endif

    #ifdef __clang__
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wc23-extensions"
    #endif

    static constexpr char blobVertData[] = { 
        #ifdef __EMSCRIPTEN__
            #embed "shaders/blob-web.vert"
        #else
            #embed "shaders/blob.vert"
        #endif
        , '\0' 
    };
    static constexpr char blobFragData[] = { 
        #ifdef __EMSCRIPTEN__
            #embed "shaders/blob-web.frag"
        #else
            #embed "shaders/blob.frag"
        #endif
        , '\0' 
    };
    static constexpr char blobletVertData[] = { 
        #ifdef __EMSCRIPTEN__
            #embed "shaders/bloblet-web.vert"
        #else
            #embed "shaders/bloblet.vert"
        #endif
        , '\0' 
    };
    static constexpr char blobletFragData[] = { 
        #ifdef __EMSCRIPTEN__
            #embed "shaders/bloblet-web.frag"
        #else
            #embed "shaders/bloblet.frag"
        #endif
        , '\0' 
    };

    #ifdef __clang__
    #pragma clang diagnostic pop
    #endif

    blobShader = LoadShaderFromMemory(blobVertData, blobFragData);
    blobletShader = LoadShaderFromMemory(blobletVertData, blobletFragData);
#else
    blobShader = LoadShaderFromMemory(
#ifdef __EMSCRIPTEN__
#include "shaders/blob-web.vert.inl"
#else
#include "shaders/blob.vert.inl"
#endif
    ,
#ifdef __EMSCRIPTEN__
#include "shaders/blob-web.frag.inl"
#else
#include "shaders/blob.frag.inl"
#endif
    );
    blobletShader = LoadShaderFromMemory(
#ifdef __EMSCRIPTEN__
#include "shaders/bloblet-web.vert.inl"
#else
#include "shaders/bloblet.vert.inl"
#endif
    ,
#ifdef __EMSCRIPTEN__
#include "shaders/bloblet-web.frag.inl"
#else
#include "shaders/bloblet.frag.inl"
#endif
    );
#endif

    blobLoc_mvp = GetShaderLocation(blobShader, "mvp");
    blobLoc_eyePos = GetShaderLocation(blobShader, "eyePos");
    blobLoc_scaling = GetShaderLocation(blobShader, "scaling");
    blobLoc_ooScaling = GetShaderLocation(blobShader, "ooScaling");
    blobLoc_center = GetShaderLocation(blobShader, "center");
    blobLoc_baseColor = GetShaderLocation(blobShader, "baseColor");
    blobLoc_ambientColor = GetShaderLocation(blobShader, "ambientColor");

    bloLoc_mvp = GetShaderLocation(blobletShader, "mvp");
    bloLoc_eyePos = GetShaderLocation(blobletShader, "eyePos");
    bloLoc_center = GetShaderLocation(blobletShader, "center");
    bloLoc_scaleDir = GetShaderLocation(blobletShader, "scaleDir");
    bloLoc_scalePerp = GetShaderLocation(blobletShader, "scalePerp");
    bloLoc_scaleDirPMP = GetShaderLocation(blobletShader, "scaleDirPMP");
    bloLoc_baseColor = GetShaderLocation(blobletShader, "baseColor");
    bloLoc_ambientColor = GetShaderLocation(blobletShader, "ambientColor");
    bloLoc_alphaScale = GetShaderLocation(blobletShader, "alphaScale");

    BuildGlowTexture();
}

void Blob::Unload()
{
    if (blobVAO)
    {
        rlUnloadVertexArray(blobVAO);
        rlUnloadVertexBuffer(blobStaticVBO);
        rlUnloadVertexBuffer(blobDynamicVBO);
        rlUnloadVertexBuffer(blobEBO);
        blobVAO = 0;
    }
    if (blobletVAO)
    {
        rlUnloadVertexArray(blobletVAO);
        rlUnloadVertexBuffer(blobletStaticVBO);
        rlUnloadVertexBuffer(blobletEBO);
        blobletVAO = 0;
    }
    if (blobShader.id) UnloadShader(blobShader);
    if (blobletShader.id) UnloadShader(blobletShader);
    if (glowTexture.id) UnloadTexture(glowTexture);
    blobShader = Shader{};
    blobletShader = Shader{};
    glowTexture = Texture2D{};
}

void Blob::ZeroChangingVertices()
{
    for (size_t i = 0; i < unitSphereNormals.size(); i++)
    {
        const Vector3& n = unitSphereNormals[i];
        changingVertices[i] = { n.x, n.y, n.z, 1.0f };
    }
}

void Blob::PrepareChangingVertices()
{
    const BlobBump* boi[MAX_BLOB_BUMPS];

    for (int face = 0; face < 6; face++)
    {
        int numBoi = 0;
        for (int i = 0; i < numBlobBumps; i++)
            if (blobBumps[i].facesOfInterest & (1 << face))
                boi[numBoi++] = &blobBumps[i];

        int base = face * numVertsPerFace;
        for (int v = 0; v < numVertsPerFace; v++)
        {
            const Vector3& usNormal = unitSphereNormals[base + v];
            Vector4 accum{ usNormal.x, usNormal.y, usNormal.z, 0.0f };

            for (int j = numBoi - 1; j >= 0; j--)
            {
                const BlobBump* b = boi[j];
                Vector3 delta = Vector3Subtract(usNormal, b->position);
                float dist2 = delta.x * delta.x + delta.y * delta.y + delta.z * delta.z;
                if (dist2 < b->radius2)
                {
                    float dist2Mo = dist2 * b->ooRadius2 - 1.0f;

                    float displacement = radius * b->magnitude * dist2Mo * dist2Mo;
                    float perturbAmount = -4.0f * b->magnitude * b->ooRadius2 * dist2Mo;

                    Vector3 lnorm = AddScaled(usNormal, delta, perturbAmount);
                    QuickNormalize(&lnorm);

                    accum.x += lnorm.x;
                    accum.y += lnorm.y;
                    accum.z += lnorm.z;
                    accum.w += displacement;
                }
            }

            changingVertices[base + v] = accum;
        }
    }
}

void Blob::Restart()
{
    numBloblets = 0;
    numBlobBumps = 0;

    while (numBlobBumps < MAX_BLOB_BUMPS)
    {
        if (blobBumps[numBlobBumps++].Create(-0.3f, (numBloblets < MAX_BLOBLETS) ? &bloblets[numBloblets] : nullptr))
            numBloblets++;
    }

    ZeroChangingVertices();
}

void Blob::AdvanceTime(float elapsedTime, float dt)
{
    if (elapsedTime < BLOB_STATIC_END_TIME)
    {
        return;
    }

    for (int i = 0; i < numBloblets; i++)
        bloblets[i].Update(elapsedTime, dt);

    for (int i = 0; i < numBlobBumps; i++)
    {
        if (blobBumps[i].Update(elapsedTime, dt, (numBloblets < MAX_BLOBLETS) ? &bloblets[numBloblets] : nullptr))
            numBloblets++;
    }

    PrepareChangingVertices();
}

void Blob::GetLightForPosition(Vector3 queryPosition, Vector3* outLightPos, float* outIntensity) const
{
    float totalWeights = 0.0f;
    Vector3 avPos{ 0, 0, 0 };
    float avIntensity = 0.0f;

    {
        float dist2 = Vector3DistanceSqr(queryPosition, position);
        dist2 = std::max(dist2, 1e-6f);
        float weight = 1.0f / dist2;
        avIntensity += 4.0f * lightIntensity * weight;
        avPos = AddScaled(avPos, position, weight);
        totalWeights += weight;
    }

    for (int i = 0; i < numBloblets; i++)
    {
        float dist2 = Vector3DistanceSqr(queryPosition, bloblets[i].position);
        dist2 = std::max(dist2, 1e-6f);
        float weight = 1.0f / dist2;
        avIntensity += lightIntensity * weight;
        avPos = AddScaled(avPos, bloblets[i].position, weight);
        totalWeights += weight;
    }

    float ooTotalWeights = 1.0f / totalWeights;
    *outLightPos = Vector3Scale(avPos, ooTotalWeights);
    *outIntensity = ooTotalWeights * avIntensity;
}

void Blob::Render(const Camera3D& camera, float pulseIntensity, float blobIntensity,
                   float baseBlobIntensity, float elapsedTime)
{
    lightIntensity = blobIntensity + pulseIntensity;
    Matrix view = MatrixLookAt(camera.position, camera.target, camera.up);
    Matrix proj = MatrixPerspective(camera.fovy * DEG2RAD,
                                     (float)GetScreenWidth() / (float)GetScreenHeight(),
                                     0.4f, 800.0f);
    Matrix viewProj = MatrixMultiply(view, proj);

    float curRad = radius * (1.0f + 1.3f * sqrtf(std::max(0.0f, pulseIntensity)));

    {
        float alpha = std::min(blobIntensity, 1.0f) * 255.0f;
        Color tint = { 0xa0, 0xff, 0x40, (unsigned char)alpha };

        float haloRadius = curRad * 5.2f;
        float haloSize = haloRadius * 2.0f;

        BeginBlendMode(BLEND_ADDITIVE);
        rlDisableDepthMask();
        DrawBillboardPro(camera, glowTexture,
                          Rectangle{ 0, 0, (float)glowTexture.width, (float)glowTexture.height },
                          position, camera.up,
                          Vector2{ haloSize, haloSize }, Vector2{ haloSize * 0.5f, haloSize * 0.5f },
                          0.0f, tint);
        rlEnableDepthMask();
        EndBlendMode();
    }

    Vector3 scaledRadius = { curRad * scale.x, curRad * scale.y, curRad * scale.z };
    Vector3 ooScaledRadius = { 1.0f / scaledRadius.x, 1.0f / scaledRadius.y, 1.0f / scaledRadius.z };

    float colorIntensity = BLOB_BASE_INTENSITY + 4.0f * (1.2f * baseBlobIntensity + 0.8f * pulseIntensity);
    colorIntensity *= std::min(1.0f, elapsedTime * 4.0f);
    Vector4 litColor = Vector4Scale(color, colorIntensity);
    Vector4 ambientColor = Vector4Scale(color, 0.0f);

BeginBlendMode(BLEND_ALPHA);
BeginShaderMode(blobShader);
SetShaderValueMatrix(blobShader, blobLoc_mvp, viewProj);
SetShaderValue(blobShader, blobLoc_eyePos, &camera.position, SHADER_UNIFORM_VEC3);
SetShaderValue(blobShader, blobLoc_scaling, &scaledRadius, SHADER_UNIFORM_VEC3);
SetShaderValue(blobShader, blobLoc_ooScaling, &ooScaledRadius, SHADER_UNIFORM_VEC3);
SetShaderValue(blobShader, blobLoc_center, &position, SHADER_UNIFORM_VEC3);
SetShaderValue(blobShader, blobLoc_baseColor, &litColor, SHADER_UNIFORM_VEC4);
SetShaderValue(blobShader, blobLoc_ambientColor, &ambientColor, SHADER_UNIFORM_VEC4);

rlUpdateVertexBuffer(blobDynamicVBO, changingVertices.data(),
                      (int)(changingVertices.size() * sizeof(Vector4)), 0);

#if defined(__EMSCRIPTEN__) || defined(PLATFORM_WEB)
    rlEnableVertexBuffer(blobStaticVBO);
    rlSetVertexAttribute(0, 3, RL_FLOAT, false, sizeof(Vector3), 0);
    rlEnableVertexAttribute(0);

    rlEnableVertexBuffer(blobDynamicVBO);
    rlSetVertexAttribute(1, 4, RL_FLOAT, false, sizeof(Vector4), 0);
    rlEnableVertexAttribute(1);

    rlEnableVertexBufferElement(blobEBO);
#else
    rlEnableVertexArray(blobVAO);
#endif

rlDrawVertexArrayElements(0, (int)blobIndexCount, nullptr);

#if defined(__EMSCRIPTEN__) || defined(PLATFORM_WEB)
    rlDisableVertexBufferElement();
    rlDisableVertexAttribute(0);
    rlDisableVertexAttribute(1);
    rlDisableVertexBuffer();
#else
    rlDisableVertexArray();
#endif

EndShaderMode();


Vector4 blobletColor = Vector4Scale(color, 0.3f * blobIntensity);
Vector4 blobletAmbient = Vector4Scale(color, 0.2f);

BeginShaderMode(blobletShader);
SetShaderValueMatrix(blobletShader, bloLoc_mvp, viewProj);
SetShaderValue(blobletShader, bloLoc_eyePos, &camera.position, SHADER_UNIFORM_VEC3);
SetShaderValue(blobletShader, bloLoc_baseColor, &blobletColor, SHADER_UNIFORM_VEC4);
SetShaderValue(blobletShader, bloLoc_ambientColor, &blobletAmbient, SHADER_UNIFORM_VEC4);
float alphaScale = 2.0f;
SetShaderValue(blobletShader, bloLoc_alphaScale, &alphaScale, SHADER_UNIFORM_FLOAT);

#if defined(__EMSCRIPTEN__) || defined(PLATFORM_WEB)
    rlEnableVertexBuffer(blobletStaticVBO);
    rlSetVertexAttribute(0, 3, RL_FLOAT, false, sizeof(Vector3), 0);
    rlEnableVertexAttribute(0);

    rlEnableVertexBufferElement(blobletEBO);
#else
    rlEnableVertexArray(blobletVAO);
#endif

for (int i = 0; i < numBloblets; i++)
{
    const Bloblet& bl = bloblets[i];

    float perp = bl.radius / sqrtf(bl.wobble);
    float parallelMinusPerp = bl.radius * bl.wobble - perp;
    Vector3 scaleDirPMP = Vector3Scale(bl.direction, parallelMinusPerp);

    SetShaderValue(blobletShader, bloLoc_center, &bl.position, SHADER_UNIFORM_VEC3);
    SetShaderValue(blobletShader, bloLoc_scaleDir, &bl.direction, SHADER_UNIFORM_VEC3);
    SetShaderValue(blobletShader, bloLoc_scalePerp, &perp, SHADER_UNIFORM_FLOAT);
    SetShaderValue(blobletShader, bloLoc_scaleDirPMP, &scaleDirPMP, SHADER_UNIFORM_VEC3);

    rlDrawVertexArrayElements(0, (int)blobletIndexCount, nullptr);
}

#if defined(__EMSCRIPTEN__) || defined(PLATFORM_WEB)
    rlDisableVertexBufferElement();
    rlDisableVertexAttribute(0);
    rlDisableVertexBuffer();
#else
    rlDisableVertexArray();
#endif

EndShaderMode();
EndBlendMode();
}
