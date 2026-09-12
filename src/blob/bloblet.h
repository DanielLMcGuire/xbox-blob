#pragma once

#include "raylib.h"

struct Bloblet
{
    float radius = 0.0f;
    Vector3 position{ 0, 0, 0 };
    bool farSide = false;

    Vector3 direction{ 0, 0, 1 };
    float startTime = 0.0f;
    float timeMultiple = 0.0f;
    float maxDist = 0.0f;

    float curDist = 0.0f;

    float wobble = 1.0f;
    float wobbleDirection = 0.0f;

    void Init() { wobble = 1.0f; wobbleDirection = 0.0f; }

    bool Update(float elapsedTime, float dt);
};
