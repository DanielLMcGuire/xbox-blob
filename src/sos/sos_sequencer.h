#pragma once

#include "sos_common.h"
#include "sos_envelope.h"
#include "sos_program.h"
#include "sos_sample.h"
#include "sos_track.h"
#include "sos_voice.h"

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace SOS 
{

class Sequencer
{
public:
    Sequencer() = default;
    Sequencer(const Sequencer&) = delete;
    Sequencer& operator=(const Sequencer&) = delete;

    void setSampleRate(float sr);

    bool play(ProgramPtr program, std::string* error = nullptr);

    ProgramPtr program() const { return controlProgram; }

    void render(float* output, int frameCount);

    inline void setPaused(bool p) { paused.store(p, std::memory_order_relaxed); }
    inline bool isPaused() const  { return paused.load(std::memory_order_relaxed); }

    void seek(double seconds);

    void applyPending();
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
        ProgramPtr program;
        int64_t    leadInFrames = 0;
    };

    ProgramPtr currentProgram;
    Voice voices[MAX_TRACKS];
    Track tracks[MAX_TRACKS];

    float sampleRate = 48000.0f;
    float samplesPerTick = 240.0f;
    float tickCountdown = 1.0f;

    std::atomic<bool> paused{false};
    int   pauseFade = 0;
    int   pauseFadeFrames() const;

    int64_t leadInFrames = 0;

    std::mutex                 mutex;
    std::atomic<bool>          playPending{false};
    std::atomic<bool>          seekPending{false};
    ProgramPtr                 stagedProgram;
    std::unique_ptr<SeekState> pendingSeek;

    ProgramPtr                 retiredProgram;
    ProgramPtr                 controlProgram;
    std::unique_ptr<Sequencer> seekScratch;
    float                      seekScratchRate = 0.0f;
    std::vector<Snapshot>      checkpoints;
    std::vector<float>         discardBuf;

    bool tryInstallProgram();
    bool tryInstallSeek();
    void installProgramLocked();
    bool installSeekLocked();
    void resetTracks();

    void     fastForward(int64_t frames);
    Snapshot capture() const;
    void     restore(const Snapshot& s);

    std::vector<float> monoMixL;
    std::vector<float> monoMixR;

    void tick();
    void renderActive(float* output, int frameCount);
};

}
