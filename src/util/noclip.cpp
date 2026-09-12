#include "noclip.h"
#include "raylib.h"
#include "raymath.h"
#include <algorithm>

void NoclipCamera::SyncFromCamera(const Camera3D& camera)
{
    Vector3 dir = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
    pitch = asinf(std::clamp(dir.z, -1.0f, 1.0f));
    yaw = atan2f(dir.y, dir.x);
}

void NoclipCamera::Toggle(Camera3D& camera, const Camera3D& homeCamera)
{
    active = !active;
    if (active)
    {
        SyncFromCamera(camera);
        DisableCursor();
    }
    else
    {
        camera = homeCamera;
        EnableCursor();
    }
}

void NoclipCamera::Update(Camera3D& camera, float dt)
{
    if (!active) return;

    Vector2 mouseDelta = GetMouseDelta();
    yaw -= mouseDelta.x * mouseSensitivity;
    pitch -= mouseDelta.y * mouseSensitivity;

    constexpr float pitchLimit = 89.0f * DEG2RAD;
    pitch = std::clamp(pitch, -pitchLimit, pitchLimit);

    Vector3 forward = Vector3Normalize(Vector3{
        cosf(pitch) * cosf(yaw),
        cosf(pitch) * sinf(yaw),
        sinf(pitch)
    });

    constexpr Vector3 worldUp{ 0.0f, 0.0f, 1.0f };
    Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, worldUp));

    float speed = moveSpeed;
    if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT))
        speed *= sprintMultiplier;

    Vector3 move{};
    if (IsKeyDown(KEY_W)) move = Vector3Add(move, forward);
    if (IsKeyDown(KEY_S)) move = Vector3Subtract(move, forward);
    if (IsKeyDown(KEY_D)) move = Vector3Add(move, right);
    if (IsKeyDown(KEY_A)) move = Vector3Subtract(move, right);
    if (IsKeyDown(KEY_SPACE)) move.z += 1.0f;
    if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) move.z -= 1.0f;

    if (Vector3LengthSqr(move) > 0.0f)
        move = Vector3Normalize(move);

    camera.position = Vector3Add(camera.position, Vector3Scale(move, speed * dt));
    camera.target = Vector3Add(camera.position, forward);
    camera.up = worldUp;
}
