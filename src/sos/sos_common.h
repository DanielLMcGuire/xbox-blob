#pragma once

#include <cstddef>
#include <cstdint>

namespace SOS 
{

constexpr int    SAMPLE_RATE      = 48000;
constexpr int    SAMPLES_PER_TICK = 240;
constexpr int    MAX_TRACKS       = 12;
constexpr size_t MAX_LOOP_DEPTH   = 8;
constexpr int    CONTROL_INTERVAL = 16;
constexpr float  PAUSE_FADE_SECONDS = 0.02f;
constexpr float  SEEK_CHECKPOINT_SECONDS = 0.5f;
constexpr int    SEEK_BLOCK_FRAMES = 4800;
constexpr double SEEK_MAX_SECONDS = 30.0;

enum OpCode
{
    F_REST = 0, F_NOTE = 1, F_JUMPTO = 2, F_LOOP = 3, F_ENDLOOP = 4,
    F_PATCH = 5, F_PAN = 6, F_MUX = 7, F_DEMUX = 8, F_VOLUME = 9,
    F_XPOSE = 10, F_XSET = 11, F_SLUR = 12, F_RING = 13, F_CLOCKSET = 14,
    F_END = 15, F_FILTERINC = 16, F_FILTERSET = 17
};

#define note(pitch, dur)   SOS::F_NOTE, pitch, dur
#define rest(dur)          SOS::F_REST, dur
#define loop(n)            SOS::F_LOOP, n
#define endloop            SOS::F_ENDLOOP
#define patch(num)         SOS::F_PATCH, num
#define volume(val)        SOS::F_VOLUME, val
#define xpose(val)         SOS::F_XPOSE, val
#define slur(pitch, dur)   SOS::F_SLUR, pitch, dur
#define ring(dur)          SOS::F_RING, dur
#define finc(f, res)       SOS::F_FILTERINC, f, res
#define fset(f, res)       SOS::F_FILTERSET, f, res

enum PatchID
{
    PSIN1 = 0, PSAW1 = 1, PSQUARE = 2, PSAW2 = 3, PSAW3 = 4,
    PNOISE1 = 5, PGLOCK = 6, PBUBBLE = 7, PFM = 8, PTHUNEL16 = 9, PREVTHUN = 10
};

}

enum Pitches
{
    cc0=0,  cs0=1,  dd0=2,  ds0=3,  ee0=4,  ff0=5,  fs0=6,  gg0=7,  gs0=8,  aa0=9,  as0=10, bb0=11,
    cc1=12, cs1=13, dd1=14, ds1=15, ee1=16, ff1=17, fs1=18, gg1=19, gs1=20, aa1=21, as1=22, bb1=23,
    cc2=24, cs2=25, dd2=26, ds2=27, ee2=28, ff2=29, fs2=30, gg2=31, gs2=32, aa2=33, as2=34, bb2=35,
    cc3=36, cs3=37, dd3=38, ds3=39, ee3=40, ff3=41, fs3=42, gg3=43, gs3=44, aa3=45, as3=46, bb3=47,
    cc4=48, cs4=49, dd4=50, ds4=51, ee4=52, ff4=53, fs4=54, gg4=55, gs4=56, aa4=57, as4=58, bb4=59,
    cc5=60, cs5=61, dd5=62, ds5=63, ee5=64, ff5=65, fs5=66, gg5=67, gs5=68, aa5=69, as5=70, bb5=71
};