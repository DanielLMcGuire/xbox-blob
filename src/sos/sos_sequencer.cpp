#include "sos_sequencer.h"

#include <algorithm>
#include <cmath>

namespace SOS 
{

bool Sequencer::play(ProgramPtr p, std::string* error)
{
    if (!p)
        p = controlProgram;

    if (!p)
    {
        if (error)
            *error = "no program loaded";
        return false;
    }

    if (p != controlProgram && !p->validate(error))
        return false;

    std::lock_guard<std::mutex> lock(mutex);

    if (p != controlProgram)
    {
        seekScratch.reset();
        controlProgram = p;
    }

    retiredProgram.reset();
    pendingSeek.reset();
    seekPending.store(false, std::memory_order_release);

    stagedProgram = p;
    playPending.store(true, std::memory_order_release);

    return true;
}

bool Sequencer::tryInstallProgram()
{
    if (!playPending.load(std::memory_order_acquire)) return false;

    std::unique_lock<std::mutex> lock(mutex, std::try_to_lock);
    if (!lock.owns_lock()) return false;

    installProgramLocked();
    return true;
}

void Sequencer::installProgramLocked()
{
    if (!playPending.load(std::memory_order_relaxed)) return;

    retiredProgram = std::move(currentProgram);
    currentProgram = std::move(stagedProgram);
    playPending.store(false, std::memory_order_release);
    resetTracks();
}

void Sequencer::resetTracks()
{
    const Program& p = *currentProgram;

    for (int i = 0; i < MAX_TRACKS; i++)
    {
        voices[i] = Voice{};
        tracks[i] = Track{};

        if (static_cast<size_t>(i) < p.tracks.size())
        {
            const TrackDesc& d = p.tracks[i];
            voices[i].panLeft  = d.pan.left;
            voices[i].panRight = d.pan.right;

            tracks[i].bytecode = d.code.data();
            tracks[i].length   = d.code.size();
            tracks[i].active   = true;
        }
    }

    samplesPerTick = sampleRate * p.tickSeconds;
    tickCountdown = 0.0f;
    pauseFade = paused.load(std::memory_order_relaxed) ? 0 : pauseFadeFrames();
    leadInFrames = 0;
}

void Sequencer::applyPending()
{
    std::lock_guard<std::mutex> lock(mutex);
    installProgramLocked();
    installSeekLocked();
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
            if (pat >= 0 && static_cast<size_t>(pat) < currentProgram->patches.size())
            {
                t.patchIdx = pat;
                t.volume = 0;
                voices[ch].setPatch(&currentProgram->patches[pat], sampleRate);
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
                    if (pat >= 0 && static_cast<size_t>(pat) < currentProgram->patches.size())
                    {
                        t.patchIdx = pat;
                        t.volume = 0;
                        voices[ch].setPatch(&currentProgram->patches[pat], sampleRate);
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
    samplesPerTick = sampleRate * DEFAULT_TICK_SECONDS;
}

int Sequencer::pauseFadeFrames() const
{
    return std::max(1, static_cast<int>(std::lround(sampleRate * PAUSE_FADE_SECONDS)));
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
    if (!controlProgram || !std::isfinite(seconds)) return;
    seconds = std::clamp(seconds, -SEEK_MAX_SECONDS, SEEK_MAX_SECONDS);

    if (!seekScratch || seekScratchRate != sampleRate)
    {
        seekScratch = std::make_unique<Sequencer>();
        Sequencer& fresh = *seekScratch;
        fresh.setSampleRate(sampleRate);
        fresh.currentProgram = controlProgram;
        fresh.resetTracks();
        fresh.checkpoints.assign(1, fresh.capture());
        fresh.discardBuf.assign(static_cast<size_t>(SEEK_BLOCK_FRAMES) * 2, 0.0f);
        seekScratchRate = sampleRate;
    }

    Sequencer& sc = *seekScratch;
    sc.fastForward(seconds > 0.0 ? std::llround(seconds * sampleRate) : 0);

    std::lock_guard<std::mutex> lock(mutex);
    if (!pendingSeek) pendingSeek = std::make_unique<SeekState>();
    static_cast<Snapshot&>(*pendingSeek) = sc.capture();
    pendingSeek->program = controlProgram;
    pendingSeek->leadInFrames = seconds < 0.0 ? std::llround(-seconds * sampleRate) : 0;
    seekPending.store(true, std::memory_order_release);
}

void Sequencer::seekImmediate(double seconds)
{
    seek(seconds);
    applyPending();
}

bool Sequencer::tryInstallSeek()
{
    if (!seekPending.load(std::memory_order_acquire)) return false;

    std::unique_lock<std::mutex> lock(mutex, std::try_to_lock);
    if (!lock.owns_lock()) return false;

    return installSeekLocked();
}

bool Sequencer::installSeekLocked()
{
    if (!seekPending.load(std::memory_order_relaxed)) return false;

    if (playPending.load(std::memory_order_relaxed)) return false;

    seekPending.store(false, std::memory_order_release);

    if (!pendingSeek || pendingSeek->program != currentProgram) return false;

    const SeekState& s = *pendingSeek;
    for (int i = 0; i < MAX_TRACKS; i++)
    {
        voices[i] = s.voices[i];
        tracks[i] = s.tracks[i];
    }
    tickCountdown = s.tickCountdown;
    leadInFrames  = s.leadInFrames;
    pauseFade     = 0;
    return true;
}

void Sequencer::render(float* output, int frameCount)
{
    if (frameCount <= 0) return;

    if (playPending.load(std::memory_order_acquire))
        tryInstallProgram();

    if (!currentProgram)
    {
        std::fill_n(output, static_cast<size_t>(frameCount) * 2, 0.0f);
        return;
    }

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
