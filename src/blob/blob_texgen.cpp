#include "blob.h"
#include "../util/x86-emu.h"

namespace {
    template <int width>
    constexpr int ComputeGlowScale()
    {
        static_assert(
            width > 0 &&
            (4096 % width) == 0 &&
            (((4096 / width) & ((4096 / width) - 1)) == 0),
            "4096/WIDTH must be a power of two"
        );

        int tmp = 4096 / width;
        int result = 1;

        while (tmp != 1)
        {
            ++result;
            tmp >>= 1;
        }

        return result;
    }
}

void Blob::BuildGlowTexture()
{
    constexpr int WIDTH = 256;
    constexpr int HEIGHT = 256;

    constexpr uint32_t NOISE = 0;
    constexpr uint32_t INITIAL_SEED = 12345;

    Image img = GenImageColor(WIDTH, HEIGHT, BLANK);
    uint32_t* pPixels = static_cast<uint32_t*>(img.data);

    int scale = ComputeGlowScale<WIDTH>();
    int cntrx = (WIDTH - 1) / 2;
    int cntry = (HEIGHT - 1) / 2;
    uint32_t seed = INITIAL_SEED;

    for (int y = 0; y < HEIGHT; y++)
    {
        for (int x = 0; x < WIDTH; x++)
        {
            uint32_t* pCurrentPixel = &pPixels[y * WIDTH + x];

#if defined(USE_ASM_MSVC_X86)
            _asm {
                mov     ecx, scale
                mov     eax, x
                mov     ebx, y
                sub     eax, cntrx
                sub     ebx, cntry
                sal     eax, cl
                imul    eax
                sal     ebx, cl
                xchg    eax, ebx
                mov     edi, pCurrentPixel
                imul    eax
                mov     edx, 16777216
                add     ebx, eax
                sub     edx, ebx
                jnc     noOverflow1
                xor     edx, edx
            noOverflow1:
                mov     ebx, edx
                mov     eax, ebx
                mul     NOISE
                mov     ecx, seed
                mov     eax, edx
                mov     edx, ecx
                rcl     ecx, 13
                sub     edx, 11
                sub     ecx, edx
                mov     seed, ecx
                mul     ecx
                shl     edx, 15
                sub     ebx, edx	
                jge     bxOk1
                xor     ebx, ebx
            bxOk1:
                and     ebx, 0x1ff0000
                rcl     ebx, 8
                sbb     ebx, 0
                mov     eax, ebx
                shr     eax, 24
                mul     al
                mul     eax
                shr     eax, 16
                mul     eax
                shr     eax, 16
                and     eax, 0xff00
                mov     ecx, eax
                shr     ecx, 8
                or      ecx, eax
                mov     eax, ecx
                shl     ecx, 16
                or      ecx, eax
                mov     [edi], ecx
            }
#elif defined(USE_ASM_GCC_X86)
            asm volatile (
                "movl %[scale], %%ecx\n\t"
                "movl %[x], %%eax\n\t"
                "movl %[y], %%ebx\n\t"
                "subl %[cntrx], %%eax\n\t"
                "subl %[cntry], %%ebx\n\t"
                "sall %%cl, %%eax\n\t"
                "imull %%eax, %%eax\n\t"
                "sall %%cl, %%ebx\n\t"
                "xchgl %%eax, %%ebx\n\t"
                "imull %%eax, %%eax\n\t"
                "movl $16777216, %%edx\n\t"
                "addl %%eax, %%ebx\n\t"
                "subl %%ebx, %%edx\n\t"
                "jnc 1f\n\t"
                "xorl %%edx, %%edx\n\t"
                "1:\n\t"
                "movl %%edx, %%ebx\n\t"
                "movl %%ebx, %%eax\n\t"
                "mull %[noise]\n\t"
                "movl %[seed], %%ecx\n\t"
                "movl %%edx, %%eax\n\t"
                "movl %%ecx, %%edx\n\t"
                "rcll $13, %%ecx\n\t"
                "subl $11, %%edx\n\t"
                "subl %%edx, %%ecx\n\t"
                "movl %%ecx, %[seed]\n\t"
                "mull %%ecx\n\t"
                "shll $15, %%edx\n\t"
                "subl %%edx, %%ebx\n\t"
                "jge 2f\n\t"
                "xorl %%ebx, %%ebx\n\t"
                "2:\n\t"
                "andl $0x1ff0000, %%ebx\n\t"
                "rcll $8, %%ebx\n\t"
                "sbbl $0, %%ebx\n\t"
                "movl %%ebx, %%eax\n\t"
                "shrl $24, %%eax\n\t"
                "mulb %%al\n\t"
                "mull %%eax\n\t"
                "shrl $16, %%eax\n\t"
                "mull %%eax\n\t"
                "shrl $16, %%eax\n\t"
                "andl $0xff00, %%eax\n\t"
                "movl %%eax, %%ecx\n\t"
                "shrl $8, %%ecx\n\t"
                "orl %%eax, %%ecx\n\t"
                "movl %%ecx, %%eax\n\t"
                "shll $16, %%ecx\n\t"
                "orl %%eax, %%ecx\n\t"
                "movl %%ecx, (%[pixel])"
                : [seed] "+r" (seed)
                : [x] "r" (x), [y] "r" (y), [cntrx] "r" (cntrx), [cntry] "r" (cntry), 
                  [scale] "r" (scale), [noise] "r" (NOISE), [pixel] "r" (pCurrentPixel)
                : "eax", "ebx", "ecx", "edx", "cc", "memory"
            );
#else
            int dx = x - cntrx;
            int dy = y - cntry;

            const int32_t dxScaled = static_cast<int32_t>(static_cast<uint32_t>(dx) << scale);
            const int32_t dyScaled = static_cast<int32_t>(static_cast<uint32_t>(dy) << scale);

            const uint32_t distSq =
                static_cast<uint32_t>(static_cast<int64_t>(dxScaled) * dxScaled) +
                static_cast<uint32_t>(static_cast<int64_t>(dyScaled) * dyScaled);

            uint32_t ebxVal = X86E::ClampedUnsignedSub(16777216u, distSq);

            const uint32_t noiseHigh = X86E::MulHigh32(ebxVal, NOISE);
            const bool carryFromNoiseMul = (noiseHigh != 0);

            const uint32_t rotatedSeed = X86E::RotateLeftThroughCarry32(seed, 13, carryFromNoiseMul).value;
            seed = rotatedSeed - (seed - 11);

            const uint32_t seedMulHighShifted = X86E::MulHigh32(noiseHigh, seed) << 15;
            ebxVal = X86E::ClampedSignedSub(ebxVal, seedMulHighShifted);

            ebxVal &= 0x1ff0000u;
            const X86E::RclResult rotated = X86E::RotateLeftThroughCarry32(ebxVal, 8, false);
            ebxVal = rotated.value - (rotated.carryOut ? 1u : 0u);

            uint32_t eax = ebxVal >> 24;
            eax = eax * eax;
            eax = static_cast<uint32_t>((static_cast<uint64_t>(eax) * eax) >> 16);
            eax = static_cast<uint32_t>((static_cast<uint64_t>(eax) * eax) >> 16);
            eax &= 0xff00u;

            const uint8_t v = static_cast<uint8_t>(eax >> 8);
            *pCurrentPixel = (uint32_t(v) << 24) | (uint32_t(v) << 16) | (uint32_t(v) << 8) | v;
#endif
        }
    }

    glowTexture = LoadTextureFromImage(img);
    UnloadImage(img);
    SetTextureFilter(glowTexture, TEXTURE_FILTER_BILINEAR);
    SetTextureWrap(glowTexture, TEXTURE_WRAP_CLAMP);
}