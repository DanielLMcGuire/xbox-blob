#pragma once

#include <bit>
#include <cstdint>

constexpr int32_t defSeed = 0x76543210;

class QRand
{
public:
    QRand() { Init(defSeed); }
    QRand(int32_t initSeed) { Init(initSeed); }
    ~QRand() = default;

    void Init(int32_t initSeed) { seed = initSeed; }
    void SetSeed(int32_t newSeed) { seed = newSeed; }

    inline int32_t Rand()
    {
        uint32_t s = static_cast<uint32_t>(seed);
        return seed = static_cast<int32_t>(std::rotr(s, 13) - (s - 11));
    }

    inline int32_t Rand(int32_t scale)
    {
        uint32_t s = static_cast<uint32_t>(seed);
        s = std::rotr(s, 13) - (s - 11);
        seed = static_cast<int32_t>(s);

        return static_cast<int32_t>(((uint64_t)s * (uint32_t)scale) >> 32);
    }

private:
    int32_t seed = defSeed;
};