#include "app.h"
#include "../util/fullscreen.h"
#include "../util/embed.h"
#include "../util/grid.h"
#include "../util/text_solvers.h"
#include "rlgl.h"

#include <algorithm>
#include <cmath>

#if defined(_WIN32)
    #undef DrawText
#endif

void XboxStartup::updateInteractive()
{
    const float dt = GetFrameTime();

#if !defined(__EMSCRIPTEN__) && !defined(PLATFORM_WEB)
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) 
    {
        const double now = GetTime();
        if (now - lastClickTime < DOUBLE_CLICK_TIME) 
        {
            Fullscreen::Toggle(screenWidth, screenHeight);
            lastClickTime = 0.0;
        } 
        else 
        {
            lastClickTime = now;
        }
    }
    
    if (IsKeyPressed(KEY_F11) || ((IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT)) && IsKeyPressed(KEY_ENTER))) 
        Fullscreen::Toggle(screenWidth, screenHeight);
    
    if (IsKeyPressed(KEY_F5)) TOGGLE(wireframeMode);
#endif

    if (IsKeyPressed(KEY_GRAVE)) TOGGLE(showGui);
    if (IsKeyPressed(KEY_F2)) TOGGLE(drawFps);
    if (IsKeyPressed(KEY_F9)) noclip->Toggle(camera, homeCamera, !manualRender);
    if (IsKeyPressed(KEY_F12)) TOGGLE(manualRender);
    if (IsKeyPressed(KEY_G)) TOGGLE(gridEnabled);
    if (IsKeyPressed(KEY_SPACE)) TOGGLE(isPaused);

    noclip->Update(camera, dt);

    if (!isPaused)
    {
        const float previousElapsedTime = driver->GetElapsedTime();
        driver->Advance(dt, *blob);
        currentElapsedTime = driver->GetElapsedTime();

        if (currentElapsedTime < previousElapsedTime)
        {
            if (audio) audio->restart();
            camController.pickPath(cameraPath);
            greenFog->restart(seed);
        }
    }

    const bool isBlobStaticEnded = (currentElapsedTime >= BLOB_STATIC_END_TIME);

    if (audio && isBlobStaticEnded && !isPaused)
        audio->init();

    Vector3 splinePos, splineLook;
    if (!manualRender)
    {
        float fCamTime = (currentElapsedTime * currentElapsedTime / 6.0f) * 0.8f;
        fCamTime += 0.2f * (currentElapsedTime * sinf(std::min(3.14159265354f / 2.0f, currentElapsedTime * 0.2833f)));

        camController.getPosition(fCamTime, &splinePos, &splineLook, &renderSceneGeom, &renderSlash);

        if (!noclip->active) {
            camera.position = splinePos;
            camera.target = splineLook;
            camera.up = { 0.0f, 0.0f, 1.0f };
        }
    }

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

    if (gridEnabled || isBlobStaticEnded) {
        BeginMode3D(camera);
        
        if (gridEnabled)
            Grid::Draw3D(10, 50, { 205, 255, 205, 255 });

        if (isBlobStaticEnded) {
            float intensity = driver->GetIntensity();
            float pulseIntensity = driver->GetPulseIntensity();
            float baseIntensity = driver->GetBaseIntensity();

            if (manualRender && IsGamepadAvailable(0))
            {
                float trigger = (GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_TRIGGER) + 1.0f) * 0.5f;
                trigger = std::clamp(trigger, 0.0f, 1.0f);
                intensity *= trigger;
                pulseIntensity *= trigger;
                baseIntensity *= trigger;
            }
            
            if (renderSceneGeom) 
                sceneRenderer->render(camera, *blob, true);

            blob->Render(camera, pulseIntensity, intensity, baseIntensity, currentElapsedTime);
            
            if (renderSceneGeom) 
                greenFog->render(camera, *blob, *sceneRenderer, intensity, currentElapsedTime);

            if (renderSlash)
                logoRenderer->render(camController.getSlashTransform(), camera, currentElapsedTime);
        }
        
        EndMode3D();
    }

#if !defined(__EMSCRIPTEN__) && !defined(PLATFORM_WEB)
    if (wireframeMode) rlDisableWireMode();
#endif

    if (drawFps)
        DrawTextEx(font, TextFormat("FPS: %i", GetFPS()), { 10, 10 }, font.baseSize, fontSpacing, XD_TEXT_FOREGROUND);

    if (noclip->active) {
        float yPos = TextSolve::solveBottomText(font, fontSpacing, 10);
        DrawTextEx(
            font,
            TextFormat("%.2f, %.2f, %.2f", camera.position.x, camera.position.y, camera.position.z),
            { 10, yPos },
            font.baseSize,
            fontSpacing,
            XD_TEXT_FOREGROUND
        );
    }
    
    if (showGui) updateUI();

    EndDrawing();
}