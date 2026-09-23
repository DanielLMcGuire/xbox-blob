#pragma once

#include "sos_common.h"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace SOS::Waveform
{

inline std::vector<float> sine(size_t n)
{
    std::vector<float> d(n);
    for (size_t i = 0; i < n; i++)
        d[i] = std::sin(2.0f * kPi * (float)i / (float)n);
    return d;
}

inline std::vector<float> saw(size_t n)
{
    std::vector<float> d(n);
    const int half = (int)(n / 2);
    for (size_t i = 0; i < n; i++)
        d[i] = (float)((int)i - half) / (float)half;
    return d;
}

inline std::vector<float> noise(size_t n, uint32_t seed)
{
    std::vector<float> d(n);
    uint32_t holdrand = seed;
    for (size_t i = 0; i < n; i++)
    {
        holdrand = holdrand * 214013L + 2531011L;
        int16_t r = (int16_t)((holdrand >> 16) & 0x7fff);
        d[i] = ((float)r - 16384.0f) / 16384.0f;
    }
    return d;
}

}
