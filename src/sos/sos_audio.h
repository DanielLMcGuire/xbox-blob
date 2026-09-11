#pragma once

#include <cstdint>

#define NOMINMAX
#define Rectangle Win32Rectangle
#define CloseWindow Win32CloseWindow
#define ShowCursor Win32ShowCursor
#include "../../thirdparty/miniaudio.h"
#undef Rectangle
#undef CloseWindow
#undef ShowCursor

#include "sos_sequencer.h"

class SOSAudio {
public:
    SOSAudio() = default;
    ~SOSAudio();

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