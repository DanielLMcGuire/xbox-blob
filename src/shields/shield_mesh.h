#pragma once

#include "raylib.h"

#include <cstdint>
#include <vector>

struct ShieldVertex
{
    Vector3 position;
    Vector3 normal;
};

struct ShieldMesh
{
    std::vector<ShieldVertex> vertices;
    std::vector<uint16_t> indices;
    int vertsPerFace = 0;
    int edgeVerts = 0;
};

ShieldMesh BuildShieldCap(int width, int height, float insideRadius, float outsideRadius, float horizDim,
                          float vertDim);

ShieldMesh BuildShieldBand(int width, int height, float insideRadius, float outsideRadius, float horizRadians,
                           float startLatitude, float endLatitude);
