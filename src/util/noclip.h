#pragma once
#include "raylib.h"

struct NoclipCamera
{
    bool active = false;
    float yaw = 0.0f;
    float pitch = 0.0f;

    float moveSpeed = 14.0f;
    float sprintMultiplier = 3.0f;
    float mouseSensitivity = 0.0020f;

    void SyncFromCamera(const Camera3D& camera);

    void Toggle(Camera3D& camera, const Camera3D& homeCamera, bool switchCamera);

    void Update(Camera3D& camera, float dt);
};