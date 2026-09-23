#include "bloblet.h"

#include "../defines.h"
#include "blob.h"
#include "blob_math.h"

#include <cmath>
#include <algorithm>

constexpr float WOBBLE_ACCEL = 1000.0f;

bool Bloblet::Update(float elapsedTime, float dt)
{
    wobble = std::min(2.0f, std::max(0.5f, wobble + wobbleDirection * dt));
    if (wobbleDirection > 0.0f)
    {
        if ((wobble < 0.95f) || (wobble > 1.0f))
            wobbleDirection -= (wobble - 1.0f) * dt * WOBBLE_ACCEL;
    }
    else
    {
        if ((wobble < 1.0f) || (wobble > 1.05f))
            wobbleDirection -= (wobble - 1.0f) * dt * WOBBLE_ACCEL;
    }

    float timeProg = std::max(0.0f, (elapsedTime - BLOB_STATIC_END_TIME) * OO_MAX_INTENSITY_DELTA);

    float t = timeMultiple * (elapsedTime - startTime);
    t *= 1.4f * (1.0f + elapsedTime / 10.0f);

    float s = sinf(t);
    float sm = fabsf(s);
    sm = 1.0f - (1.0f - sm) * sqrtf(1.0f - sm);
    s = (s > 0.0f) ? sm : -sm;

    curDist = maxDist * s * timeProg;
    farSide = (curDist < 0.0f);

    position = AddScaled(g_Blob->GetCenter(), direction, curDist);

    return (fabsf(curDist) + radius < g_Blob->GetRadius() * 0.5f);
}
