#include "sos_audio.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace SOS 
{

Audio* Audio::s_activeInstance = nullptr;

bool Audio::init()
{
    if (initialized) return true;
    if (!IsAudioDeviceReady()) return false;

    stream = LoadAudioStream(48000, 32, 2);
    if (!IsAudioStreamValid(stream)) return false;

    sequencer.setSampleRate(static_cast<float>(stream.sampleRate));
    sequencer.startBootSound();

    s_activeInstance = this;
    SetAudioStreamCallback(stream, dataCallback);

    initialized = true;
    PlayAudioStream(stream);
    return true;
}

void Audio::restart()
{
    if (!initialized) return;

    StopAudioStream(stream);
    sequencer.startBootSound();
    PlayAudioStream(stream);
}

void Audio::deinit()
{
    if (initialized) {
        StopAudioStream(stream);
        UnloadAudioStream(stream);
        if (s_activeInstance == this) s_activeInstance = nullptr;
        initialized = false;
    }
}

bool Audio::exportWav(const char* filename, double durationSeconds, uint32_t sampleRate)
{
    if (!filename || durationSeconds <= 0.0 || sampleRate == 0)
        return false;

    const uint32_t totalFrames = static_cast<uint32_t>(durationSeconds * sampleRate);
    Sequencer exportSeq;
    exportSeq.setSampleRate(static_cast<float>(sampleRate));
    exportSeq.startBootSound();

    constexpr uint32_t BLOCK = 4096;
    std::vector<int16_t> pcmBuf(static_cast<size_t>(totalFrames) * 2);
    std::vector<float> fBuf(static_cast<size_t>(BLOCK) * 2);

    uint32_t framesWritten = 0;
    while (framesWritten < totalFrames)
    {
        uint32_t toRender = std::min(totalFrames - framesWritten, BLOCK);
        exportSeq.render(fBuf.data(), static_cast<int>(toRender));

        for (uint32_t i = 0; i < toRender * 2; ++i)
            pcmBuf[static_cast<size_t>(framesWritten) * 2 + i] =
                static_cast<int16_t>(std::lrintf(std::clamp(fBuf[i], -1.0f, 1.0f) * 32767.0f));

        framesWritten += toRender;
    }

    Wave wave{};
    wave.frameCount = totalFrames;
    wave.sampleRate = sampleRate;
    wave.sampleSize = 16;
    wave.channels   = 2;
    wave.data       = pcmBuf.data();

    return ExportWave(wave, filename);
}

void Audio::dataCallback(void *bufferData, unsigned int frames)
{
    if (s_activeInstance && bufferData && frames > 0)
        s_activeInstance->sequencer.render(static_cast<float*>(bufferData), static_cast<int>(frames));
}

}
