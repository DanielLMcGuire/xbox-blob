#pragma once

#include "raylib.h"

#include "sos_sequencer.h"

#include <cstdint>
#include <string>
#include <utility>

namespace SOS 
{

class Audio
{
public:
    Audio() = default;
    explicit Audio(ProgramPtr program, bool doInit = true)
    {
        setProgram(std::move(program));
        if (doInit) init();
    }
    ~Audio() { deinit(); }

    bool setProgram(ProgramPtr program);
    ProgramPtr program() const { return programPtr; }
    const std::string& lastError() const { return error; }

    bool init();
    void restart();
    void deinit();

    void setPaused(bool paused) { sequencer.setPaused(paused); }
    bool isPaused() const { return sequencer.isPaused(); }
    void seek(double seconds);

    bool exportWav(const char* filename, double durationSeconds, uint32_t sampleRate = 48000);

private:
    static void dataCallback(void *bufferData, unsigned int frames);

    ProgramPtr programPtr;
    std::string error;
    Sequencer sequencer;
    AudioStream stream{};
    bool initialized = false;
    bool hasDeferredSeek = false;
    double deferredSeek = 0.0;

    static Audio* s_activeInstance;
};

}
