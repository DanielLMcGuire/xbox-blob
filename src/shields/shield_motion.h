#pragma once

#include "raylib.h"
#include "raymath.h"

#include "../util/qrand.h"

class SolidShieldMotion
{
public:
    void restart(QRand &rng, float radiusScale);

    Matrix matrixAt(float elapsed) const;

    Vector3 centerFor(const Matrix &pose) const;

private:
    Matrix startRotation = MatrixIdentity();
    Vector3 rotationAxis = {0.0f, 0.0f, 1.0f};
    float thetaZero = 0.0f;
    float radiusScale = 1.0f;
};

class BandShieldMotion
{
public:
    void restart(QRand &rng);

    Matrix matrixAt(float elapsed) const;

private:
    float thetaZero = 0.0f;
};
