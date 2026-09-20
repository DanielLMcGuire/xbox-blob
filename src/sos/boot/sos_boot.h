#pragma once

#include "../sos_program.h"

namespace SOS
{

enum PatchID
{
    PSIN1 = 0, PSAW1 = 1, PSQUARE = 2, PSAW2 = 3, PSAW3 = 4,
    PNOISE1 = 5, PGLOCK = 6, PBUBBLE = 7, PFM = 8, PTHUNEL16 = 9, PREVTHUN = 10
};

namespace Boot
{

Program build();

ProgramPtr program();

}
}
