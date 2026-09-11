#pragma once
#include <cstdint>

struct DSENVELOPEDESC {
    uint32_t dwEG; 
    uint32_t dwMode; 
    uint32_t dwDelay; 
    uint32_t dwAttack;
    uint32_t dwHold; 
    uint32_t dwDecay; 
    uint32_t dwRelease; 
    uint32_t dwSustain;
    int32_t  lPitchScale; 
    int32_t  lFilterCutOff;
};