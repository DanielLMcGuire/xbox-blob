#pragma once

#include "raylib.h"

#include <cstdint>
#include <vector>

class SceneAnimTables
{
public:
    void create();

    void advanceTime(float fpos);

    std::vector<Quaternion> quats;
    std::vector<std::vector<int16_t>> quatIdSeq;
    std::vector<Vector3> pos;
    std::vector<std::vector<int16_t>> posIdSeq;

    std::vector<Quaternion> rotAnims;
    std::vector<Vector3> posAnims;

    static std::vector<int16_t> decompressIndices(const char *p_indices, int nindices);
};
