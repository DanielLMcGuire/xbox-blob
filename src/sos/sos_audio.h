#pragma once

#include <cstdint>

#ifdef _WIN32
#define NOMINMAX
#define Rectangle Win32Rectangle
#define CloseWindow Win32CloseWindow
#define ShowCursor Win32ShowCursor
#endif
#include "../../thirdparty/miniaudio.h"
#ifdef _WIN32
#undef Rectangle
#undef CloseWindow
#undef ShowCursor
#endif

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
    
    bool exportWav(const char* filename, double durationSeconds, uint32_t sampleRate = 48000);

private:
    static void dataCallback(ma_device* pDevice, void* pOutput, const void*, ma_uint32 frameCount);

    Sequencer sequencer;
    ma_device device{};
    bool initialized = false;
};

}