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

class SOSAudio
{
public:
    SOSAudio() { init(); }
    ~SOSAudio() { deinit(); }

    bool init();
    void restart();
    void deinit();
    
    bool exportWav(const char* filename, double durationSeconds, uint32_t sampleRate = 48000);

private:
    static void dataCallback(ma_device* pDevice, void* pOutput, const void*, ma_uint32 frameCount);

    SOSSequencer sequencer;
    ma_device device{};
    bool initialized = false;
};