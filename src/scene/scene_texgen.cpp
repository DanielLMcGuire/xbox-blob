#include "scene_texgen.h"

#include "raymath.h"

#include "../util/qrand.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <vector>

Image CreateIntensityMapImage(
    int size,
    bool convertToNormalMap,
    float heightScale,
    int noise,
    int seed,
    unsigned int clrMask,
    int intensitySeed,
    bool useIntensitySeed,
    unsigned int intensityMax,
    int negativeProb)
{
    std::vector<uint32_t> intensity((size_t)size * size, 0);

    QRand rng;
    rng.Init(seed);
    uint32_t i0 = useIntensitySeed ? (uint32_t)intensitySeed : (uint32_t)rng.Rand(intensityMax);
    intensity[0] = i0;

    int curSize = size >> 1;
    int curX = curSize;
    int curY = curSize;
    int curNoise = noise >> 1;
    int curStep = size;
    bool bSquare = true;
    bool bSecondPass = false;

    while (curSize > 0)
    {
        int lx = curX - curSize, rx = curX + curSize;
        int ly = curY - curSize, uy = curY + curSize;
        if (lx < 0) lx += size;
        if (rx >= size) rx -= size;
        if (ly < 0) ly += size;
        if (uy >= size) uy -= size;

        if (bSquare)
        {
            uint32_t crnSW = intensity[(size_t)size * ly + lx];
            uint32_t crnSE = intensity[(size_t)size * ly + rx];
            uint32_t crnNW = intensity[(size_t)size * uy + lx];
            uint32_t crnNE = intensity[(size_t)size * uy + rx];
            uint32_t dwI = ((crnSW & 0xff) + (crnSE & 0xff) + (crnNW & 0xff) + (crnNE & 0xff)) >> 2;
            if ((uint32_t)rng.Rand(100) > (uint32_t)negativeProb)
            {
                dwI += (uint32_t)rng.Rand(curNoise);
                if (dwI > intensityMax) dwI = intensityMax;
            }
            else
            {
                dwI -= (uint32_t)rng.Rand(curNoise);
                if (dwI > 255) dwI = 0;
            }
            intensity[(size_t)size * curY + curX] = dwI;
            curX += curStep;
            if (curX >= size)
            {
                curY += curStep;
                if (curY >= size)
                {
                    curX = curSize;
                    curY = 0;
                    bSquare = false;
                    continue;
                }
                curX = curSize;
            }
        }
        else
        {
            uint32_t crnN = intensity[(size_t)size * uy + curX];
            uint32_t crnS = intensity[(size_t)size * ly + curX];
            uint32_t crnW = intensity[(size_t)size * curY + lx];
            uint32_t crnE = intensity[(size_t)size * curY + rx];
            uint32_t dwI = ((crnN & 0xff) + (crnS & 0xff) + (crnE & 0xff) + (crnW & 0xff)) >> 2;
            if ((uint32_t)rng.Rand(100) > (uint32_t)negativeProb)
            {
                dwI += (uint32_t)rng.Rand(curNoise);
                if (dwI > intensityMax) dwI = intensityMax;
            }
            else
            {
                dwI -= (uint32_t)rng.Rand(curNoise);
                if (dwI > 255) dwI = 0;
            }
            intensity[(size_t)size * curY + curX] = dwI;
            curX += curStep;
            if (curX >= size)
            {
                curY += curStep;
                if (curY >= size)
                {
                    if (bSecondPass)
                    {
                        curStep = curSize;
                        curSize >>= 1;
                        curNoise >>= 1;
                        curX = curSize;
                        curY = curSize;
                        bSquare = true;
                    }
                    else
                    {
                        curX = 0;
                        curY = curSize;
                    }
                    bSecondPass = !bSecondPass;
                    continue;
                }
                curX = bSecondPass ? 0 : curSize;
            }
        }
    }

    (void)clrMask;

    Image img{};
    img.width = size;
    img.height = size;
    img.mipmaps = 1;
    img.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
    auto *pixels = (uint8_t *)RL_MALLOC((size_t)size * size * 4);
    img.data = pixels;

    if (!convertToNormalMap)
    {
        for (int p = 0; p < size * size; p++)
        {
            uint8_t v = (uint8_t)intensity[p];
            pixels[p * 4 + 0] = v;
            pixels[p * 4 + 1] = v;
            pixels[p * 4 + 2] = v;
            pixels[p * 4 + 3] = 255;
        }
        return img;
    }

    for (int y = 0; y < size; y++)
    {
        int y1 = (y + 1) % size;
        for (int x = 0; x < size; x++)
        {
            int x1 = (x + 1) % size;
            float h00 = (float)intensity[(size_t)size * y + x] * heightScale;
            float h10 = (float)intensity[(size_t)size * y + x1] * heightScale;
            float h01 = (float)intensity[(size_t)size * y1 + x] * heightScale;

            Vector3 v10 = {0.1f, 0.0f, h10 - h00};
            Vector3 v01 = {0.0f, 0.1f, h01 - h00};
            Vector3 n = Vector3Normalize(Vector3CrossProduct(v10, v01));

            int p = y * size + x;
            pixels[p * 4 + 0] = (uint8_t)((n.x + 1.0f) * 127.5f);
            pixels[p * 4 + 1] = (uint8_t)((n.y + 1.0f) * 127.5f);
            pixels[p * 4 + 2] = (uint8_t)((n.z + 1.0f) * 127.5f);
            pixels[p * 4 + 3] = 255;
        }
    }
    return img;
}

Image CreateGradientMapImage(int width, int height, unsigned int argbStart, unsigned int argbEnd)
{
    float aStart = ((argbStart >> 24) & 0xff) / 255.f, aEnd = ((argbEnd >> 24) & 0xff) / 255.f;
    float rStart = ((argbStart >> 16) & 0xff) / 255.f, rEnd = ((argbEnd >> 16) & 0xff) / 255.f;
    float gStart = ((argbStart >> 8) & 0xff) / 255.f, gEnd = ((argbEnd >> 8) & 0xff) / 255.f;
    float bStart = (argbStart & 0xff) / 255.f, bEnd = (argbEnd & 0xff) / 255.f;

    Image img{};
    img.width = width;
    img.height = height;
    img.mipmaps = 1;
    img.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
    auto *pixels = (uint8_t *)RL_MALLOC((size_t)width * height * 4);
    img.data = pixels;

    for (int y = 0; y < height; y++)
    {
        float t = (float)y / (float)(height - 1);
        uint8_t r = (uint8_t)((rStart * (1.f - t) + rEnd * t) * 255.f);
        uint8_t g = (uint8_t)((gStart * (1.f - t) + gEnd * t) * 255.f);
        uint8_t b = (uint8_t)((bStart * (1.f - t) + bEnd * t) * 255.f);
        uint8_t a = (uint8_t)((aStart * (1.f - t) + aEnd * t) * 255.f);
        for (int x = 0; x < width; x++)
        {
            uint8_t *p = pixels + ((size_t)y * width + x) * 4;
            p[0] = r; p[1] = g; p[2] = b; p[3] = a;
        }
    }
    return img;
}

Image CreateHighlightMapImage(int size, int power, bool falloffAlpha, float fLinearW, float fCosW)
{
    std::vector<uint8_t> cosTable(257);
    for (int i = 0; i < 256; i++)
    {
        float c = cosf((float)i / 256.f);
        for (int k = power; k; --k) c *= c;
        float sum = 255.f * (c * fCosW + ((float)(256 - i) / 256.f) * fLinearW);
        if (sum < 0.f) sum = 0.f;
        if (sum > 255.f) sum = 255.f;
        cosTable[i] = (uint8_t)sum;
    }

    Image img{};
    img.width = size;
    img.height = size;
    img.mipmaps = 1;
    img.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
    auto *pixels = (uint8_t *)RL_MALLOC((size_t)size * size * 4);
    img.data = pixels;

    float ooRadius = 1.0f / (float)(size / 2);
    int cntr = (size - 1) / 2;
    for (int y = 0; y < size; y++)
    {
        for (int x = 0; x < size; x++)
        {
            float dist = sqrtf((float)((x - cntr) * (x - cntr) + (y - cntr) * (y - cntr))) * ooRadius;
            uint8_t *p = pixels + ((size_t)y * size + x) * 4;
            if (dist < 1.f)
            {
                int idx = (int)(dist * 256.f);
                if (idx > 256) idx = 256;
                uint8_t c = cosTable[idx];
                p[0] = c; p[1] = c; p[2] = c;
                p[3] = falloffAlpha ? c : 255;
            }
            else
            {
                p[0] = p[1] = p[2] = 0;
                p[3] = falloffAlpha ? 0 : 255;
            }
        }
    }
    return img;
}

std::vector<Image> CreatePlasmaMapImages(int num, int size, int noise, int seed, int intensitySeed, int intensityMax)
{
    if (num > 3) num = 3;

    size_t texSize = (size_t)size * (size_t)size;
    std::vector<uint8_t> pixels(texSize * (size_t)num, 0);
    pixels[0] = (uint8_t)intensitySeed;

    QRand rng;
    rng.Init(seed);

    int curSize = size >> 1;
    int curX = curSize;
    int curY = curSize;
    int curNoise = noise >> 1;
    int curStep = size;
    bool bSquare = true;
    bool bSecondPass = false;

    while (curSize > 0)
    {
        int lx = curX - curSize, rx = curX + curSize;
        int ly = curY - curSize, uy = curY + curSize;
        if (lx < 0) lx += size;
        if (rx >= size) rx -= size;
        if (ly < 0) ly += size;
        if (uy >= size) uy -= size;

        if (bSquare)
        {
            for (int t = 0; t < num; t++)
            {
                int crnSW = pixels[(size_t)texSize * t + (size_t)size * ly + lx];
                int crnSE = pixels[(size_t)texSize * t + (size_t)size * ly + rx];
                int crnNW = pixels[(size_t)texSize * t + (size_t)size * uy + lx];
                int crnNE = pixels[(size_t)texSize * t + (size_t)size * uy + rx];

                int dwI = (crnSW + crnSE + crnNW + crnNE) >> 2;
                dwI += rng.Rand(curNoise * 2) - curNoise;

                pixels[(size_t)texSize * t + (size_t)size * curY + curX] =
                    (uint8_t)std::max(0, std::min(intensityMax, dwI));
            }

            curX += curStep;
            if (curX >= size)
            {
                curY += curStep;
                if (curY >= size)
                {
                    curX = curSize;
                    curY = 0;
                    bSquare = false;
                    continue;
                }
                curX = curSize;
            }
        }
        else
        {
            for (int t = 0; t < num; t++)
            {
                int crnN = pixels[(size_t)texSize * t + (size_t)size * uy + curX];
                int crnS = pixels[(size_t)texSize * t + (size_t)size * ly + curX];
                int crnW = pixels[(size_t)texSize * t + (size_t)size * curY + lx];
                int crnE = pixels[(size_t)texSize * t + (size_t)size * curY + rx];

                int dwI = ((crnN & 0xff) + (crnS & 0xff) + (crnE & 0xff) + (crnW & 0xff)) >> 2;
                dwI += rng.Rand(curNoise * 2) - curNoise;

                pixels[(size_t)texSize * t + (size_t)size * curY + curX] =
                    (uint8_t)std::max(0, std::min(intensityMax, dwI));
            }

            curX += curStep;
            if (curX >= size)
            {
                curY += curStep;
                if (curY >= size)
                {
                    if (bSecondPass)
                    {
                        curStep = curSize;
                        curSize >>= 1;
                        curNoise >>= 1;
                        curX = curSize;
                        curY = curSize;
                        bSquare = true;
                    }
                    else
                    {
                        curX = 0;
                        curY = curSize;
                    }

                    bSecondPass = !bSecondPass;
                    continue;
                }

                curX = bSecondPass ? 0 : curSize;
            }
        }
    }

    std::vector<Image> images(num);

    for (int t = 0; t < num; t++)
    {
        Image &img = images[t];
        img.width = size;
        img.height = size;
        img.mipmaps = 1;
        img.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;

        auto *out = (uint8_t *)RL_MALLOC(texSize * 4);
        img.data = out;

        for (size_t p = 0; p < texSize; p++)
        {
            out[p * 4 + 0] = 255;
            out[p * 4 + 1] = 255;
            out[p * 4 + 2] = 255;
            out[p * 4 + 3] = pixels[(size_t)texSize * t + p];
        }
    }

    return images;
}

