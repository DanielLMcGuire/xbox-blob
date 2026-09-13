#include "blob_bump.h"

#include "bloblet.h"
#include "blob.h"
#include "blob_math.h"
#include "../defines.h"
#include "../util/qrand.h"

#include "raymath.h"
#include <cstdint>
#include <cmath>
#include <algorithm>

static QRand g_BlobRand;

constexpr int32_t LLI_RAND_MAX = 0x00010000;
constexpr int32_t LLI_RAND_MASK = 0x0000FFFF;

float BlobRandom01()
{
    static const float mul = 1.0f / (float)LLI_RAND_MAX;
    return (float)(g_BlobRand.Rand() & LLI_RAND_MASK) * mul;
}

float BlobRandom11()
{
    static const float mul = 2.0f / (float)LLI_RAND_MAX;
    return (float)(g_BlobRand.Rand() & LLI_RAND_MASK) * mul - 1.0f;
}

void BlobSetRandomSeed(int32_t seed)
{
    g_BlobRand.SetSeed(seed);
}

void BlobBump::RecalculateFacesOfInterest()
{
    facesOfInterest =
        ((direction.x - radius < -0.57735f) ? 0x0001 : 0) +
        ((direction.y - radius < -0.57735f) ? 0x0002 : 0) +
        ((direction.z - radius < -0.57735f) ? 0x0004 : 0) +
        ((direction.x + radius > +0.57735f) ? 0x0008 : 0) +
        ((direction.y + radius > +0.57735f) ? 0x0010 : 0) +
        ((direction.z + radius > +0.57735f) ? 0x0020 : 0);
}

bool BlobBump::Create(float curTime, Bloblet* bloblet)
{
    if (curTime < 0.0f) myBloblet = nullptr;

    direction = { BlobRandom11(), BlobRandom11(), BlobRandom11() };
    if (Vector3LengthSqr(direction) < 0.001f)
        direction = { BlobRandom11(), BlobRandom11(), 1.0f };

    position = { 0, 0, 0 };
    QuickNormalize(&direction);

    float timeProg = std::max(0.0f, (curTime - BLOB_STATIC_END_TIME) * OO_MAX_INTENSITY_DELTA);
    float radMagRand = BlobRandom01();

    radius = radMagRand * 0.4f + 0.4f;
    radius2 = radius * radius;
    ooRadius2 = 1.0f / radius2;
    magnitude = 0.0f;

    RecalculateFacesOfInterest();

    startTime = curTime + 0.4f * BlobRandom01();

    maxMagnitude = (1.0f - radMagRand) * 0.5f + 0.2f;
    maxMagnitude *= 0.5f + 0.5f * timeProg;

    if (!myBloblet) myBloblet = bloblet;
    if (myBloblet)
    {
        float mainRad = g_Blob->GetRadius();
        myBloblet->radius = (BlobRandom01() + 1.0f) * 0.25f * mainRad * radius;
        myBloblet->direction = direction;

        myBloblet->maxDist = mainRad * (5.0f + BlobRandom11() * 2.0f);
        myBloblet->maxDist *= 0.6f;

        myBloblet->startTime = (curTime < -1.0f) ? -BlobRandom01() * 0.3f : curTime;

        float period = 0.8f + 0.3f * BlobRandom01();
        period *= 1.0f / 0.6f;
        myBloblet->timeMultiple = 2.0f * PI / period;

        myBloblet->wobble = 1.2f;
        myBloblet->wobbleDirection = 0.0f;

        stillAttachedToBloblet = (curTime - startTime < 0.4f * period);

        myBloblet->Update(curTime, 0.0f);
        Update(curTime, 0.0f, nullptr);
    }
    else
    {
        float sequenceLen = maxMagnitude * 0.3f + BlobRandom01() * 0.3f;
        timeMul = PI / sequenceLen;
        timeMul *= timeProg * 0.2f + 0.8f;

        if (curTime < -1.0f) startTime = -BlobRandom01() * PI / timeMul;
    }

    return (myBloblet != nullptr);
}

bool BlobBump::Update(float elapsedTime, float dt, Bloblet* bloblet)
{
    if (myBloblet)
    {
        float bMag = (fabsf(myBloblet->curDist) + myBloblet->radius) / g_Blob->GetRadius();
        magnitude = std::min(2.0f, std::max(0.0f, bMag - 1.0f));

        if (stillAttachedToBloblet)
        {
            if (magnitude > 0.8f)
            {
                stillAttachedToBloblet = false;
                maxMagnitude = magnitude;
                float sequenceLen = 0.3f * magnitude;
                timeMul = 2.0f * PI / sequenceLen;
                startTime = elapsedTime - 0.25f * sequenceLen;
                myBloblet->wobble = std::max(0.6f, std::min(0.8f, magnitude - 0.5f));
                myBloblet->wobbleDirection = 0.0f;
            }
            else
            {
                if ((Vector3DotProduct(direction, myBloblet->direction) < 0.0f) != myBloblet->farSide)
                {
                    direction = Vector3Scale(direction, -1.0f);
                    position = direction;
                    RecalculateFacesOfInterest();
                }
                return false;
            }
        }

        if (!stillAttachedToBloblet)
        {
            if (bMag < 0.9f)
                stillAttachedToBloblet = true;
        }
    }

    float t = (elapsedTime - startTime) * timeMul;
    if (t > PI)
    {
        if (myBloblet == nullptr)
            return Create(elapsedTime, bloblet);
        magnitude = 0.0f;
        return false;
    }
    if (t < 0.0f) return false;

    float sinVal = sinf(t);
    magnitude = maxMagnitude * sinVal;
    position = direction;
    RecalculateFacesOfInterest();

    return false;
}
