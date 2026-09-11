#include "raylib.h"
#include "raymath.h"
#include "blob.h"
#include "qrand.h"
#include "defines.h"
#include <algorithm>
#include <cmath>
#include "rlgl.h"

//#define FRAMERATE 60.0f
#define CAPTURE_FRAMERATE 240.0f

//#define DO_CAPTURE 1
#ifdef DO_CAPTURE
#include <cstdio>
#include <filesystem>
#endif

class IntensityDriver
{
public:
    IntensityDriver()
    {
        Init();
    }

    void Init()
    {
        rng.Init();
        timeElapsed = 0.0f;
        smoothedIntensity = intensity = baseIntensity = DEMO_START_INTENSITY;
        iidt = 0.0f;
        InitPulses();
    }

    bool Advance(float dt, Blob& blob)
    {
        if (dt > 1.0f) dt = 0.001f;
        timeElapsed += dt;

        if (timeElapsed < BLOB_ZERO_INTENSE_END_TIME)
        {
            baseIntensity = 0.0f;
        }
        else
        {
            float t = (timeElapsed - BLOB_ZERO_INTENSE_END_TIME) * OO_MAX_INTENSITY_DELTA;
            t = 0.5f * t * t + 0.5f * t;
            baseIntensity = DEMO_START_INTENSITY + t * (1.0f - DEMO_START_INTENSITY);
        }

        float pulses = SumPulses(timeElapsed);
        intensity = baseIntensity + pulses;

        float s = 0.5f * dt;
        smoothedIntensity = (1.0f - s) * smoothedIntensity + s * intensity;
        iidt += dt * intensity;

        if (timeElapsed >= DEMO_TOTAL_TIME)
        {
            if (!loop) return false;

            iidt = 0.0f;
            smoothedIntensity = intensity = baseIntensity = DEMO_START_INTENSITY;
            timeElapsed = 0.0f;
            InitPulses();
            blob.Restart();
        }

        blob.AdvanceTime(timeElapsed, dt);
        return true;
    }

    float GetElapsedTime() const { return timeElapsed; }
    float GetBaseIntensity() const { return baseIntensity; }
    float GetIntensity() const { return intensity; }
    float GetPulseIntensity() const { return intensity - baseIntensity; }

    bool loop = true;

private:
    enum { NUM_PULSES = 12 };
    Vector3 pulses[NUM_PULSES]{};

    QRand rng;
    float timeElapsed = 0.0f;
    float baseIntensity = 0.0f;
    float intensity = 0.0f;
    float smoothedIntensity = 0.0f;
    float iidt = 0.0f;

    float Rand01() { static const float mul = 1.0f / 65536.0f; return (float)(rng.Rand() & 0xFFFF) * mul; }
    float Rand11() { static const float mul = 2.0f / 65536.0f; return (float)(rng.Rand() & 0xFFFF) * mul - 1.0f; }

    void InitPulses()
    {
        for (int i = 0; i < NUM_PULSES; i++)
        {
            float x = (float)(i + 1) / (float)(NUM_PULSES + 1) + Rand11() * 0.03f;
            x = 1.0f - (0.5f * (x * x) + 0.5f * x);

            float temp = ((1.2f - x) * (1.2f - x)) * (Rand01() + 2.0f) * 0.05f;
            float y = std::max(0.1f, temp);
            float z = (x + 0.5f) * (Rand01() + 1.0f) * 0.2f;

            x = x * BLOB_PULSE_ELAPSED + BLOB_PULSE_START;
            x = std::max(x, BLOB_PULSE_START + y);

            pulses[i] = { x, y, z };
        }
        pulses[NUM_PULSES - 1].x = BLOB_PULSE_START + pulses[NUM_PULSES - 1].y;
        pulses[NUM_PULSES - 1].z *= 3.0f;
    }

    float SumPulses(float et) const
    {
        float sum = 0.0f;
        for (int i = 0; i < NUM_PULSES; i++)
        {
            float fdt = fabsf(et - pulses[i].x);
            if (fdt > pulses[i].y) continue;
            float c = cosf(fdt * 0.5f * PI / pulses[i].y);
            sum += pulses[i].z * c;
        }
        return sum;
    }
};

#ifndef DO_CAPTURE
int main()
{
    constexpr int screenWidth = 960;
    constexpr int screenHeight = 720;
    constexpr float distance = 25.0f;
    constexpr float fov = 45.0f;

#ifndef FRAMERATE
    SetConfigFlags(FLAG_VSYNC_HINT);
#endif
    InitWindow(screenWidth, screenHeight, "Xbox Blob");
    rlDisableBackfaceCulling();
#ifdef FRAMERATE
    SetTargetFPS(FRAMERATE);
#endif

    Camera3D camera{
        { 0.0f, distance, -6.0f },
        { 0.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 1.0f },
        fov,
        CAMERA_PERSPECTIVE
    };

    Blob blob;
    IntensityDriver driver;

    while (!WindowShouldClose())
    {
        driver.Advance(GetFrameTime(), blob);

        BeginDrawing();
        ClearBackground(BLACK);

        if (driver.GetElapsedTime() >= BLOB_STATIC_END_TIME)
        {
            BeginMode3D(camera);
            blob.Render(camera, driver.GetPulseIntensity(), driver.GetIntensity(),
                        driver.GetBaseIntensity(), driver.GetElapsedTime());
            EndMode3D();
        }

        DrawFPS(10, 10);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
#else
int main()
{
    constexpr int screenWidth = 1000;
    constexpr int screenHeight = 1000;
    constexpr float distance = 25.0f;
    constexpr float fov = 45.0f;
    constexpr float fixedDt = 1.0f / CAPTURE_FRAMERATE;

    InitWindow(screenWidth, screenHeight, "Xbox Blob");
    std::filesystem::create_directories("frames");
    rlDisableBackfaceCulling();

    Camera3D camera{
        { 0.0f, distance, -6.0f },
        { 0.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 1.0f },
        fov,
        CAMERA_PERSPECTIVE
    };

    Blob blob;
    IntensityDriver driver;
    driver.loop = false;

    int frameNumber = 0;

    while (!WindowShouldClose())
    {
        if (!driver.Advance(fixedDt, blob))
            break;

        BeginDrawing();

        ClearBackground(BLACK);

        if (driver.GetElapsedTime() >= BLOB_STATIC_END_TIME)
        {
            BeginMode3D(camera);

            blob.Render(
                camera,
                driver.GetPulseIntensity(),
                driver.GetIntensity(),
                driver.GetBaseIntensity(),
                driver.GetElapsedTime()
            );

            EndMode3D();
        }

        EndDrawing();

        char filename[256];
        snprintf(
            filename,
            sizeof(filename),
            "frames/frame_%06d.tga",
            frameNumber++
        );

        TakeScreenshot(filename);
    }

    CloseWindow();

    return 0;
}
#endif