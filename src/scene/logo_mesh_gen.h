#pragma once

#include "raylib.h"

#include <cstdint>
#include <vector>

struct LogoVertex
{
    Vector3 pos;
    float u = 0.f, v = 0.f;
};

struct LogoMesh
{
    std::vector<LogoVertex> vertices;
    std::vector<uint16_t> indices;
};

struct TextVertex
{
    Vector3 pos;
};

struct TextMesh
{
    std::vector<TextVertex> vertices;
    std::vector<uint16_t> indices;
};

std::vector<uint16_t> DecompressLogoIndices(const char *raw, int count);

LogoMesh DecodeLogoMesh(const short *rawVerts, int vertCount, const char *rawIndices, int indexCount,
                         float ooPosScale, float posDelta, float ooTexScale, float texDelta);

TextMesh DecodeTextMesh(const short *rawVerts, int vertCount, const char *rawIndices, int indexCount,
                         float ooPosScale, float posDelta);
