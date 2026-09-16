#include "app.h"
#include "rlgl.h"
#include "imgui.h"
#include "rlImGui.h"

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
    camController.getPosition(currentElapsedTime, &splinePos, &splineLook, &renderSceneGeom, &renderSlash);

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
        
        blob->Render(camera, driver->GetPulseIntensity(), driver->GetIntensity(), driver->GetBaseIntensity(), currentElapsedTime);

        if (renderSceneGeom) 
        {
            sceneRenderer->render(camera, *blob, true);
            //greenFog->render(camera, *blob, driver->GetIntensity(), currentElapsedTime);
        }
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