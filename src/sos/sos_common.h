#pragma once

#include <cstddef>
#include <cstdint>

#ifndef kPi
#define kPi 3.141592653589793
#endif

namespace SOS 
{

constexpr int    SAMPLE_RATE      = 48000;
constexpr int    SAMPLES_PER_TICK = 240;
constexpr int    MAX_TRACKS       = 12;
constexpr size_t MAX_LOOP_DEPTH   = 8;
constexpr int    CONTROL_INTERVAL = 16;
constexpr float  PAUSE_FADE_SECONDS = 0.02f;
constexpr float  SEEK_CHECKPOINT_SECONDS = 0.5f;
constexpr int    SEEK_BLOCK_FRAMES = 4800;
constexpr double SEEK_MAX_SECONDS = 30.0;

enum OpCode
{
    F_REST = 0, F_NOTE = 1, F_JUMPTO = 2, F_LOOP = 3, F_ENDLOOP = 4,
    F_PATCH = 5, F_PAN = 6, F_MUX = 7, F_DEMUX = 8, F_VOLUME = 9,
    F_XPOSE = 10, F_XSET = 11, F_SLUR = 12, F_RING = 13, F_CLOCKSET = 14,
    F_END = 15, F_FILTERINC = 16, F_FILTERSET = 17
};


constexpr float DEFAULT_TICK_SECONDS = 0.005f;

constexpr int opArity(int op) noexcept
{
    switch (op)
    {
    case F_REST:      return 1;
    case F_NOTE:      return 2;
    case F_LOOP:      return 1;
    case F_ENDLOOP:   return 0;
    case F_PATCH:     return 1;
    case F_VOLUME:    return 1;
    case F_XPOSE:     return 1;
    case F_SLUR:      return 2;
    case F_RING:      return 1;
    case F_END:       return 0;
    case F_FILTERINC: return 2;
    case F_FILTERSET: return 2;
    default:          return -1;
    }
}

}
