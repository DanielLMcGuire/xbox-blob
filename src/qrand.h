#pragma once
#include <cstdint>
#include "defines.h"

class QRand
{
public:
    void Init(int32_t initSeed = 0x76543210) { seed = initSeed; }
    void SetSeed(int32_t newSeed) { seed = newSeed; }

    int32_t Rand()
    {
#if defined(USE_ASM_MSVC_X86)
        int32_t result;
        __asm
        {
            mov     edi, this
            mov     eax, [edi]
            mov     ebx, eax
            ror     eax, 13
            sub     ebx, 11
            sub     eax, ebx
            mov     [edi], eax
            mov     result, eax
        }
        return result;
#elif defined(USE_ASM_GCC_X86)
        int32_t result;
        asm volatile (
            "movl %[seed], %%eax\n\t"
            "movl %%eax, %%ebx\n\t"
            "rorl $13, %%eax\n\t"
            "subl $11, %%ebx\n\t"
            "subl %%ebx, %%eax\n\t"
            "movl %%eax, %[seed]\n\t"
            "movl %%eax, %[res]"
            : [seed] "+m" (seed), [res] "=r" (result)
            :
            : "eax", "ebx", "cc"
        );
        return result;
#else
        uint32_t eax = (uint32_t)seed;
        uint32_t ebx = eax;
        eax = (eax >> 13) | (eax << (32 - 13));
        ebx -= 11;
        eax -= ebx;
        seed = (int32_t)eax;
        return seed;
#endif
    }

    int32_t Rand(int32_t scale)
    {
#if defined(USE_ASM_MSVC_X86)
        int32_t result;
        __asm
        {
            mov     edi, this
            mov     eax, [edi]
            mov     ebx, eax
            ror     eax, 13
            sub     ebx, 11
            sub     eax, ebx
            mov     [edi], eax
            mul     scale
            mov     eax, edx
            mov     result, eax
        }
        return result;
#elif defined(USE_ASM_GCC_X86)
        int32_t result;
        asm volatile (
            "movl %[seed], %%eax\n\t"
            "movl %%eax, %%ebx\n\t"
            "rorl $13, %%eax\n\t"
            "subl $11, %%ebx\n\t"
            "subl %%ebx, %%eax\n\t"
            "movl %%eax, %[seed]\n\t"
            "mull %[scale]\n\t"
            "movl %%edx, %[res]"
            : [seed] "+m" (seed), [res] "=r" (result)
            : [scale] "r" (scale)
            : "eax", "ebx", "edx", "cc"
        );
        return result;
#else
        uint32_t eax = (uint32_t)seed;
        uint32_t ebx = eax;
        eax = (eax >> 13) | (eax << (32 - 13));
        ebx -= 11;
        eax -= ebx;
        seed = (int32_t)eax;

        uint64_t product = (uint64_t)eax * (uint32_t)scale;
        return (int32_t)(product >> 32);
#endif
    }

private:
    int32_t seed = 0x76543210;
};