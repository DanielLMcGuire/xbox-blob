#pragma once

#include "raylib.h"
#include "raymath.h"
#include "blob/blob.h"
#include "blob/intensity_driver.h"
#include "util/qrand.h"
#include "util/noclip.h"
#include "sos/sos_audio.h"

#include <algorithm>
#include <cmath>
#include <string>

class XboxStartup
{
public:
    XboxStartup(int argc, char** argv);
    ~XboxStartup();

    void update();
    inline bool isRunning() const { return running; }

private:
    void parseArgs(int argc, char** argv);
    void updateInteractive();
    void updateCapture();

    bool captureMode = false;
    bool fullscreen = false;
    bool doAudio = true;
    int framerate = 0;
    bool drawFps = false;
    bool msaaEnabled = false;
    bool gridEnabled = false;
    bool wireframeMode = false;
    int32_t seed = defSeed;

    int screenWidth = 960;
    int screenHeight = 720;
    Camera3D camera;
    Camera3D homeCamera;

    Font font;
    float fontSpacing = 2.0f;
    
    Blob* blob = nullptr;
    BlobIntensityDriver* driver = nullptr;
    NoclipCamera* noclip = nullptr;
    SOS::Audio* audio = nullptr;

    double lastClickTime;
    int frameNumber = 0;
    bool running = true;
};