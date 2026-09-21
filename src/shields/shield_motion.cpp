#include "shield_motion.h"

#include "shield_config.h"

#include <algorithm>

namespace
{
inline float Rand01(QRand &rng)
{
    static const float mul = 1.0f / 65536.0f;
    return static_cast<float>(rng.Rand() & 0xFFFF) * mul;
}

float PushoutRadius(float elapsed)
{
    using namespace ShieldConfig;
    float p = std::max(0.0f, (PUSHOUT_START_TIME + PUSHOUT_DELTA - elapsed) / PUSHOUT_DELTA);
    return START_PUSHOUT_RADIUS * p * p;
}

void SetRadialTranslation(Matrix &m, float elapsed)
{
    const float push = ShieldConfig::RADIAL_OFFSET + PushoutRadius(elapsed);
    m.m12 = m.m0 * push;
    m.m13 = m.m1 * push;
    m.m14 = m.m2 * push;
}
} // namespace

void SolidShieldMotion::restart(QRand &rng, float scale)
{
    radiusScale = scale;

    const float crossing = Rand01(rng) * 2.09f * PI;

    const float ryArc = PI * 1.2f;
    bool flipped = false;
    const float rz = Rand01(rng) * 2.0f * PI;
    float ry = Rand01(rng) * ryArc * 2.0f - ryArc * 0.5f;
    if (ry > ryArc * 0.5f)
    {
        ry += PI - ryArc;
        flipped = true;
    }

    thetaZero = flipped ? (rz + PI - crossing) : (-rz - crossing);

    startRotation = MatrixMultiply(MatrixRotateY(ry), MatrixRotateZ(rz));
    rotationAxis = {startRotation.m8, startRotation.m9, startRotation.m10};
}

Matrix SolidShieldMotion::matrixAt(float elapsed) const
{
    using namespace ShieldConfig;

    const float t = std::max(elapsed, 0.0f);
    const float theta = thetaZero + SOLID_ROTATION_RATE * 0.5f * SPEED_ACCEL * t * t;

    Matrix m = MatrixMultiply(startRotation, MatrixRotate(rotationAxis, theta));

    m.m0 *= radiusScale;  m.m1 *= radiusScale;  m.m2 *= radiusScale;
    m.m4 *= radiusScale;  m.m5 *= radiusScale;  m.m6 *= radiusScale;
    m.m8 *= radiusScale;  m.m9 *= radiusScale;  m.m10 *= radiusScale;

    SetRadialTranslation(m, t);
    return m;
}

Vector3 SolidShieldMotion::centerFor(const Matrix &pose) const
{
    return Vector3Transform({ShieldConfig::CAP_MID_RADIUS, 0.0f, 0.0f}, pose);
}

void BandShieldMotion::restart(QRand &rng)
{
    thetaZero = Rand01(rng) * 2.0f * PI;
}

Matrix BandShieldMotion::matrixAt(float elapsed) const
{
    using namespace ShieldConfig;

    const float t = std::max(elapsed, 0.0f);
    const float theta = thetaZero + 0.5f * SPEED_ACCEL * t * t;

    Matrix m = MatrixRotateZ(theta);
    SetRadialTranslation(m, t);
    return m;
}
