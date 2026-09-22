#include "app.h"
#include "raylib.h"
#include "rlgl.h"

void XboxStartup::createShields()
{
#if !defined(__EMSCRIPTEN__) && !defined(PLATFORM_WEB)
    if (wireframeMode) rlDisableWireMode();
#endif

    const double bakeStart = GetTime();
    shields = new ShieldManager();
    shields->create(seed, *sceneRenderer, *blob);
    shieldBakeDebt += GetTime() - bakeStart;

#if !defined(__EMSCRIPTEN__) && !defined(PLATFORM_WEB)
    if (wireframeMode) rlEnableWireMode();
#endif

    float animPos = currentElapsedTime / DEMO_TOTAL_TIME;
    if (animPos > 1.0f) animPos = 1.0f;
    sceneRenderer->advanceTime(animPos);
}

void XboxStartup::renderShields(ShieldPass pass, float intensity)
{
    if (!shieldsEnabled || !shields) return;
    shields->render(camera, *blob, intensity, currentElapsedTime, pass);
}

void XboxStartup::update()
{
    if (WindowShouldClose())
    {
        running = false;
        return;
    }
    rlDisableBackfaceCulling();
    if (captureMode) updateCapture();
    else updateInteractive();
    rlEnableBackfaceCulling();
}