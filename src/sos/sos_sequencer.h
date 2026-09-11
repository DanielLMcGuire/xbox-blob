#pragma once

#include <cstdint>
#include <vector>

#include "sos_common.h"
#include "sos_envelope.h"
#include "sos_sample.h"
#include "sos_track.h"
#include "sos_voice.h"

class SOSSequencer {
private:
    Patch patches[11];
    Voice voices[MAX_TRACKS];
    Track tracks[MAX_TRACKS];

    float sampleRate = 48000.0f;
    float samplesPerTick = 240.0f;
    float tickCountdown = 1.0f;

    std::vector<float> monoMixL;
    std::vector<float> monoMixR;

    void initPatches();
    void initPanning();
    void stepTrack(int ch);
    void tick();

public:
    SOSSequencer();

    void setSampleRate(float sr);
    void startBootSound();
    void render(float* output, int frameCount);
};