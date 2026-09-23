#pragma once

#include "sos_common.h"

#include <cstddef>
#include <cstdint>

namespace SOS 
{

struct LoopFrame
{
    int count = 0;
    union
    {
        size_t returnIndex = 0;
        size_t addr;
    };
};

struct Track
{
    const int16_t* bytecode = nullptr;
    size_t length = 0;
    size_t pc = 0;

    int32_t timer = 0;
    int16_t pitch = 0;
    int16_t transpose = 0;
    int16_t volume = 0;
    int16_t filterCutoff = 0;
    uint16_t filterRes = 0;
    int patchIdx = 0;
    bool active = false;

    LoopFrame loopStack[MAX_LOOP_DEPTH]{};
    size_t loopDepth = 0;

    inline bool canRead(size_t n = 1) const {
        return pc + n <= length;
    }

    inline int16_t read() {
        return (pc < length) ? bytecode[pc++] : 0;
    }
};

}