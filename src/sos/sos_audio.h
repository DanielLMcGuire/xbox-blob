#pragma once

#include <cstdint>

#include "raylib.h"

#include "sos_sequencer.h"

namespace SOS 
{

class Audio
{
public:
    Audio(bool doInit) { if (doInit) init(); }
    Audio() { init(); }
    ~Audio() { deinit(); }

    bool init();
    void restart();
    void deinit();

    void setPaused(bool paused) { sequencer.setPaused(paused); }
    bool isPaused() const { return sequencer.isPaused(); }
    void seek(double seconds);

    bool exportWav(const char* filename, double durationSeconds, uint32_t sampleRate = 48000);

private:
    static void dataCallback(void *bufferData, unsigned int frames);

    Sequencer sequencer;
    AudioStream stream{};
    bool initialized = false;
    bool hasDeferredSeek = false;
    double deferredSeek = 0.0;

    static Audio* s_activeInstance;
};

}
