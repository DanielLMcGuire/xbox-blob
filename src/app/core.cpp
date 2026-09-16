#include "app.h"
#include "raylib.h"
#include "rlgl.h"

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
