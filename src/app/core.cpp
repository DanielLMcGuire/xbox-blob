#include "app.h"
#include "raylib.h"
#include "rlgl.h"

void XboxStartup::renderShields(ShieldPass pass, float intensity)
{
    if (!shieldsEnabled) return;

    if (!shields)
    {
        shields = new ShieldManager();
        shields->create(seed);
    }

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
