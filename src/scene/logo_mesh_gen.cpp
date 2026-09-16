#include "logo_mesh_gen.h"

std::vector<uint16_t> DecompressLogoIndices(const char *raw, int count)
{
    std::vector<uint16_t> out(count);
    out[0] = (uint16_t)(int8_t)raw[0];
    for (int i = 1; i < count; i++)
    {
        if (raw[i] == 126)
        {
            int8_t hi = raw[i + 1];
            int8_t lo = raw[i + 2];
            int16_t delta = (int16_t)(((((int16_t)hi) & 0xff) << 8) | (((int16_t)lo) & 0xff));
            out[i] = (uint16_t)(out[i - 1] + delta);
            raw += 2;
        }
        else
        {
            out[i] = (uint16_t)(out[i - 1] + raw[i]);
        }
    }
    return out;
}

LogoMesh DecodeLogoMesh(const short *rawVerts, int vertCount, const char *rawIndices, int indexCount,
                         float ooPosScale, float posDelta, float ooTexScale, float texDelta)
{
    LogoMesh mesh;
    mesh.vertices.resize(vertCount);
    for (int i = 0; i < vertCount; i++)
    {
        const short *rv = rawVerts + i * 5;
        LogoVertex &v = mesh.vertices[i];
        v.pos.x = (float)rv[0] * ooPosScale + posDelta;
        v.pos.y = (float)rv[1] * ooPosScale + posDelta;
        v.pos.z = (float)rv[2] * ooPosScale + posDelta;
        v.u = (float)rv[3] * ooTexScale + texDelta;
        v.v = (float)rv[4] * ooTexScale + texDelta;
    }
    mesh.indices = DecompressLogoIndices(rawIndices, indexCount);
    return mesh;
}

TextMesh DecodeTextMesh(const short *rawVerts, int vertCount, const char *rawIndices, int indexCount,
                         float ooPosScale, float posDelta)
{
    TextMesh mesh;
    mesh.vertices.resize(vertCount);
    for (int i = 0; i < vertCount; i++)
    {
        const short *rv = rawVerts + i * 3;
        mesh.vertices[i].pos = {
            (float)rv[0] * ooPosScale + posDelta,
            (float)rv[1] * ooPosScale + posDelta,
            (float)rv[2] * ooPosScale + posDelta,
        };
    }
    mesh.indices = DecompressLogoIndices(rawIndices, indexCount);
    return mesh;
}