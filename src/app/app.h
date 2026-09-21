#pragma once

#include "raylib.h"
#include "raymath.h"
#include "../blob/blob.h"
#include "../blob/intensity_driver.h"
#include "../util/qrand.h"
#include "../util/noclip.h"
#include "../sos/sos_audio.h"

#include "../scene/scene_renderer.h"
#include "../scene/logo_renderer.h"
#include "../scene/green_fog.h"
#include "../scene/cam_control.h"
#include "../shields/shield_manager.h"

#include <algorithm>
#include <cmath>
#include <string>

#define TOGGLE(x) do { if (x) { x = false; } else { x = true; } } while(0)

class XboxStartup
{
public:
    XboxStartup(int argc, char** argv);
    ~XboxStartup();

    void update();
    inline bool isRunning() const { return running; }

private:
#if !defined(__EMSCRIPTEN__) && !defined(PLATFORM_WEB) && !defined(XBS_WIN32_DESKTOP)
    void parseArgs(int argc, char** argv);
#endif
    void updateInteractive();
    void updateCapture();
    void updateUI();
    void renderShields(ShieldPass pass, float intensity);

    bool showGui = false;
    bool captureMode = false;
    bool fullscreen = false;
    bool doAudio = true;
    int framerate = 0;
    bool drawFps = false;
#if defined(__EMSCRIPTEN__) || defined(PLATFORM_WEB) || defined(XBS_WIN32_DESKTOP)
    bool msaaEnabled = true;
#else
    bool msaaEnabled = false;
#endif
    bool gridEnabled = false;
    bool wireframeMode = false;
    bool shieldsEnabled = false;
    bool shieldsStartup = false;
    int32_t seed = defSeed;
    bool cursorEnabled = true;

    int screenWidth = 0;
    int screenHeight = 0;
    Camera3D camera;
    Camera3D homeCamera;

    Font font;
    float fontSpacing = 2.0f;
    
    Blob* blob = nullptr;
    BlobIntensityDriver* driver = nullptr;
    NoclipCamera* noclip = nullptr;
    SOS::Audio* audio = nullptr;
    IntroSceneRenderer* sceneRenderer = nullptr;
    LogoRenderer* logoRenderer = nullptr;
    GreenFog* greenFog = nullptr;
    ShieldManager* shields = nullptr;

    CameraController camController;
    int cameraPath = 0;
    bool renderSceneGeom = true;
    bool renderSlash = false;
    bool manualRender = false;
    bool isPaused = false;

    double lastClickTime;
    int frameNumber = 0;
    bool running = true;
    float currentElapsedTime;
};