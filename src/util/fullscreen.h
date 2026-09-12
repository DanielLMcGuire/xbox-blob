#pragma once
#include "raylib.h"

inline static void ToggleExclusiveFullscreen(int windowedWidth, int windowedHeight)
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