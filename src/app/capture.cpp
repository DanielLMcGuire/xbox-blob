#include "app.h"
#include "rlgl.h"
#include "imgui.h"
#include "rlImGui.h"

#include <algorithm>
#include <cmath>

#if defined(_WIN32)
    #undef DrawText
#endif

void XboxStartup::updateCapture()
{
    const float fixedDt = 1.0f / static_cast<float>(framerate);

    if (!driver->Advance(fixedDt, *blob))
    {
        running = false; 
        return;
    }

    currentElapsedTime = driver->GetElapsedTime();
    const bool isBlobStaticEnded = (currentElapsedTime >= BLOB_STATIC_END_TIME);

    Vector3 splinePos, splineLook;
    float fCamTime = (currentElapsedTime * currentElapsedTime / 6.0f) * 0.8f;
    fCamTime += 0.2f * (currentElapsedTime * sinf(std::min(3.14159265354f / 2.0f, currentElapsedTime * 0.2833f)));

    camController.getPosition(fCamTime, &splinePos, &splineLook, &renderSceneGeom, &renderSlash);

    camera.position = splinePos;
    camera.target = splineLook;
    camera.up = { 0.0f, 0.0f, 1.0f };

    const float animProgress = std::min(currentElapsedTime / DEMO_TOTAL_TIME, 1.0f);
    sceneRenderer->advanceTime(animProgress);
    logoRenderer->advanceTime(currentElapsedTime);

    if (isBlobStaticEnded && renderSceneGeom)
        sceneRenderer->updateShadows(*blob);

    BeginDrawing();
    ClearBackground(BLACK);

#if !defined(__EMSCRIPTEN__) && !defined(PLATFORM_WEB)
    if (wireframeMode) rlEnableWireMode();
#endif

    if (isBlobStaticEnded)
    {
        BeginMode3D(camera);
        float intensity = driver->GetIntensity();

        if (renderSceneGeom)
            sceneRenderer->render(camera, *blob, true);
        
        blob->Render(camera, driver->GetPulseIntensity(), intensity, driver->GetBaseIntensity(), currentElapsedTime);
        
        if (renderSceneGeom) 
            greenFog->render(camera, *blob, *sceneRenderer, intensity, currentElapsedTime);
        
        if (renderSlash)
            logoRenderer->render(camController.getSlashTransform(), camera, currentElapsedTime);
            
        EndMode3D();
    }

#if !defined(__EMSCRIPTEN__) && !defined(PLATFORM_WEB)
    if (wireframeMode) rlDisableWireMode();
#endif

    EndDrawing();

    TakeScreenshot(TextFormat("frames/frame_%06d.tga", frameNumber++));
}