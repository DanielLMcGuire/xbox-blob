#pragma once
#include <cstdint>

#if defined(_MSC_VER)
    #if defined(_M_IX86)
        #define HAS_VC_X86
    #elif defined(__x86_64__)
        #define HAS_VC_X64
    #endif
#elif defined(__i386__) || defined(__x86_64__)
    #define HAS_GNU_X86
#endif

namespace X86E
{
    inline constexpr uint32_t MulHigh32(uint32_t a, uint32_t b)
    {
        return static_cast<uint32_t>((static_cast<uint64_t>(a) * b) >> 32);
    }

    struct RclResult
    {
        uint32_t value;
        bool carryOut;
    };

    inline constexpr RclResult RotateLeftThroughCarry32(uint32_t value, int count, bool carryIn)
    {
        count %= 33;
        if (count == 0)
            return { value, carryIn };

        constexpr uint64_t kMask33 = (uint64_t{ 1 } << 33) - 1;
        const uint64_t word = (uint64_t{ carryIn } << 32) | value;
        const uint64_t rotated = ((word << count) | (word >> (33 - count))) & kMask33;

        return { static_cast<uint32_t>(rotated & 0xFFFFFFFFu), ((rotated >> 32) & 1u) != 0 };
    }

    inline constexpr uint32_t ClampedUnsignedSub(uint32_t a, uint32_t b)
    { return (a >= b) ? (a - b) : 0u; }

    inline constexpr uint32_t ClampedSignedSub(uint32_t a, uint32_t b)
    {
        const auto signedA = static_cast<int32_t>(a);
        const auto signedB = static_cast<int32_t>(b);
        const int64_t signedDiff = static_cast<int64_t>(signedA) - static_cast<int64_t>(signedB);
        return (signedDiff < 0) ? 0u : (a - b);
    }
}