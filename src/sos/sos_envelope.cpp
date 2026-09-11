#include "sos_envelope.h"

void EnvelopeGenerator::setDesc(const DSENVELOPEDESC* desc, float sampleRate) {
    if (!desc) {
        state = SUSTAIN;
        level = 1.0f;
        return;
    }
    const float timeScale = (512.0f / 48000.0f) * sampleRate;
    delaySamples   = static_cast<uint32_t>(desc->dwDelay * timeScale);
    attackSamples  = static_cast<uint32_t>(desc->dwAttack * timeScale);
    holdSamples    = static_cast<uint32_t>(desc->dwHold * timeScale);
    decaySamples   = static_cast<uint32_t>(desc->dwDecay * timeScale);
    releaseSamples = static_cast<uint32_t>(desc->dwRelease * timeScale);
    sustainLevel   = desc->dwSustain / 255.0f;
    pitchScale     = desc->lPitchScale;
    filterCutoff   = desc->lFilterCutOff;
    state = OFF;
    level = 0.0f;
    count = 0;
}

void EnvelopeGenerator::trigger() {
    count = 0;
    if (delaySamples > 0)       { state = DELAY; level = 0.0f; }
    else if (attackSamples > 0) { state = ATTACK; level = 0.0f; }
    else if (holdSamples > 0)   { state = HOLD; level = 1.0f; }
    else if (decaySamples > 0)  { state = DECAY; level = 1.0f; }
    else                        { state = SUSTAIN; level = sustainLevel; }
}

void EnvelopeGenerator::release() {
    if (state != OFF && state != RELEASE) {
        state = RELEASE;
        releaseStart = level;
        count = 0;
        if (releaseSamples == 0) {
            level = 0.0f;
            state = OFF;
        }
    }
}

float EnvelopeGenerator::processBlock(int stepSamples) {
    for (int i = 0; i < stepSamples; ++i) {
        switch (state) {
        case DELAY:
            if (++count >= delaySamples) {
                count = 0;
                state = (attackSamples > 0) ? ATTACK : (holdSamples > 0 ? HOLD : (decaySamples > 0 ? DECAY : SUSTAIN));
                if (state == HOLD || state == DECAY) level = 1.0f;
                if (state == SUSTAIN) level = sustainLevel;
            }
            break;
        case ATTACK:
            if (attackSamples > 0) {
                count++;
                level = (float)count / (float)attackSamples;
                if (count >= attackSamples) {
                    level = 1.0f;
                    count = 0;
                    state = (holdSamples > 0) ? HOLD : (decaySamples > 0 ? DECAY : SUSTAIN);
                    if (state == SUSTAIN) level = sustainLevel;
                }
            }
            break;
        case HOLD:
            level = 1.0f;
            if (++count >= holdSamples) {
                count = 0;
                state = (decaySamples > 0) ? DECAY : SUSTAIN;
                if (state == SUSTAIN) level = sustainLevel;
            }
            break;
        case DECAY:
            if (decaySamples > 0) {
                count++;
                level = 1.0f - (1.0f - sustainLevel) * ((float)count / (float)decaySamples);
                if (count >= decaySamples) {
                    level = sustainLevel;
                    state = SUSTAIN;
                }
            }
            break;
        case SUSTAIN:
            level = sustainLevel;
            break;
        case RELEASE:
            if (releaseSamples > 0) {
                count++;
                level = releaseStart * (1.0f - ((float)count / (float)releaseSamples));
                if (count >= releaseSamples) {
                    level = 0.0f;
                    state = OFF;
                }
            }
            break;
        case OFF:
        default:
            level = 0.0f;
            return 0.0f;
        }
    }
    return level;
}