#include "sos_envelope.h"
#include <algorithm>

void EnvelopeGenerator::setDesc(const DSENVELOPEDESC* desc, float sampleRate)
{
    if (!desc)
    {
        delaySamples   = 0;
        attackSamples  = 0;
        holdSamples    = 0;
        decaySamples   = 0;
        releaseSamples = 0;
        sustainLevel   = 1.0f;
        pitchScale     = 0;
        filterCutoff   = 0;
        state          = SUSTAIN;
        level          = 1.0f;
        count          = 0;
        return;
    }

    const float timeScale = (512.0f / 48000.0f) * sampleRate;

    auto toSamples = [timeScale](uint32_t units) -> uint32_t
    {
        return (units > 0) ? std::max<uint32_t>(1, static_cast<uint32_t>(units * timeScale)) : 0;
    };

    delaySamples   = toSamples(desc->dwDelay);
    attackSamples  = toSamples(desc->dwAttack);
    holdSamples    = toSamples(desc->dwHold);
    decaySamples   = toSamples(desc->dwDecay);
    releaseSamples = toSamples(desc->dwRelease);

    sustainLevel   = std::clamp((float)desc->dwSustain / 255.0f, 0.0f, 1.0f);
    pitchScale     = desc->lPitchScale;
    filterCutoff   = desc->lFilterCutOff;

    state = OFF;
    level = 0.0f;
    count = 0;
}

void EnvelopeGenerator::trigger()
{
    count = 0;
    if (delaySamples > 0)       { state = DELAY;   level = 0.0f; }
    else if (attackSamples > 0) { state = ATTACK;  level = 0.0f; }
    else if (holdSamples > 0)   { state = HOLD;    level = 1.0f; }
    else if (decaySamples > 0)  { state = DECAY;   level = 1.0f; }
    else                        { state = SUSTAIN; level = sustainLevel; }
}

void EnvelopeGenerator::release()
{
    if (state != OFF && state != RELEASE)
    {
        state = RELEASE;
        releaseStart = level;
        count = 0;
        if (releaseSamples == 0)
        {
            level = 0.0f;
            state = OFF;
        }
    }
}

float EnvelopeGenerator::processBlock(int stepSamples) 
{
    if (state == OFF)
    {
        level = 0.0f;
        return 0.0f;
    }
    if (state == SUSTAIN) 
    {
        level = sustainLevel;
        return level;
    }

    for (int i = 0; i < stepSamples; ++i) 
    {
        switch (state) 
        {
        case DELAY:
            if (++count >= delaySamples) 
            {
                count = 0;
                if (attackSamples > 0)      { state = ATTACK;  level = 0.0f; }
                else if (holdSamples > 0)   { state = HOLD;    level = 1.0f; }
                else if (decaySamples > 0)  { state = DECAY;   level = 1.0f; }
                else                        { state = SUSTAIN; level = sustainLevel; }
            }
            break;

        case ATTACK:
            count++;
            level = (float)count / (float)attackSamples;
            if (count >= attackSamples)
            {
                level = 1.0f;
                count = 0;
                if (holdSamples > 0)       { state = HOLD; }
                else if (decaySamples > 0) { state = DECAY; }
                else                       { state = SUSTAIN; level = sustainLevel; }
            }
            break;

        case HOLD:
            level = 1.0f;
            if (++count >= holdSamples)
            {
                count = 0;
                if (decaySamples > 0) { state = DECAY; }
                else                  { state = SUSTAIN; level = sustainLevel; }
            }
            break;

        case DECAY:
            count++;
            level = 1.0f - (1.0f - sustainLevel) * ((float)count / (float)decaySamples);
            if (count >= decaySamples)
            {
                level = sustainLevel;
                state = SUSTAIN;
            }
            break;

        case SUSTAIN:
            level = sustainLevel;
            break;

        case RELEASE:
            count++;
            level = releaseStart * (1.0f - ((float)count / (float)releaseSamples));
            if (count >= releaseSamples)
            {
                level = 0.0f;
                state = OFF;
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