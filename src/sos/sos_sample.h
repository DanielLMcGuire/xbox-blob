#pragma once

#include "sos_envelope.h"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace SOS {

struct SampleBuffer
{
    std::vector<float> data;
    bool loop = false;

    static SampleBuffer fromFloat(std::vector<float> pcm, bool loop)
    {
        SampleBuffer s;
        s.data = std::move(pcm);
        s.loop = loop;
        return s;
    }

    static SampleBuffer fromPcm8(const uint8_t* src, size_t count, bool loop, bool offsetBinary = false)
    {
        SampleBuffer s;
        s.loop = loop;
        s.data.resize(count);
        for (size_t i = 0; i < count; i++)
        {
            uint16_t v = offsetBinary ? ((uint16_t)(src[i] ^ 0x80) << 8) : ((uint16_t)src[i] << 8);
            s.data[i] = (float)(int16_t)v / 32768.0f;
        }
        return s;
    }

    static SampleBuffer fromPcm16(const int16_t* src, size_t count, bool loop)
    {
        SampleBuffer s;
        s.loop = loop;
        s.data.resize(count);
        for (size_t i = 0; i < count; i++)
            s.data[i] = (float)src[i] / 32768.0f;
        return s;
    }

    inline float read(double pos) const
    {
        if (data.empty()) return 0.0f;
        if (loop)
        {
            pos = std::fmod(pos, (double)data.size());
            if (pos < 0) pos += data.size();
            size_t i0 = (size_t)pos;
            size_t i1 = (i0 + 1) % data.size();
            return data[i0] + (float)(pos - i0) * (data[i1] - data[i0]);
        } 
        else
        {
            if (pos < 0.0) return 0.0f;
            size_t i0 = (size_t)pos;
            if (i0 >= data.size()) return 0.0f;
            if (i0 + 1 >= data.size()) return data[i0];
            return data[i0] + (float)(pos - i0) * (data[i0 + 1] - data[i0]);
        }
    }
};

struct Patch
{
    SampleBuffer sample;
    DSENVELOPEDESC ampEnv = OpenEnva;
    DSENVELOPEDESC multiEnv = OpenEnvm;
};

}
