#define MINIAUDIO_IMPLEMENTATION
#include "sos_audio.h"

#include <algorithm>
#include <cmath>
#include <vector>

bool SOSAudio::init()
{
    if (initialized) return true;

    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format   = ma_format_f32;
    config.playback.channels = 2;
    config.sampleRate        = 48000;
    config.dataCallback      = dataCallback;
    config.pUserData         = this;

    if (ma_device_init(nullptr, &config, &device) != MA_SUCCESS) {
        return false;
    }

    sequencer.setSampleRate(static_cast<float>(device.sampleRate));
    sequencer.startBootSound();

    initialized = true;
    if (ma_device_start(&device) != MA_SUCCESS) {
        ma_device_uninit(&device);
        initialized = false;
        return false;
    }
    return true;
}

void SOSAudio::restart()
{
    if (!initialized) return;

    ma_device_stop(&device);
    sequencer.startBootSound();
    ma_device_start(&device);
}

void SOSAudio::deinit()
{
    if (initialized) {
        ma_device_uninit(&device);
        initialized = false;
    }
}

bool SOSAudio::exportWav(const char* filename, double durationSeconds, uint32_t sampleRate)
{
    if (!filename || durationSeconds <= 0.0 || sampleRate == 0)
        return false;

    const ma_uint64 totalFrames = static_cast<ma_uint64>(durationSeconds * sampleRate);
    SOSSequencer exportSeq;
    exportSeq.setSampleRate(static_cast<float>(sampleRate));
    exportSeq.startBootSound();

    ma_encoder_config encoderConfig = ma_encoder_config_init(
        ma_encoding_format_wav, ma_format_s16, 2, sampleRate
    );

    ma_encoder encoder{};
    if (ma_encoder_init_file(filename, &encoderConfig, &encoder) != MA_SUCCESS)
        return false;

    constexpr ma_uint64 BLOCK = 4096;
    std::vector<float> fBuf(BLOCK * 2);
    std::vector<int16_t> pcmBuf(BLOCK * 2);
    ma_uint64 remaining = totalFrames;

    while (remaining > 0) {
        ma_uint64 toRender = std::min<ma_uint64>(remaining, BLOCK);
        exportSeq.render(fBuf.data(), static_cast<int>(toRender));

        for (ma_uint64 i = 0; i < toRender * 2; ++i)
            pcmBuf[i] = static_cast<int16_t>(std::lrintf(std::clamp(fBuf[i], -1.0f, 1.0f) * 32767.0f));

        ma_uint64 written = 0;
        if (ma_encoder_write_pcm_frames(&encoder, pcmBuf.data(), toRender, 
            &written) != MA_SUCCESS || written == 0)
        {
            ma_encoder_uninit(&encoder);
            return false;
        }
        remaining -= written;
    }

    ma_encoder_uninit(&encoder);
    return true;
}

void SOSAudio::dataCallback(ma_device* pDevice, void* pOutput, const void*, ma_uint32 frameCount)
{
    auto* audio = static_cast<SOSAudio*>(pDevice->pUserData);
    if (audio && pOutput && frameCount > 0)
        audio->sequencer.render(static_cast<float*>(pOutput), static_cast<int>(frameCount));
}