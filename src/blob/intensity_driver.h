#pragma once
#include "../defines.h"
#include "../util/qrand.h"
#include "blob.h"

class BlobIntensityDriver
{
public:
    BlobIntensityDriver(int32_t seed = defSeed) : rng(seed), startSeed(seed) { Init(); }

    void Init();
    bool Advance(float dt, Blob& blob);

    void Reset(Blob& blob);
    void Seek(float targetTime, Blob& blob);

    inline float GetElapsedTime() const { return timeElapsed; }
    inline float GetBaseIntensity() const { return baseIntensity; }
    inline float GetIntensity() const { return intensity; }
    inline float GetPulseIntensity() const { return intensity - baseIntensity; }

    bool loop = true;

private:
    enum { NUM_PULSES = 12 };
    Vector3 pulses[NUM_PULSES]{};

    QRand rng;
    int32_t startSeed = defSeed;
    float timeElapsed = 0.0f;
    float baseIntensity = 0.0f;
    float intensity = 0.0f;
    float smoothedIntensity = 0.0f;
    float iidt = 0.0f;

    inline float Rand01() { static const float mul = 1.0f / 65536.0f; return (float)(rng.Rand() & 0xFFFF) * mul; }
    inline float Rand11() { static const float mul = 2.0f / 65536.0f; return (float)(rng.Rand() & 0xFFFF) * mul - 1.0f; }

    void InitPulses();

    float SumPulses(float et) const;
};