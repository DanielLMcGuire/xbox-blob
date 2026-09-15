#include "scene_texgen.h"
#include "../util/qrand.h"
#include "raymath.h"
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
