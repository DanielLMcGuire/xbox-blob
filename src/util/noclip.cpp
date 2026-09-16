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

void NoclipCamera::Toggle(
    Camera3D& camera,
    const Camera3D& homeCamera,
    bool switchCamera
)
{
    if (!active)
    {
        SyncFromCamera(camera);
        active = true;

        if (IsGamepadAvailable(0))
            EnableCursor();
        else
            DisableCursor();

        return;
    }

    active = false;

    if (switchCamera)
        camera = homeCamera;

    EnableCursor();
}

void NoclipCamera::Update(Camera3D& camera, float dt)
{
    if (!active) return;

    constexpr int gamepad = 0;
    constexpr float stickDeadzone = 0.15f;

    const bool hasGamepad = IsGamepadAvailable(gamepad);

    if (!hasGamepad)
    {
        Vector2 mouseDelta = GetMouseDelta();
        yaw -= mouseDelta.x * mouseSensitivity;
        pitch -= mouseDelta.y * mouseSensitivity;
    }
    else
    {
        Vector2 look{
            GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_RIGHT_X),
            GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_RIGHT_Y)
        };

        if (Vector2LengthSqr(look) > stickDeadzone * stickDeadzone)
        {
            yaw -= look.x * gamepadLookSensitivity * dt;
            pitch -= look.y * gamepadLookSensitivity * dt;
        }
    }

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

    if (hasGamepad)
    {
        float trigger = GetGamepadAxisMovement(
            gamepad,
            GAMEPAD_AXIS_RIGHT_TRIGGER
        );

        trigger = std::clamp(trigger, 0.0f, 1.0f);
        speed *= Lerp(1.0f, sprintMultiplier, trigger * 2.0f);
    }
    else if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT))
    {
        speed *= sprintMultiplier;
    }

    Vector3 move{};

    if (hasGamepad)
    {
        float x = GetGamepadAxisMovement(
            gamepad,
            GAMEPAD_AXIS_LEFT_X
        );

        float y = GetGamepadAxisMovement(
            gamepad,
            GAMEPAD_AXIS_LEFT_Y
        );

        if (fabsf(x) < stickDeadzone) x = 0.0f;
        if (fabsf(y) < stickDeadzone) y = 0.0f;

        move = Vector3Add(move, Vector3Scale(right, x));
        move = Vector3Add(move, Vector3Scale(forward, -y));

        if (IsGamepadButtonDown(
                gamepad,
                GAMEPAD_BUTTON_RIGHT_FACE_DOWN))
        {
            move.z += 1.0f;
        }

        if (IsGamepadButtonDown(
                gamepad,
                GAMEPAD_BUTTON_RIGHT_FACE_RIGHT))
        {
            move.z -= 1.0f;
        }
    }
    else
    {
        if (IsKeyDown(KEY_W))
            move = Vector3Add(move, forward);

        if (IsKeyDown(KEY_S))
            move = Vector3Subtract(move, forward);

        if (IsKeyDown(KEY_D))
            move = Vector3Add(move, right);

        if (IsKeyDown(KEY_A))
            move = Vector3Subtract(move, right);

        if (IsKeyDown(KEY_SPACE))
            move.z += 1.0f;

        if (IsKeyDown(KEY_LEFT_CONTROL) ||
            IsKeyDown(KEY_RIGHT_CONTROL))
        {
            move.z -= 1.0f;
        }
    }

    if (Vector3LengthSqr(move) > 0.0f)
        move = Vector3Normalize(move);

    camera.position = Vector3Add(
        camera.position,
        Vector3Scale(move, speed * dt)
    );

    camera.target = Vector3Add(camera.position, forward);
    camera.up = worldUp;
}