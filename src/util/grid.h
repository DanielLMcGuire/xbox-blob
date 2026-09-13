#pragma once
#include "raylib.h"
#include "rlgl.h"

#define rlColor(color) rlColor4f( \
        color.r / 255.0f, \
        color.g / 255.0f, \
        color.b / 255.0f, \
        color.a / 255.0f \
    )

namespace Grid
{

inline static void Draw3D(int slices, float spacing, Color color)
{
    int half = slices / 2;
    float extent = (float)half * spacing;

    rlBegin(RL_LINES);
        rlColor(color);

        for (int i = -half; i <= half; i++)
        {
            for (int j = -half; j <= half; j++)
            {
                float a = (float)i * spacing;
                float b = (float)j * spacing;

                rlVertex3f(-extent, a, b);
                rlVertex3f( extent, a, b);

                rlVertex3f(a, -extent, b);
                rlVertex3f(a,  extent, b);

                rlVertex3f(a, b, -extent);
                rlVertex3f(a, b,  extent);
            }
        }
    rlEnd();
}

}