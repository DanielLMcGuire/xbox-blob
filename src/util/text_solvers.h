#pragma once

#include "raylib.h"

namespace TextSolve
{

inline float solveBottomText(Font font, float fontSpacing, float padding) {
    return static_cast<float>(GetScreenHeight()) - (font.baseSize + fontSpacing + padding);
}

inline float solveRightText(Font font, const char* text, float fontSpacing, float padding) {
    Vector2 textSize = MeasureTextEx(font, text, font.baseSize, fontSpacing);
    return static_cast<float>(GetScreenWidth()) - (textSize.x + padding);
}

}