#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include "raymath.h"

#include "sos_common.h"
#include "sos_envelope.h"
#include "sos_sample.h"


namespace SOS 
{

class Voice
{
public:
    const Patch* patch = nullptr;
    EnvelopeGenerator ampEG;
    EnvelopeGenerator multiEG;

    double samplePos = 0.0;
    int16_t currentPitch = 0;
    int16_t filterCutoff = 0;
    uint16_t filterRes = 0;

    float channelGain = 1.0f;
    float panLeft = 1.0f;
    float panRight = 1.0f;
    bool active = false;

    float y1 = 0.0f;
    float y2 = 0.0f;

    inline void setPatch(const Patch* p, float sampleRate)
    {
        patch = p;
        if (p)
        {
            ampEG.setDesc(p->ampEnv, sampleRate);
            multiEG.setDesc(p->multiEnv, sampleRate);
        }
        samplePos = 0.0;
    }

    inline void noteOn(int16_t pitch)
    {
        currentPitch = pitch;
        samplePos = 0.0;
        ampEG.trigger();
        multiEG.trigger();
        y1 = 0.0f;
        y2 = 0.0f;
        active = true;
    }

    inline void setPitch(int16_t pitch) { currentPitch = pitch; }
    inline void setFilter(int16_t cut, uint16_t res) { filterCutoff = cut; filterRes = res; }
    inline void noteOff() { ampEG.release(); multiEG.release(); }

    inline void setVolume(int16_t vol)
    {
        float mB = (float)(-vol * 30 + 200);
        channelGain = std::pow(10.0f, (mB / 100.0f) / 20.0f);
    }

    inline void renderBlock(float* outL, float* outR, int numFrames, float sampleRate)
    {
        if (!active || !patch) return;

        int framesRendered = 0;
        while (framesRendered < numFrames) {
            int framesToProcess = std::min(numFrames - framesRendered, CONTROL_INTERVAL);

            float aLvl = ampEG.processBlock(framesToProcess);
            float mLvl = multiEG.processBlock(framesToProcess);

            if (ampEG.state == EnvelopeGenerator::OFF)
            {
                active = false;
                return;
            }

            int32_t noteNum  = (int16_t)(currentPitch >> 8);
            int32_t noteFrac = (int16_t)(currentPitch & 0xFF);
            float basePitch  = (noteNum - 60) * (4096.0f / 12.0f) + (noteFrac * 341.0f) / 255.0f;
            float totalPitch = basePitch + (multiEG.pitchScale * 32.0f) * mLvl;
            float speed      = std::pow(2.0f, totalPitch / 4096.0f);

            float cutUnits = (float)filterCutoff + (multiEG.filterCutoff * 32.0f) * mLvl;
            float octaves  = cutUnits - 32768.0f;
            float fc       = 8000.0f * std::pow(2.0f, octaves / 4096.0f);
            fc = std::clamp(fc, 20.0f, sampleRate * 0.45f);

            float r = std::clamp((float)filterRes / 32768.0f, 0.0f, 0.985f);
            float theta = (2.0f * PI * fc) / sampleRate;
            float b1 = -2.0f * r * std::cos(theta);
            float b2 = r * r;
            float K  = 1.0f + b1 + b2;

            float gain = aLvl * channelGain;

            for (int i = 0; i < framesToProcess; ++i) {
                float raw = patch->sample.read(samplePos);
                samplePos += speed;
                if (!patch->sample.loop && samplePos >= patch->sample.data.size())
                {
                    active = false;
                    return;
                }

                float filtered = K * raw - b1 * y1 - b2 * y2;
                y2 = y1;
                y1 = filtered;

                float s = filtered * gain;
                outL[framesRendered + i] += s * panLeft;
                outR[framesRendered + i] += s * panRight;
            }

            framesRendered += framesToProcess;
        }
    }
};

}