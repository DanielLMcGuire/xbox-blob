#pragma once

#include "raymath.h"

#include <cmath>

inline Vector3 RotateLH(Vector3 v, Quaternion q)
{
    return Vector3RotateByQuaternion(v, QuaternionInvert(q));
}

inline Vector3 RotateRH(Vector3 v, Quaternion q)
{
    return Vector3RotateByQuaternion(v, q);
}

inline Matrix MatrixRotateLH(Quaternion q)
{
    return QuaternionToMatrix(QuaternionInvert(q));
}

inline void SinCos(const float& a, float* ps, float* pc)
{
    *ps = std::sin(a);
    *pc = std::cos(a);
}