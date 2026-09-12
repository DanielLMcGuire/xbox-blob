#pragma once

#include "raymath.h"
#include <cmath>

inline float QuickLength(const Vector3& v)
{
    float h = fabsf(v.x), m = fabsf(v.y), l = fabsf(v.z), t;
    if (m > h) { t = m; m = h; h = t; }
    if (l > m) { t = l; l = m; m = t; }
    if (m > h) { t = m; m = h; h = t; }
    return 1.043388475f * (h + 0.34375f * m + 0.25f * l);
}

inline void QuickNormalize(Vector3* v)
{
    float qlen = QuickLength(*v);
    if (qlen < 0.000001f) return;
    float ooQlen = 1.0f / qlen;
    v->x *= ooQlen; v->y *= ooQlen; v->z *= ooQlen;
}

inline Vector3 AddScaled(Vector3 target, const Vector3& src, float scale)
{
    target.x += src.x * scale;
    target.y += src.y * scale;
    target.z += src.z * scale;
    return target;
}
