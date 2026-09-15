#pragma once
#include "raylib.h"

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
