#include "logo_renderer.h"
#include "logo_data.h"
#include "text_data.h"
#include "tm_pixels.h"
#include "scene_texgen.h"
#include "../defines.h"

#include <rlgl.h>
#include <algorithm>
#include <cmath>
#include <cstddef>

void LogoRenderer::uploadLogoMesh(GPUMesh &gm, const LogoMesh &mesh)
{
    gm.vao = rlLoadVertexArray();
    rlEnableVertexArray(gm.vao);
    gm.vbo = rlLoadVertexBuffer(mesh.vertices.data(), (int)(mesh.vertices.size() * sizeof(LogoVertex)), false);
    rlSetVertexAttribute(0, 3, RL_FLOAT, false, sizeof(LogoVertex), offsetof(LogoVertex, pos));
    rlEnableVertexAttribute(0);
    rlSetVertexAttribute(1, 2, RL_FLOAT, false, sizeof(LogoVertex), offsetof(LogoVertex, u));
    rlEnableVertexAttribute(1);
    gm.ebo = rlLoadVertexBufferElement(mesh.indices.data(), (int)(mesh.indices.size() * sizeof(uint16_t)), false);
    gm.indexCount = (int)mesh.indices.size();
    rlDisableVertexArray();
}

void LogoRenderer::uploadTextMesh(GPUMesh &gm, const TextMesh &mesh)
{
    gm.vao = rlLoadVertexArray();
    rlEnableVertexArray(gm.vao);
    gm.vbo = rlLoadVertexBuffer(mesh.vertices.data(), (int)(mesh.vertices.size() * sizeof(TextVertex)), false);
    rlSetVertexAttribute(0, 3, RL_FLOAT, false, sizeof(TextVertex), 0);
    rlEnableVertexAttribute(0);
    gm.ebo = rlLoadVertexBufferElement(mesh.indices.data(), (int)(mesh.indices.size() * sizeof(uint16_t)), false);
    gm.indexCount = (int)mesh.indices.size();
    rlDisableVertexArray();
}

void LogoRenderer::drawMeshRaw(const GPUMesh &gm)
{
    rlEnableVertexArray(gm.vao);
    rlDrawVertexArrayElements(0, gm.indexCount, nullptr);
    rlDisableVertexArray();
}

void LogoRenderer::create()
{
    LogoMesh lip = DecodeLogoMesh(verts_xboxlogolip_0C, vertex_count_xboxlogolip_0,
                                  indices_xboxlogolip_0C, index_count_xboxlogolip_0,
                                  xbl_OO_POS_SCALE, xbl_POS_DELTA, xbl_OO_TEX_SCALE, xbl_TEX_DELTA);
    uploadLogoMesh(lipMesh, lip);

    LogoMesh surface = DecodeLogoMesh(verts_xboxlogosurface_0C, vertex_count_xboxlogosurface_0,
                                       indices_xboxlogosurface_0C, index_count_xboxlogosurface_0,
                                       xbl_OO_POS_SCALE, xbl_POS_DELTA, xbl_OO_TEX_SCALE, xbl_TEX_DELTA);
    uploadLogoMesh(surfaceMesh, surface);

    LogoMesh surfaceTop = DecodeLogoMesh(verts_xboxlogosurfacetop_0C, vertex_count_xboxlogosurfacetop_0,
                                          indices_xboxlogosurfacetop_0C, index_count_xboxlogosurfacetop_0,
                                          xbl_OO_POS_SCALE, xbl_POS_DELTA, xbl_OO_TEX_SCALE, xbl_TEX_DELTA);

    for (auto &v : surfaceTop.vertices)
        if (fabsf(v.v - 1.f) <= 0.01f) v.v = -1.f;
    uploadLogoMesh(surfaceTopMesh, surfaceTop);

    LogoMesh interior = DecodeLogoMesh(verts_xboxlogointerior_0C, vertex_count_xboxlogointerior_0,
                                        indices_xboxlogointerior_0C, index_count_xboxlogointerior_0,
                                        xbl_OO_POS_SCALE, xbl_POS_DELTA, xbl_OO_TEX_SCALE, xbl_TEX_DELTA);
    uploadLogoMesh(interiorMesh, interior);

    LogoMesh tmSlash = DecodeLogoMesh(verts_tm_slash_0C, vertex_count_tm_slash_0,
                                       indices_tm_slash_0C, index_count_tm_slash_0,
                                       xbl_OO_POS_SCALE, xbl_POS_DELTA, xbl_OO_TEX_SCALE, xbl_TEX_DELTA);
    uploadLogoMesh(tmSlashMesh, tmSlash);

    LogoMesh tmWordmark = DecodeLogoMesh(verts_tm_wordmark_0C, vertex_count_tm_wordmark_0,
                                          indices_tm_wordmark_0C, index_count_tm_wordmark_0,
                                          xbl_OO_POS_SCALE, xbl_POS_DELTA, xbl_OO_TEX_SCALE, xbl_TEX_DELTA);
    uploadLogoMesh(tmWordmarkMesh, tmWordmark);

    TextMesh text = DecodeTextMesh(verts_text_0C, vertex_count_text_0,
                                    indices_text_0C, index_count_text_0,
                                    xbt_OO_POS_SCALE, xbt_POS_DELTA);
    uploadTextMesh(textMesh, text);

    Image lipImg = CreateGradientMapImage(16, 128, 0xff000100, 0xff4b9b4b);
    lipTex = LoadTextureFromImage(lipImg);
    UnloadImage(lipImg);

    Image surfaceImg = CreateHighlightMapImage(256, 6, false, 0.5f, 0.5f);
    surfaceTex = LoadTextureFromImage(surfaceImg);
    UnloadImage(surfaceImg);

    Image surfaceTopImg = CreateGradientMapImage(16, 128, 0xff000000, 0xffffffff);
    surfaceTopTex = LoadTextureFromImage(surfaceTopImg);
    UnloadImage(surfaceTopImg);

    Image tmImg{};
    tmImg.width = 16;
    tmImg.height = 16;
    tmImg.mipmaps = 1;
    tmImg.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
    auto *tmPixelsOut = (uint8_t *)RL_MALLOC(16 * 16 * 4);
    tmImg.data = tmPixelsOut;
    for (int i = 0; i < 256; i++)
    {
        uint32_t argb = tm_pixels[i];
        tmPixelsOut[i * 4 + 0] = (uint8_t)((argb >> 16) & 0xff);
        tmPixelsOut[i * 4 + 1] = (uint8_t)((argb >> 8) & 0xff);
        tmPixelsOut[i * 4 + 2] = (uint8_t)(argb & 0xff);
        tmPixelsOut[i * 4 + 3] = (uint8_t)((argb >> 24) & 0xff);
    }
    tmTex = LoadTextureFromImage(tmImg);
    UnloadImage(tmImg);

#ifdef HAS_EMBED
    #ifdef __clang__
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wc23-extensions"
    #endif
    static constexpr char unlitVertData[]    = { 
        #embed "shaders/logo_unlit.vert"
        , '\0' };
    static constexpr char unlitFragData[]    = { 
        #embed "shaders/logo_unlit.frag"
        , '\0' };
    static constexpr char interiorFragData[] = { 
        #embed "shaders/logo_interior.frag"
        , '\0' };
    static constexpr char tmFragData[] = { 
        #embed "shaders/logo_tm.frag" 
        , '\0' 
    };
    static constexpr char textVertData[] = { 
        #embed "shaders/logo_text.vert" 
        , '\0' 
    };
    static constexpr char textFragData[] = { 
        #embed "shaders/logo_text.frag"
        , '\0' };
    #ifdef __clang__
    #pragma clang diagnostic pop
    #endif
    unlitShader = LoadShaderFromMemory(unlitVertData, unlitFragData);
    interiorShader = LoadShaderFromMemory(unlitVertData, interiorFragData);
    tmShader = LoadShaderFromMemory(unlitVertData, tmFragData);
    textShader = LoadShaderFromMemory(textVertData, textFragData);
#else
    unlitShader = LoadShaderFromMemory(
#include "shaders/logo_unlit.vert.inl"
    ,
#include "shaders/logo_unlit.frag.inl"
    );
    interiorShader = LoadShaderFromMemory(
#include "shaders/logo_unlit.vert.inl"
    ,
#include "shaders/logo_interior.frag.inl"
    );
    tmShader = LoadShaderFromMemory(
#include "shaders/logo_unlit.vert.inl"
    ,
#include "shaders/logo_tm.frag.inl"
    );
    textShader = LoadShaderFromMemory(
#include "shaders/logo_text.vert.inl"
    ,
#include "shaders/logo_text.frag.inl"
    );
#endif

    unlitLoc_mvp = GetShaderLocation(unlitShader, "mvp");
    unlitLoc_tex0 = GetShaderLocation(unlitShader, "tex0");

    interiorLoc_mvp = GetShaderLocation(interiorShader, "mvp");
    interiorLoc_gradStart = GetShaderLocation(interiorShader, "gradStart");
    interiorLoc_gradEnd = GetShaderLocation(interiorShader, "gradEnd");
    interiorLoc_flatStart = GetShaderLocation(interiorShader, "flatStart");
    interiorLoc_flatEnd = GetShaderLocation(interiorShader, "flatEnd");
    interiorLoc_blendW = GetShaderLocation(interiorShader, "blendW");

    tmLoc_mvp = GetShaderLocation(tmShader, "mvp");
    tmLoc_tex0 = GetShaderLocation(tmShader, "tex0");
    tmLoc_fadeAlpha = GetShaderLocation(tmShader, "fadeAlpha");

    textLoc_mvp = GetShaderLocation(textShader, "mvp");
    textLoc_flatColor = GetShaderLocation(textShader, "flatColor");

    matTextAnim = MatrixIdentity();
    renderText = false;
    tmAlpha = 0.f;
}

void LogoRenderer::unload()
{
    UnloadShader(unlitShader);
    UnloadShader(interiorShader);
    UnloadShader(tmShader);
    UnloadShader(textShader);
    UnloadTexture(lipTex);
    UnloadTexture(surfaceTex);
    UnloadTexture(surfaceTopTex);
    UnloadTexture(tmTex);
}

void LogoRenderer::advanceTime(float fElapsedTime)
{
    if (fElapsedTime >= TEXT_ANIM_START_TIME)
    {
        renderText = true;
        constexpr int nSamples = sizeof(pos_anim_text) / sizeof(pos_anim_text[0]);
        float fNormPos = (fElapsedTime - TEXT_ANIM_START_TIME) / TEXT_ANIM_LEN;
        float fPos = fNormPos * (float)(nSamples - 1);
        int posIdx = (int)fPos;

        Vector3 pos;
        if (fNormPos <= 0.f)
        {
            pos = {pos_anim_text[0].x, pos_anim_text[0].y, pos_anim_text[0].z};
        }
        else if (fNormPos >= 1.f)
        {
            const auto &p = pos_anim_text[nSamples - 1];
            pos = {p.x, p.y, p.z};
        }
        else
        {
            float frac = fPos - (float)posIdx;
            const auto &s = pos_anim_text[posIdx];
            const auto &e = pos_anim_text[posIdx + 1];
            pos.x = s.x * (1.f - frac) + e.x * frac;
            pos.y = s.y * (1.f - frac) + e.y * frac;
            pos.z = s.z * (1.f - frac) + e.z * frac;
        }
        matTextAnim = MatrixTranslate(pos.x, pos.y, pos.z);

        tmAlpha = (fElapsedTime - TEXT_ANIM_START_TIME) / TEXT_ANIM_LEN;
        tmAlpha = std::max(0.f, std::min(1.f, tmAlpha));
    }
    else
    {
        renderText = false;
        tmAlpha = 0.f;
    }
}

void LogoRenderer::render(const Matrix &matOtw, const Camera3D &camera, float fElapsedTime)
{
    Matrix view = GetCameraMatrix(camera);
    float aspect = (float)GetScreenWidth() / (float)GetScreenHeight();
    Matrix proj = MatrixPerspective(camera.fovy * DEG2RAD, aspect, 0.4f, 800.0f);
    Matrix mvp = MatrixMultiply(matOtw, MatrixMultiply(view, proj));

    BeginShaderMode(unlitShader);
    SetShaderValueMatrix(unlitShader, unlitLoc_mvp, mvp);
    SetShaderValueTexture(unlitShader, unlitLoc_tex0, lipTex);
    drawMeshRaw(lipMesh);
    SetShaderValueTexture(unlitShader, unlitLoc_tex0, surfaceTex);
    drawMeshRaw(surfaceMesh);
    SetShaderValueTexture(unlitShader, unlitLoc_tex0, surfaceTopTex);
    drawMeshRaw(surfaceTopMesh);
    EndShaderMode();

    float fmag = -1.0f + 2.0f * (fElapsedTime - SLASH_GRADIENT_TRANSITION_START) * SLASH_GRADIENT_TRANSITION_MUL;
    float w1 = std::max(0.f, std::min(1.f, -fmag));
    float w3 = std::max(0.f, std::min(1.f, fmag));
    float w2 = std::max(0.f, std::min(1.f, 1.f - w1 - w3));

    Vector4 gradStart, gradEnd, flatStart, flatEnd;
    float blendW;
    if (fmag < 0.f)
    {
        flatStart = flatEnd = {0.81568f, 1.f, 0.5921f, 1.f};
        gradStart = gradEnd = {0.81568f, 1.f, 0.5294f, 1.f};
        blendW = w1;
    }
    else
    {
        flatStart = flatEnd = {0.81568f, 1.f, 0.5294f, 1.f};
        gradStart = {0.796f, 0.8745f, 0.0039f, 1.f};
        gradEnd = {0.1294f, 0.4168f, 0.0901f, 1.f};
        blendW = w2;
    }

    BeginShaderMode(interiorShader);
    SetShaderValueMatrix(interiorShader, interiorLoc_mvp, mvp);
    SetShaderValue(interiorShader, interiorLoc_gradStart, &gradStart, SHADER_UNIFORM_VEC4);
    SetShaderValue(interiorShader, interiorLoc_gradEnd, &gradEnd, SHADER_UNIFORM_VEC4);
    SetShaderValue(interiorShader, interiorLoc_flatStart, &flatStart, SHADER_UNIFORM_VEC4);
    SetShaderValue(interiorShader, interiorLoc_flatEnd, &flatEnd, SHADER_UNIFORM_VEC4);
    SetShaderValue(interiorShader, interiorLoc_blendW, &blendW, SHADER_UNIFORM_FLOAT);
    drawMeshRaw(interiorMesh);
    EndShaderMode();

    if (!renderText) return;

    rlDisableDepthTest();
    rlDisableDepthMask();

    BeginShaderMode(tmShader);
    SetShaderValueMatrix(tmShader, tmLoc_mvp, mvp);
    SetShaderValueTexture(tmShader, tmLoc_tex0, tmTex);
    SetShaderValue(tmShader, tmLoc_fadeAlpha, &tmAlpha, SHADER_UNIFORM_FLOAT);
    drawMeshRaw(tmSlashMesh);
    drawMeshRaw(tmWordmarkMesh);
    EndShaderMode();

    Matrix flip = MatrixRotateX(PI / 2.0f);
    Matrix animOtw = MatrixMultiply(flip, MatrixMultiply(matTextAnim, matOtw));
    Matrix textMvp = MatrixMultiply(animOtw, MatrixMultiply(view, proj));
    Vector4 wordmarkColor = {0x62 / 255.f, 0xca / 255.f, 0x13 / 255.f, 1.0f};

    BeginShaderMode(textShader);
    SetShaderValueMatrix(textShader, textLoc_mvp, textMvp);
    SetShaderValue(textShader, textLoc_flatColor, &wordmarkColor, SHADER_UNIFORM_VEC4);
    drawMeshRaw(textMesh);
    EndShaderMode();

    rlEnableDepthTest();
    rlEnableDepthMask();
}