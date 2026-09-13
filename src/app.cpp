#include "app.h"
#include "util/fullscreen.h"
#include "util/embed.h"
#include "util/grid.h"
#include "util/text_solvers.h"
#include "rlgl.h"

#include <cstdio>
#include <filesystem>
#include <iostream>

#if defined(_WIN32)
    #undef DrawText
#endif

#define TOGGLE(x) do { if (x) { x = false; } else { x = true; } } while(0)

XboxStartup::XboxStartup(int argc, char** argv)
{
    parseArgs(argc, argv);
    constexpr float fov = 45.0f;

    if (captureMode)
    {
        screenWidth = 1000;
        screenHeight = 1000;
        float distance = 45.0f;

        std::filesystem::create_directories("frames");
        if (msaaEnabled)
            SetConfigFlags(FLAG_MSAA_4X_HINT);
        InitWindow(screenWidth, screenHeight, "Xbox Startup | Rendering...");
        camera = { { 0.0f, distance, -6.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, fov, CAMERA_PERSPECTIVE };
        if (framerate == 0) framerate = 240; 
    }
    else
    {
        screenWidth = 960;
        screenHeight = 720;
        float distance = 35.0f;

        unsigned int cfg = FLAG_WINDOW_RESIZABLE;
        if (framerate == 0) cfg |= FLAG_VSYNC_HINT;
        if (msaaEnabled) cfg |= FLAG_MSAA_4X_HINT;
        SetConfigFlags(cfg);
        InitWindow(screenWidth, screenHeight, "Xbox Startup");
        
        if (framerate > 0) SetTargetFPS(framerate);
        if (fullscreen) Fullscreen::Toggle(screenWidth, screenHeight);

        camera = { { 0.0f, distance, -6.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, fov, CAMERA_PERSPECTIVE };
    }

    homeCamera = camera;
    lastClickTime = -DOUBLE_CLICK_TIME;

    blob = new Blob();
    driver = new BlobIntensityDriver();
    noclip = new NoclipCamera();

#ifdef HAS_EMBED

    #if HAS_EMBED == 2
        #if __has_embed("../assets/xbox.ttf")
        #else
            #error FAILED TO FIND FONT!
        #endif
    #endif

    #ifdef __clang__
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wc23-extensions"
    #endif
    static constexpr unsigned char font_data[] = {
        #embed "../assets/xbox.ttf"
    };
    #ifdef __clang__
    #pragma clang diagnostic pop
    #endif

    static constexpr int font_data_size = sizeof(font_data);
    font = LoadFontFromMemory(
        ".ttf",
        font_data,
        font_data_size,
        32,
        nullptr,
        0
    );
#else
    font = LoadFont("../assets/xbox.ttf");
#endif
    if (!IsFontValid(font)) font = GetFontDefault();

    if (captureMode) driver->loop = false;
    if (doAudio)
    {
        if (!captureMode) InitAudioDevice();
        audio = new SOS::Audio(false);
    }
}

XboxStartup::~XboxStartup()
{
    if (captureMode && audio && driver)
    {
        if (!audio->exportWav("frames/audio.wav", driver->GetElapsedTime()))
        {
            std::fputs("Failed to export WAV\n", stderr);
        }
    }

    delete blob;
    delete driver;
    delete noclip;
    delete audio;

    if (doAudio && !captureMode) CloseAudioDevice();

    CloseWindow();
}

void XboxStartup::parseArgs(int argc, char** argv)
{
    for (int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];
        if (arg == "-c" || arg == "--capture") captureMode = true;
        else if (arg == "-fs" || arg == "--fullscreen") fullscreen = true;
        else if (arg == "-na" || arg == "--no-audio") doAudio = false;
        else if (arg == "-f" || arg == "--fps") {
            if (i + 1 < argc) framerate = std::stoi(argv[++i]);
        }
        else if (arg == "-df" || arg == "--draw-fps") drawFps = true;
        else if (arg == "-m" || arg == "--msaa") msaaEnabled = true;
        else if (arg == "-g" || arg == "--grid") gridEnabled = true;
        else if (arg == "-w" || arg == "--wireframe") wireframeMode = true;
        else if (arg == "--help" || arg == "-h" || arg == "/?")
        {
            std::string program = std::filesystem::path(argv[0]).stem().string(); 
            std::puts(TextFormat("%s [OPTIONS...]\n\nOPTIONS:\n"
                        "  %s -c, --capture      Render a capture\n"
                        "  %s -fs, --fullscreen  Enter fullscreen on startup\n"
                        "  %s -na, --no-audio    Disable audio\n"
                        "  %s -f, --fps          Set framerate (VSYNC if not set)\n"
                        "  %s -df, --draw-fps    Draw FPS to screen\n"
                        "  %s -m, --msaa         Enable MSAA (Antialiasing)\n"
                        "  %s -g, --grid         Show 3D grid\n"
                        "  %s -w, --wireframe    Enable wireframe mode on startup",
            program.c_str(), program.c_str(), program.c_str(), program.c_str(), program.c_str(), program.c_str(), program.c_str(), program.c_str(), program.c_str()
            ));
            std::exit(0);
        }
    }
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

void XboxStartup::updateInteractive()
{
    const float dt = GetFrameTime();

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        const double now = GetTime();

        if (now - lastClickTime < DOUBLE_CLICK_TIME)
        {
            Fullscreen::Toggle(screenWidth, screenHeight);
            lastClickTime = 0.0;
        }
        else
        {
            lastClickTime = now;
        }
    }

    if (IsKeyPressed(KEY_F11) || ((IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT)) && IsKeyPressed(KEY_ENTER)))
        Fullscreen::Toggle(screenWidth, screenHeight);

    if (IsKeyPressed(KEY_F5)) noclip->Toggle(camera, homeCamera);

    if (IsKeyPressed(KEY_F2)) TOGGLE(drawFps);

    if (IsKeyPressed(KEY_F6)) TOGGLE(wireframeMode);

    if (IsKeyPressed(KEY_F7)) TOGGLE(gridEnabled);

    noclip->Update(camera, dt);

    const float previousElapsedTime = driver->GetElapsedTime();
    driver->Advance(dt, *blob);
    const float currentElapsedTime = driver->GetElapsedTime();

    if (audio)
    {
        if (currentElapsedTime < previousElapsedTime) audio->restart();
        if (currentElapsedTime >= BLOB_STATIC_END_TIME) audio->init();
    }

    BeginDrawing();
    ClearBackground(BLACK);

    if (wireframeMode)
        rlEnableWireMode(); 

    if (gridEnabled) {
        BeginMode3D(camera);
        Grid::Draw3D(10, 50, { 255, 255, 255, 255 });
        EndMode3D();
    }

    if (currentElapsedTime >= BLOB_STATIC_END_TIME)
    {
        BeginMode3D(camera);
        blob->Render(camera, driver->GetPulseIntensity(), driver->GetIntensity(), driver->GetBaseIntensity(), currentElapsedTime);
        EndMode3D();
    }

    if (wireframeMode)
        rlDisableWireMode();

    if (drawFps)
        DrawTextEx(
            font, 
            TextFormat("FPS: %i", GetFPS()), 
            Vector2{ 10, 10 }, 
            font.baseSize,
            fontSpacing,
            XD_TEXT_FOREGROUND
        );

    if (noclip->active)
        DrawTextEx(
            font,
            TextFormat("%.2f, %.2f, %.2f",
                camera.position.x,
                camera.position.y,
                camera.position.z),
            Vector2{ 10, TextSolve::solveBottomText(font, fontSpacing, 10) },
            font.baseSize,
            fontSpacing,
            XD_TEXT_FOREGROUND);

    EndDrawing();
}

void XboxStartup::updateCapture()
{
    const float fixedDt = 1.0f / static_cast<float>(framerate);

    if (!driver->Advance(fixedDt, *blob))
    {
        running = false; 
        return;
    }

    BeginDrawing();
    ClearBackground(BLACK);

    if (wireframeMode)
        rlEnableWireMode();

    if (driver->GetElapsedTime() >= BLOB_STATIC_END_TIME)
    {
        BeginMode3D(camera);
        blob->Render(camera, driver->GetPulseIntensity(), driver->GetIntensity(), driver->GetBaseIntensity(), driver->GetElapsedTime());
        EndMode3D();
    }

    if (wireframeMode)
        rlDisableWireMode();

    EndDrawing();

    TakeScreenshot(TextFormat("frames/frame_%06d.tga", frameNumber++));
    
}