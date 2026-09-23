#pragma once

#include "raylib.h"

namespace Fullscreen
{

inline static void Toggle(int windowedWidth, int windowedHeight)
{
    if (!IsWindowFullscreen())
    {
        const int display = GetCurrentMonitor();
        SetWindowSize(GetMonitorWidth(display), GetMonitorHeight(display));
    }

    ToggleFullscreen();

    if (!IsWindowFullscreen())
        SetWindowSize(windowedWidth, windowedHeight);
}

}