#define track(n) const int16_t n[] =

#include "0"
#include "1"
#include "2"
#include "3"
#include "4"
#include "5"
#include "6"
#include "7"
#include "8"
#include "9"
#include "A"
#include "B"

#undef track

struct TrackSource { const int16_t* data; size_t count; };
#define TRACK_DEF(arr) { arr, sizeof(arr) / sizeof(arr[0]) }

const TrackSource BootSequence[SOS::MAX_TRACKS] = {
    TRACK_DEF(Boot0), 
    TRACK_DEF(Boot1), 
    TRACK_DEF(Boot2), 
    TRACK_DEF(Boot3),
    TRACK_DEF(Boot4), 
    TRACK_DEF(Boot5), 
    TRACK_DEF(Boot6), 
    TRACK_DEF(Boot7),
    TRACK_DEF(Boot8), 
    TRACK_DEF(Boot9), 
    TRACK_DEF(BootA), 
    TRACK_DEF(BootB)
};