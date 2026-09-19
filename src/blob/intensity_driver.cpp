#include "intensity_driver.h"
#include "../defines.h"
#include "../util/qrand.h"
#include "blob.h"

#include <cmath>
#include <algorithm>

void BlobIntensityDriver::Init()
{
    timeElapsed = 0.0f;
    smoothedIntensity = intensity = baseIntensity = DEMO_START_INTENSITY;
    iidt = 0.0f;
    InitPulses();
}

void BlobIntensityDriver::Reset(Blob& blob)
{
    iidt = 0.0f;
    smoothedIntensity = intensity = baseIntensity = DEMO_START_INTENSITY;
    timeElapsed = 0.0f;
    
    rng.SetSeed(startSeed);
    InitPulses();
    BlobSetRandomSeed(startSeed);
    blob.Restart();
}

bool BlobIntensityDriver::Advance(float dt, Blob& blob)
{
    if (dt > 1.0f) dt = 0.001f;
    timeElapsed += dt;

    if (timeElapsed < BLOB_ZERO_INTENSE_END_TIME)
    {
        baseIntensity = 0.0f;
    }
    else
    {
        float t = (timeElapsed - BLOB_ZERO_INTENSE_END_TIME) * OO_MAX_INTENSITY_DELTA;
        t = 0.5f * t * t + 0.5f * t;
        baseIntensity = DEMO_START_INTENSITY + t * (1.0f - DEMO_START_INTENSITY);
    }

    float pulses = SumPulses(timeElapsed);
    intensity = baseIntensity + pulses;

    float s = 0.5f * dt;
    smoothedIntensity = (1.0f - s) * smoothedIntensity + s * intensity;
    iidt += dt * intensity;

    if (timeElapsed >= DEMO_TOTAL_TIME)
    {
        if (!loop) return false;
        Reset(blob);
    }

    blob.AdvanceTime(timeElapsed, dt);
    return true;
}

void BlobIntensityDriver::Seek(float targetTime, Blob& blob)
{
    targetTime = std::max(0.0f, targetTime);
    if (!loop) {
        targetTime = std::min(targetTime, DEMO_TOTAL_TIME);
    } else {
        targetTime = std::fmod(targetTime, DEMO_TOTAL_TIME);
    }

    Reset(blob);

    const float SIM_STEP = 1.0f / 60.0f;
    while (timeElapsed + SIM_STEP < targetTime)
    {
        Advance(SIM_STEP, blob);
    }

    float remainder = targetTime - timeElapsed;
    if (remainder > 0.0f)
    {
        Advance(remainder, blob);
    }
}

void BlobIntensityDriver::InitPulses()
{
    for (int i = 0; i < NUM_PULSES; i++)
    {
        float x = (float)(i + 1) / (float)(NUM_PULSES + 1) + Rand11() * 0.03f;
        x = 1.0f - (0.5f * (x * x) + 0.5f * x);

        float temp = ((1.2f - x) * (1.2f - x)) * (Rand01() + 2.0f) * 0.05f;
        float y = std::max(0.1f, temp);
        float z = (x + 0.5f) * (Rand01() + 1.0f) * 0.2f;

        x = x * BLOB_PULSE_ELAPSED + BLOB_PULSE_START;
        x = std::max(x, BLOB_PULSE_START + y);

        pulses[i] = { x, y, z };
    }
    pulses[NUM_PULSES - 1].x = BLOB_PULSE_START + pulses[NUM_PULSES - 1].y;
    pulses[NUM_PULSES - 1].z *= 3.0f;
}

float BlobIntensityDriver::SumPulses(float et) const
{
    float sum = 0.0f;
    for (int i = 0; i < NUM_PULSES; i++)
    {
        float fdt = fabsf(et - pulses[i].x);
        if (fdt > pulses[i].y) continue;
        float c = cosf(fdt * 0.5f * PI / pulses[i].y);
        sum += pulses[i].z * c;
    }
    return sum;
}