#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>

#include "sos_common.h"
#include "sos_envelope.h"
#include "sos_sample.h"
#include "sos_track.h"
#include "sos_voice.h"


namespace SOS 
{

class Sequencer
{
public:
    Sequencer() { initPatches(); initPanning(); }

    void setSampleRate(float sr);
    void startBootSound();
    void render(float* output, int frameCount);

    inline void setPaused(bool p) { paused.store(p, std::memory_order_relaxed); }
    inline bool isPaused() const  { return paused.load(std::memory_order_relaxed); }

    void seek(double seconds);
    void seekImmediate(double seconds);

private:
    struct Snapshot
    {
        Voice voices[MAX_TRACKS];
        Track tracks[MAX_TRACKS];
        float tickCountdown = 0.0f;
    };

    struct SeekState : Snapshot
    {
        int     patchIdx[MAX_TRACKS]{};
        int64_t leadInFrames = 0;
    };

    Patch patches[11];
    Voice voices[MAX_TRACKS];
    Track tracks[MAX_TRACKS];

    float sampleRate = 48000.0f;
    float samplesPerTick = 240.0f;
    float tickCountdown = 1.0f;

    std::atomic<bool> paused{false};
    int   pauseFade = 0;
    int   pauseFadeFrames() const;

    std::atomic<bool>          seekPending{false};
    std::mutex                 seekMutex;
    std::unique_ptr<SeekState> pendingSeek;
    int64_t                    leadInFrames = 0;

    std::unique_ptr<Sequencer> seekScratch;
    float                      seekScratchRate = 0.0f;
    std::vector<Snapshot>      checkpoints;
    std::vector<float>         discardBuf;

    bool     tryInstallSeek();
    void     fastForward(int64_t frames);
    Snapshot capture() const;
    void     restore(const Snapshot& s);
    int      patchIndexOf(const Patch* p) const;

    std::vector<float> monoMixL;
    std::vector<float> monoMixR;

    void initPatches();
    void initPanning();
    void tick();
    void renderActive(float* output, int frameCount);
};

}