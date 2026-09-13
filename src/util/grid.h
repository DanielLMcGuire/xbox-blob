#pragma once
#include "raylib.h"
#include "rlgl.h"

static void DrawGrid3D(int slices, float spacing)
{
    int half = slices / 2;
    float extent = (float)half * spacing;

    rlBegin(RL_LINES);
        rlColor3f(1.0f, 1.0f, 1.0f);

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