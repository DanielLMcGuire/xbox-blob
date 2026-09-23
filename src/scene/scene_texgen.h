#pragma once

#include "raylib.h"

#include <vector>

Image CreateIntensityMapImage(
    int size,
    bool convertToNormalMap = false,
    float heightScale = 1.f / 512.f,
    int noise = 1024,
    int seed = 0,
    unsigned int clrMask = 0x00ffffff,
    int intensitySeed = 255,
    bool useIntensitySeed = false,
    unsigned int intensityMax = 255,
    int negativeProb = 50);

Image CreateGradientMapImage(int width, int height, unsigned int argbStart, unsigned int argbEnd);

Image CreateHighlightMapImage(int size, int power, bool falloffAlpha, float fLinearW, float fCosW);

std::vector<Image> CreatePlasmaMapImages(int num, int size, int noise, int seed, int intensitySeed, int intensityMax);
