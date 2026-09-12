#pragma once

#include <cstdint>
#include "ds_envelope.h"


namespace SOS {

inline constexpr DSENVELOPEDESC Env1a      = { 1, 1, 0, 0x1,   0x5,   0x20,  0x0,  0x7f, 0,    0 };
inline constexpr DSENVELOPEDESC Env1m      = { 0, 1, 0, 0x1,   0x0,   0x10,  0x0,  0x1f, 0x10, 0x7f };
inline constexpr DSENVELOPEDESC SawEnv1a   = { 1, 1, 0, 0x1,   0x2,   0x10,  0x0,  0x9f, 0,    0 };
inline constexpr DSENVELOPEDESC SawEnv1m   = { 0, 1, 0, 0x10,  0x100, 0x100, 0x80, 0xff, 0,    -80 };
inline constexpr DSENVELOPEDESC SawEnv2a   = { 1, 1, 0, 0x1,   0x0,   0x40,  0x0,  0x3f, 0x7f, 0 };
inline constexpr DSENVELOPEDESC SawEnv2m   = { 0, 1, 0, 0x100, 0x0,   0x10,  0x0,  0x1f, 0,    0 };
inline constexpr DSENVELOPEDESC NoiseEnv1a = { 1, 1, 0, 0x1,   0x3,   0x10,  0x20, 0xff, 0,    0 };
inline constexpr DSENVELOPEDESC NoiseEnv1m = { 0, 1, 0, 0x100, 0x0,   0x30,  0xc0, 0xff, 0,    0 };
inline constexpr DSENVELOPEDESC Env3a      = { 1, 1, 0, 0x1,   0x3,   0x10,  0x20, 0x10, 0,    0 };
inline constexpr DSENVELOPEDESC Env3m      = { 0, 1, 0, 0x1,   0x0,   0x10,  0x0,  0x1f, 0x10, 0x1f };
inline constexpr DSENVELOPEDESC OpenEnva   = { 1, 1, 0, 0x0,   0x0,   0x0,   0x0,  0xff, 0,    0 };
inline constexpr DSENVELOPEDESC OpenEnvm   = { 0, 1, 0, 0,     0x0,   0x0,   0x0,  0xff, 0,    0 };

struct EnvelopeGenerator
{
    enum State { OFF, DELAY, ATTACK, HOLD, DECAY, SUSTAIN, RELEASE } state = OFF;
    
    float level = 0.0f;
    uint32_t count = 0;
    uint32_t delaySamples = 0;
    uint32_t attackSamples = 0;
    uint32_t holdSamples = 0;
    uint32_t decaySamples = 0;
    uint32_t releaseSamples = 0;
    
    float sustainLevel = 1.0f;
    float releaseStart = 0.0f;
    int32_t pitchScale = 0;
    int32_t filterCutoff = 0;

    void setDesc(const DSENVELOPEDESC* desc, float sampleRate);
    void trigger();
    void release();
    float processBlock(int stepSamples);
};

}