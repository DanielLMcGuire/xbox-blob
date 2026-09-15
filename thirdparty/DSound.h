#pragma once

// windef.h
typedef unsigned long DWORD;
typedef long LONG;



typedef struct _DSENVELOPEDESC
{
    DWORD           dwEG;
    DWORD           dwMode;
    DWORD           dwDelay;
    DWORD           dwAttack;
    DWORD           dwHold;
    DWORD           dwDecay;
    DWORD           dwRelease;
    DWORD           dwSustain;
    LONG            lPitchScale;
    LONG            lFilterCutOff;
} DSENVELOPEDESC, *LPDSENVELOPEDESC;

typedef const DSENVELOPEDESC *LPCDSENVELOPEDESC;