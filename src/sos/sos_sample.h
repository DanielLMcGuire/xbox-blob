#pragma once

#include <cmath>
#include <vector>

#include "sos_envelope.h"

struct SampleBuffer {
    std::vector<float> data;
    bool loop = false;

    inline float read(double pos) const {
        if (data.empty()) return 0.0f;
        if (loop) {
            pos = std::fmod(pos, (double)data.size());
            if (pos < 0) pos += data.size();
            size_t i0 = (size_t)pos;
            size_t i1 = (i0 + 1) % data.size();
            return data[i0] + (float)(pos - i0) * (data[i1] - data[i0]);
        } else {
            if (pos < 0.0) return 0.0f;
            size_t i0 = (size_t)pos;
            if (i0 >= data.size()) return 0.0f;
            if (i0 + 1 >= data.size()) return data[i0];
            return data[i0] + (float)(pos - i0) * (data[i0 + 1] - data[i0]);
        }
    }
};

struct Patch {
    SampleBuffer sample;
    const DSENVELOPEDESC* ampEnv = nullptr;
    const DSENVELOPEDESC* multiEnv = nullptr;
};
