#pragma once

#include "sos_common.h"
#include "sos_sample.h"

#include <cmath>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace SOS {

struct Pan
{
    float left  = 1.0f;
    float right = 1.0f;

    static float gainFromMillibels(float mB) { return std::pow(10.0f, mB / 2000.0f); }

    static Pan fromMillibels(float leftMB, float rightMB)
    {
        return { gainFromMillibels(leftMB), gainFromMillibels(rightMB) };
    }
};

struct TrackDesc
{
    std::vector<int16_t> code;
    Pan pan;
};

struct Program;

using ProgramPtr = std::shared_ptr<const Program>;

struct Program
{
    std::vector<Patch>     patches;
    std::vector<TrackDesc> tracks;
    float tickSeconds = DEFAULT_TICK_SECONDS;

    bool validate(std::string* error = nullptr) const;
};

inline ProgramPtr makeProgram(Program program)
{
    return std::make_shared<const Program>(std::move(program));
}

}
