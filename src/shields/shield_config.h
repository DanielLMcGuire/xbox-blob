#pragma once

#include "../defines.h"

namespace ShieldConfig
{
constexpr float FADE_IN_START_TIME = BLOB_STATIC_END_TIME;
constexpr float FADE_IN_DELTA = 1.2f;
constexpr float FADE_OUT_START_TIME = FINISH_START_TIME - 0.1f;
constexpr float FADE_OUT_DELTA = FINISH_TRANSITION_TIME * 0.2f;

constexpr float PUSHOUT_START_TIME = 0.5f;
constexpr float PUSHOUT_DELTA = 2.7f;
constexpr float START_PUSHOUT_RADIUS = 0.0f;

constexpr float MAX_SHADING = 0.75f;

constexpr int SOLID_SHIELD_COUNT = 3;
constexpr int BAND_SHIELD_COUNT = 5;

constexpr int GRID_WIDTH = 8;
constexpr int GRID_HEIGHT = 6;

constexpr float CAP_INSIDE_RADIUS = 13.1f;
constexpr float CAP_OUTSIDE_RADIUS = 14.0f;
constexpr float CAP_HORIZ_DIM = 1.2f;
constexpr float CAP_VERT_DIM = 0.9f;
constexpr float CAP_MID_RADIUS = 0.5f * (CAP_INSIDE_RADIUS + CAP_OUTSIDE_RADIUS);
constexpr float CAP_RADIUS_SCALE = 1.0f - 1.2f * (CAP_OUTSIDE_RADIUS - CAP_INSIDE_RADIUS) / CAP_OUTSIDE_RADIUS;

constexpr float BAND_THICKNESS = 0.5f;
constexpr float BAND_HORIZ_RADIANS = 1.2f;
constexpr float BAND_MIN_LATITUDE = -0.45f * PI;
constexpr float BAND_MAX_LATITUDE = +0.45f * PI;
constexpr float BAND_OUTSIDE_RADIUS =
    CAP_MID_RADIUS * CAP_RADIUS_SCALE * CAP_RADIUS_SCALE * CAP_RADIUS_SCALE;

constexpr float SOLID_ROTATION_RATE = 2.0f;
constexpr float SPEED_ACCEL = 0.8f;
constexpr float RADIAL_OFFSET = 2.0f;

constexpr Vector3 BLOB_SPEC_COLOR = {0.4f, 1.0f, 0.3f};
constexpr float BLOB_INTENSITY_SCALE = 2.0f;
constexpr Vector3 MOOD_LIGHT_POSITION = {0.0f, -40.0f, 30.0f};
} // namespace ShieldConfig
