#include "sos_boot.h"

#include <cmath>
#include <vector>

#include "raymath.h"

#include "../sos_asm.h"
#include "tracks/tracks.inl"
#include "../sos_asm_end.h"
#include "samples.h"

namespace SOS
{
namespace Boot
{

namespace
{

std::vector<Patch> buildPatches()
{
    std::vector<Patch> patches(11);

    patches[PSIN1].sample.data.resize(128);
    patches[PSIN1].sample.loop = true;
    for (int i = 0; i < 128; i++)
        patches[PSIN1].sample.data[i] = std::sin(2.0f * PI * (float)i / 128.0f);

    patches[PSIN1].ampEnv = Env1a; 
    patches[PSIN1].multiEnv = Env1m;

    patches[PSAW1].sample.data.resize(128);
    patches[PSAW1].sample.loop = true;
    for (int i = 0; i < 128; i++)
        patches[PSAW1].sample.data[i] = (float)(i - 64) / 64.0f;

    patches[PSAW1].ampEnv = SawEnv1a; 
    patches[PSAW1].multiEnv = SawEnv1m;

    patches[PSQUARE] = patches[PSAW1]; 
    patches[PSQUARE].ampEnv = Env3a; 
    patches[PSQUARE].multiEnv = Env3m;
    
    patches[PSAW2] = patches[PSAW1]; 
    patches[PSAW2].ampEnv = SawEnv2a; 
    patches[PSAW2].multiEnv = SawEnv2m;
    
    patches[PSAW3] = patches[PSAW1]; 
    patches[PSAW3].ampEnv = Env3a; 
    patches[PSAW3].multiEnv = Env3m;

    patches[PNOISE1].sample.data.resize(8192);
    patches[PNOISE1].sample.loop = true;
    uint32_t holdrand = 1003;
    for (int i = 0; i < 8192; i++)
    {
        holdrand = holdrand * 214013L + 2531011L;
        int16_t r = (int16_t)((holdrand >> 16) & 0x7fff);
        patches[PNOISE1].sample.data[i] = ((float)r - 16384.0f) / 16384.0f;
    }
    patches[PNOISE1].ampEnv = NoiseEnv1a; 
    patches[PNOISE1].multiEnv = NoiseEnv1m;

    auto loadRaw = [](Patch& p, const unsigned char* src, size_t sz, bool loop, bool xor80)
    {
        p.sample.data.resize(sz);
        p.sample.loop = loop;
        for (size_t i = 0; i < sz; i++)
        {
            uint16_t v = xor80 ? ((uint16_t)(src[i] ^ 0x80) << 8) : ((uint16_t)src[i] << 8);
            p.sample.data[i] = (float)(int16_t)v / 32768.0f;
        }
        p.ampEnv = OpenEnva; 
        p.multiEnv = OpenEnvm;
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
    patches[PFM].ampEnv = OpenEnva;
    patches[PFM].multiEnv = OpenEnvm;

    loadRaw(patches[PTHUNEL16], ThunEl16Data, sizeof(ThunEl16Data), false, false);

    patches[PREVTHUN].sample.data.resize(sizeof(ThunEl16Data));
    patches[PREVTHUN].sample.loop = false;
    for (size_t i = 0; i < sizeof(ThunEl16Data); i++)
        patches[PREVTHUN].sample.data[i] = patches[PTHUNEL16].sample.data[sizeof(ThunEl16Data) - 1 - i];

    patches[PREVTHUN].ampEnv = OpenEnva; 
    patches[PREVTHUN].multiEnv = OpenEnvm;

    return patches;
}

Pan panForChannel(int i)
{
    Pan p;
    if (i == 3 || i == 5)
    {
        p.left = 1.0f;
        p.right = std::pow(10.0f, -100.0f / 2000.0f);
    } 
    else if (i % 2 != 0) 
    {
        p.left = std::pow(10.0f, -600.0f / 2000.0f);
        p.right = 1.0f;
    } 
    else 
    {
        p.left = 1.0f;
        p.right = std::pow(10.0f, -600.0f / 2000.0f);
    }
    return p;
}

}

Program build()
{
    Program p;
    p.patches = buildPatches();

    for (int i = 0; i < MAX_TRACKS; i++)
    {
        const TrackSource& src = BootSequence[i];
        TrackDesc t;
        if (src.data) t.code.assign(src.data, src.data + src.count);
        t.pan = panForChannel(i);
        p.tracks.push_back(std::move(t));
    }

    return p;
}

ProgramPtr program()
{
    static const ProgramPtr instance = makeProgram(build());
    return instance;
}

}
}
