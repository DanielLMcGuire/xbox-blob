#include "sos_sequencer.h"

#include <algorithm>
#include <cmath>

#include "tracks/tracks.inl"
#include "samples.h"

namespace SOS 
{

void Sequencer::initPatches()
{
    patches[PSIN1].sample.data.resize(128);
    patches[PSIN1].sample.loop = true;
    for (int i = 0; i < 128; i++)
        patches[PSIN1].sample.data[i] = std::sin(2.0f * PI * (float)i / 128.0f);

    patches[PSIN1].ampEnv = &Env1a; 
    patches[PSIN1].multiEnv = &Env1m;

    patches[PSAW1].sample.data.resize(128);
    patches[PSAW1].sample.loop = true;
    for (int i = 0; i < 128; i++)
        patches[PSAW1].sample.data[i] = (float)(i - 64) / 64.0f;

    patches[PSAW1].ampEnv = &SawEnv1a; 
    patches[PSAW1].multiEnv = &SawEnv1m;

    patches[PSQUARE] = patches[PSAW1]; 
    patches[PSQUARE].ampEnv = &Env3a; 
    patches[PSQUARE].multiEnv = &Env3m;
    
    patches[PSAW2] = patches[PSAW1]; 
    patches[PSAW2].ampEnv = &SawEnv2a; 
    patches[PSAW2].multiEnv = &SawEnv2m;
    
    patches[PSAW3] = patches[PSAW1]; 
    patches[PSAW3].ampEnv = &Env3a; 
    patches[PSAW3].multiEnv = &Env3m;

    patches[PNOISE1].sample.data.resize(8192);
    patches[PNOISE1].sample.loop = true;
    uint32_t holdrand = 1003;
    for (int i = 0; i < 8192; i++)
    {
        holdrand = holdrand * 214013L + 2531011L;
        int16_t r = (int16_t)((holdrand >> 16) & 0x7fff);
        patches[PNOISE1].sample.data[i] = ((float)r - 16384.0f) / 16384.0f;
    }
    patches[PNOISE1].ampEnv = &NoiseEnv1a; 
    patches[PNOISE1].multiEnv = &NoiseEnv1m;

    auto loadRaw = [](Patch& p, const unsigned char* src, size_t sz, bool loop, bool xor80)
    {
        p.sample.data.resize(sz);
        p.sample.loop = loop;
        for (size_t i = 0; i < sz; i++)
        {
            uint16_t v = xor80 ? ((uint16_t)(src[i] ^ 0x80) << 8) : ((uint16_t)src[i] << 8);
            p.sample.data[i] = (float)(int16_t)v / 32768.0f;
        }
        p.ampEnv = &OpenEnva; 
        p.multiEnv = &OpenEnvm;
    };

    loadRaw(patches[PGLOCK], GlockData, sizeof(GlockData), false, true);
    loadRaw(patches[PBUBBLE], BubbleData, sizeof(BubbleData), true, true);

    patches[PFM].sample.data.resize(32768);
    patches[PFM].sample.loop = false;
    double FMc = 4.0, FMm = 2.0;
    int j = 0;
    for (int i = 0; i < 32768; i++)
    {
        if (i < 16384) j++;
        else j--;
        double dtmp = ((double)j / 16384.0) * std::sin(FMm * 2.0 * PI * (double)i / 128.0);
        patches[PFM].sample.data[i] = (float)std::sin(dtmp + FMc * 2.0 * PI * (double)i / 128.0);
    }
    patches[PFM].ampEnv = &OpenEnva;
    patches[PFM].multiEnv = &OpenEnvm;

    loadRaw(patches[PTHUNEL16], ThunEl16Data, sizeof(ThunEl16Data), false, false);

    patches[PREVTHUN].sample.data.resize(sizeof(ThunEl16Data));
    patches[PREVTHUN].sample.loop = false;
    for (size_t i = 0; i < sizeof(ThunEl16Data); i++)
        patches[PREVTHUN].sample.data[i] = patches[PTHUNEL16].sample.data[sizeof(ThunEl16Data) - 1 - i];

    patches[PREVTHUN].ampEnv = &OpenEnva; 
    patches[PREVTHUN].multiEnv = &OpenEnvm;
}

void Sequencer::initPanning()
{
    for (int i = 0; i < MAX_TRACKS; i++)
    {
        if (i == 3 || i == 5)
        {
            voices[i].panLeft = 1.0f;
            voices[i].panRight = std::pow(10.0f, -100.0f / 2000.0f);
        } 
        else if (i % 2 != 0) 
        {
            voices[i].panLeft = std::pow(10.0f, -600.0f / 2000.0f);
            voices[i].panRight = 1.0f;
        } 
        else 
        {
            voices[i].panLeft = 1.0f;
            voices[i].panRight = std::pow(10.0f, -600.0f / 2000.0f);
        }
    }
}

void Sequencer::tick()
{
#if defined(__GNUC__) || defined(__clang__)
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc99-designator"
#pragma clang diagnostic ignored "-Wc99-extensions"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif

    static const void* const dispatchTable[] = {
        &&lbl_REST,
        &&lbl_NOTE,
        &&lbl_DEFAULT,
        &&lbl_LOOP,
        &&lbl_ENDLOOP,
        &&lbl_PATCH,
        &&lbl_DEFAULT,
        &&lbl_DEFAULT,
        &&lbl_DEFAULT,
        &&lbl_VOLUME,
        &&lbl_XPOSE,
        &&lbl_DEFAULT,
        &&lbl_SLUR,
        &&lbl_RING,
        &&lbl_DEFAULT,
        &&lbl_END,
        &&lbl_FILTERINC,
        &&lbl_FILTERSET,
    };

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

    constexpr size_t tableSize = sizeof(dispatchTable) / sizeof(dispatchTable[0]);
#endif

    for (int ch = 0; ch < MAX_TRACKS; ch++)
    {
        auto& t = tracks[ch];
        if (!t.active) continue;

        t.timer--;
        if (t.timer > 0) continue;

#if defined(__GNUC__) || defined(__clang__)
        #define NEXT() \
            do { \
                if (!t.canRead(1)) { t.active = false; goto track_done; } \
                int16_t op = t.read(); \
                if (op >= tableSize || !dispatchTable[op]) goto lbl_DEFAULT; \
                goto *dispatchTable[op]; \
            } while (0)

        NEXT();

    lbl_REST:
        if (t.canRead(1))
        {
            voices[ch].noteOff();
            t.timer += t.read();
        }
        else
        {
            t.active = false;
            goto track_done;
        }
        if (t.timer > 0) goto track_done;
        NEXT();

    lbl_NOTE:
        if (t.canRead(2))
        {
            uint8_t p = (uint8_t)t.read();
            int16_t dur = t.read();
            t.pitch = (p << 8) + t.transpose;
            voices[ch].noteOn(t.pitch);
            t.timer += dur;
        }
        else
        {
            t.active = false;
            goto track_done;
        }
        if (t.timer > 0) goto track_done;
        NEXT();

    lbl_SLUR:
        if (t.canRead(2))
        {
            uint8_t p = (uint8_t)t.read();
            int16_t dur = t.read();
            t.pitch = (p << 8) + t.transpose;
            voices[ch].setPitch(t.pitch);
            t.timer += dur;
        }
        else
        {
            t.active = false;
            goto track_done;
        }
        if (t.timer > 0) goto track_done;
        NEXT();

    lbl_RING:
        if (t.canRead(1))
        {
            t.timer += t.read();
        }
        else
        {
            t.active = false;
            goto track_done;
        }
        if (t.timer > 0) goto track_done;
        NEXT();

    lbl_FILTERSET:
        if (t.canRead(2))
        {
            t.filterCutoff = (int16_t)t.read();
            t.filterRes = t.read();
            voices[ch].setFilter(t.filterCutoff, t.filterRes);
        }
        else
        {
            t.active = false;
            goto track_done;
        }
        NEXT();

    lbl_FILTERINC:
        if (t.canRead(2))
        {
            t.filterCutoff += (int16_t)t.read();
            t.filterRes = t.read();
            voices[ch].setFilter(t.filterCutoff, t.filterRes);
        }
        else
        {
            t.active = false;
            goto track_done;
        }
        NEXT();

    lbl_PATCH:
        if (t.canRead(1))
        {
            int16_t pat = t.read();
            if (pat < 11)
            {
                t.patchIdx = pat;
                t.volume = 0;
                voices[ch].setPatch(&patches[pat], sampleRate);
                voices[ch].setVolume(t.volume);
            }
        }
        else
        {
            t.active = false;
            goto track_done;
        }
        NEXT();

    lbl_VOLUME:
        if (t.canRead(1))
        {
            t.volume += (int16_t)t.read();
            voices[ch].setVolume(t.volume);
        }
        else
        {
            t.active = false;
            goto track_done;
        }
        NEXT();

    lbl_XPOSE:
        if (t.canRead(1))
        {
            t.transpose += (int16_t)t.read();
        }
        else
        {
            t.active = false;
            goto track_done;
        }
        NEXT();

    lbl_LOOP:
        if (t.canRead(1))
        {
            int16_t count = t.read();
            if (t.loopDepth < MAX_LOOP_DEPTH)
                t.loopStack[t.loopDepth++] = { count, t.pc };
        }
        else
        {
            t.active = false;
            goto track_done;
        }
        NEXT();

    lbl_ENDLOOP:
        if (t.loopDepth > 0)
        {
            if (--t.loopStack[t.loopDepth - 1].count > 0)
                t.pc = t.loopStack[t.loopDepth - 1].returnIndex;
            else
                t.loopDepth--;
        }
        NEXT();

    lbl_END:
        t.active = false;
        voices[ch].noteOff();
        goto track_done;

    lbl_DEFAULT:
        t.active = false;
        goto track_done;

        #undef NEXT
    track_done:;
#else
        while (t.active && t.timer <= 0)
        {
            if (!t.canRead(1))
            {
                t.active = false;
                break;
            }
            int16_t op = t.read();
            switch (op)
            {
            case F_REST:
                if (t.canRead(1))
                {
                    voices[ch].noteOff();
                    t.timer += t.read();
                }
                break;
            case F_NOTE:
                if (t.canRead(2))
                {
                    uint8_t p = (uint8_t)t.read();
                    int16_t dur = t.read();
                    t.pitch = (p << 8) + t.transpose;
                    voices[ch].noteOn(t.pitch);
                    t.timer += dur;
                }
                break;
            case F_SLUR:
                if (t.canRead(2))
                {
                    uint8_t p = (uint8_t)t.read();
                    int16_t dur = t.read();
                    t.pitch = (p << 8) + t.transpose;
                    voices[ch].setPitch(t.pitch);
                    t.timer += dur;
                }
                break;
            case F_RING:
                if (t.canRead(1)) t.timer += t.read();
                break;
            case F_FILTERSET:
                if (t.canRead(2))
                {
                    t.filterCutoff = (int16_t)t.read();
                    t.filterRes = t.read();
                    voices[ch].setFilter(t.filterCutoff, t.filterRes);
                }
                break;
            case F_FILTERINC:
                if (t.canRead(2))
                {
                    t.filterCutoff += (int16_t)t.read();
                    t.filterRes = t.read();
                    voices[ch].setFilter(t.filterCutoff, t.filterRes);
                }
                break;
            case F_PATCH:
                if (t.canRead(1))
                {
                    int16_t pat = t.read();
                    if (pat < 11)
                    {
                        t.patchIdx = pat;
                        t.volume = 0;
                        voices[ch].setPatch(&patches[pat], sampleRate);
                        voices[ch].setVolume(t.volume);
                    }
                }
                break;
            case F_VOLUME:
                if (t.canRead(1))
                {
                    t.volume += (int16_t)t.read();
                    voices[ch].setVolume(t.volume);
                }
                break;
            case F_XPOSE:
                if (t.canRead(1)) t.transpose += (int16_t)t.read();
                break;
            case F_LOOP:
                if (t.canRead(1))
                {
                    int16_t count = t.read();
                    if (t.loopDepth < MAX_LOOP_DEPTH)
                        t.loopStack[t.loopDepth++] = { count, t.pc };
                }
                break;
            case F_ENDLOOP:
                if (t.loopDepth > 0)
                {
                    if (--t.loopStack[t.loopDepth - 1].count > 0)
                        t.pc = t.loopStack[t.loopDepth - 1].returnIndex;
                    else
                        t.loopDepth--;
                }
                break;
            case F_END:
                t.active = false;
                voices[ch].noteOff();
                break;
            default:
                t.active = false;
                break;
            }
        }
#endif
    }
}

void Sequencer::setSampleRate(float sr)
{
    sampleRate = sr;
    samplesPerTick = sampleRate * 0.005f;
}

void Sequencer::startBootSound()
{
    constexpr int BOOT_TRACK_COUNT = 12;
    for (int i = 0; i < MAX_TRACKS; i++)
    {
        if (i < BOOT_TRACK_COUNT && BootSequence[i].data != nullptr)
        {
            tracks[i].bytecode = BootSequence[i].data;
            tracks[i].length = BootSequence[i].count;
            tracks[i].pc = 0;
            tracks[i].timer = 0;
            tracks[i].pitch = 0;
            tracks[i].transpose = 0;
            tracks[i].volume = 0;
            tracks[i].filterCutoff = 0;
            tracks[i].filterRes = 0;
            tracks[i].active = true;
            tracks[i].loopDepth = 0;
        }
        else
        {
            tracks[i].active = false;
        }
        voices[i].active = false;
    }
    tickCountdown = 0.0f;
    pauseFade = paused.load(std::memory_order_relaxed) ? 0 : pauseFadeFrames();
    leadInFrames = 0;
    seekPending.store(false, std::memory_order_relaxed);
}

int Sequencer::pauseFadeFrames() const
{
    return std::max(1, static_cast<int>(std::lround(sampleRate * PAUSE_FADE_SECONDS)));
}

int Sequencer::patchIndexOf(const Patch* p) const
{
    if (!p) return -1;
    const std::ptrdiff_t i = p - patches;
    return (i >= 0 && i < static_cast<std::ptrdiff_t>(sizeof(patches) / sizeof(patches[0]))) ? static_cast<int>(i) : -1;
}

Sequencer::Snapshot Sequencer::capture() const
{
    Snapshot s;
    for (int i = 0; i < MAX_TRACKS; i++) { s.voices[i] = voices[i]; s.tracks[i] = tracks[i]; }
    s.tickCountdown = tickCountdown;
    return s;
}

void Sequencer::restore(const Snapshot& s)
{
    for (int i = 0; i < MAX_TRACKS; i++) { voices[i] = s.voices[i]; tracks[i] = s.tracks[i]; }
    tickCountdown = s.tickCountdown;
}

void Sequencer::fastForward(int64_t target)
{
    const int64_t interval = std::max<int64_t>(1, std::llround(sampleRate * SEEK_CHECKPOINT_SECONDS));

    const size_t idx = static_cast<size_t>(std::min<int64_t>(target / interval, static_cast<int64_t>(checkpoints.size()) - 1));
    restore(checkpoints[idx]);
    int64_t pos = static_cast<int64_t>(idx) * interval;

    while (pos < target)
    {
        const int64_t boundary = (pos / interval + 1) * interval;
        const int64_t next     = std::min(target, boundary);

        for (int64_t left = next - pos; left > 0; )
        {
            const int n = static_cast<int>(std::min<int64_t>(left, SEEK_BLOCK_FRAMES));
            renderActive(discardBuf.data(), n);
            left -= n;
        }
        pos = next;

        if (pos == boundary && static_cast<size_t>(pos / interval) == checkpoints.size())
            checkpoints.push_back(capture());
    }
}

void Sequencer::seek(double seconds)
{
    if (!std::isfinite(seconds)) return;
    seconds = std::clamp(seconds, -SEEK_MAX_SECONDS, SEEK_MAX_SECONDS);

    if (!seekScratch || seekScratchRate != sampleRate)
    {
        seekScratch = std::make_unique<Sequencer>();
        Sequencer& fresh = *seekScratch;
        fresh.setSampleRate(sampleRate);
        fresh.startBootSound();
        fresh.checkpoints.assign(1, fresh.capture());
        fresh.discardBuf.assign(static_cast<size_t>(SEEK_BLOCK_FRAMES) * 2, 0.0f);
        seekScratchRate = sampleRate;
    }

    Sequencer& sc = *seekScratch;
    sc.fastForward(seconds > 0.0 ? std::llround(seconds * sampleRate) : 0);

    std::lock_guard<std::mutex> lock(seekMutex);
    if (!pendingSeek) pendingSeek = std::make_unique<SeekState>();
    static_cast<Snapshot&>(*pendingSeek) = sc.capture();
    for (int i = 0; i < MAX_TRACKS; i++)
        pendingSeek->patchIdx[i] = sc.patchIndexOf(sc.voices[i].patch);
    pendingSeek->leadInFrames = seconds < 0.0 ? std::llround(-seconds * sampleRate) : 0;
    seekPending.store(true, std::memory_order_release);
}

void Sequencer::seekImmediate(double seconds)
{
    seek(seconds);
    tryInstallSeek();
}

bool Sequencer::tryInstallSeek()
{
    if (!seekPending.load(std::memory_order_acquire)) return false;

    std::unique_lock<std::mutex> lock(seekMutex, std::try_to_lock);
    if (!lock.owns_lock() || !pendingSeek) return false;

    const SeekState& s = *pendingSeek;
    for (int i = 0; i < MAX_TRACKS; i++)
    {
        voices[i] = s.voices[i];
        voices[i].patch = s.patchIdx[i] >= 0 ? &patches[s.patchIdx[i]] : nullptr;
        tracks[i] = s.tracks[i];
    }
    tickCountdown = s.tickCountdown;
    leadInFrames  = s.leadInFrames;
    pauseFade     = 0;
    seekPending.store(false, std::memory_order_release);
    return true;
}

void Sequencer::render(float* output, int frameCount)
{
    if (frameCount <= 0) return;

    if (seekPending.load(std::memory_order_acquire) && pauseFade <= 0)
        tryInstallSeek();

    const bool run        = !paused.load(std::memory_order_relaxed) &&
                            !seekPending.load(std::memory_order_acquire);
    const int  fadeFrames = pauseFadeFrames();

    if (!run && pauseFade <= 0)
    {
        std::fill_n(output, static_cast<size_t>(frameCount) * 2, 0.0f);
        return;
    }

    const int activeFrames = run ? frameCount : std::min(frameCount, pauseFade);

    renderActive(output, activeFrames);

    if (!(run && pauseFade >= fadeFrames))
    {
        for (int f = 0; f < activeFrames; f++)
        {
            if (run) pauseFade = std::min(pauseFade + 1, fadeFrames);
            else     pauseFade = std::max(pauseFade - 1, 0);

            const float gain = static_cast<float>(pauseFade) / static_cast<float>(fadeFrames);
            output[f * 2]     *= gain;
            output[f * 2 + 1] *= gain;
        }
    }

    if (activeFrames < frameCount)
        std::fill(output + static_cast<size_t>(activeFrames) * 2,
                  output + static_cast<size_t>(frameCount) * 2, 0.0f);
}

void Sequencer::renderActive(float* output, int frameCount)
{
    constexpr int SOLO_TRACK = -1;

    if (leadInFrames > 0)
    {
        const int silent = static_cast<int>(std::min<int64_t>(leadInFrames, frameCount));
        std::fill_n(output, static_cast<size_t>(silent) * 2, 0.0f);
        leadInFrames -= silent;
        output += static_cast<size_t>(silent) * 2;
        frameCount -= silent;
        if (frameCount <= 0) return;
    }

    monoMixL.assign(frameCount, 0.0f);
    monoMixR.assign(frameCount, 0.0f);

    int framesProcessed = 0;
    while (framesProcessed < frameCount) 
    {
        if (tickCountdown <= 0.0f) { tick(); tickCountdown += samplesPerTick; }

        int framesToTick = static_cast<int>(std::ceil(tickCountdown));
        int sliceFrames = std::min(frameCount - framesProcessed, framesToTick);
        if (sliceFrames <= 0) break;

        for (int i = 0; i < MAX_TRACKS; i++)
        {
            if (SOLO_TRACK >= 0 && i != SOLO_TRACK) continue;
            voices[i].renderBlock(monoMixL.data() + framesProcessed,
                                  monoMixR.data() + framesProcessed,
                                  sliceFrames, sampleRate);
        }

        tickCountdown -= (float)sliceFrames;
        framesProcessed += sliceFrames;
    }

    for (int f = 0; f < frameCount; f++)
    {
        output[f * 2]     = std::tanh(monoMixL[f] * 0.45f);
        output[f * 2 + 1] = std::tanh(monoMixR[f] * 0.45f);
    }
}

}