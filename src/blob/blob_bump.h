#pragma once

#include "raylib.h"

struct Bloblet;

class BlobBump
{
public:
    void Init() { myBloblet = nullptr; }

    bool Create(float curTime, Bloblet* bloblet);

    bool Update(float elapsedTime, float dt, Bloblet* bloblet);

    Vector3 position{ 0, 0, 0 };
    float radius = 0.0f, radius2 = 0.0f, ooRadius2 = 0.0f;
    float magnitude = 0.0f;
    int facesOfInterest = 0;
private:
    Vector3 direction{ 0, 0, 1 };
    float startTime = 0.0f;
    float timeMul = 0.0f;
    float maxMagnitude = 0.0f;

    Bloblet* myBloblet = nullptr;
    bool stillAttachedToBloblet = false;

    void RecalculateFacesOfInterest();
};

float BlobRandom01();
float BlobRandom11();
