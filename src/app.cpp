#include "app.h"
#include "util/fullscreen.h"
#include "rlgl.h"

#include <cstdio>
#include <filesystem>
#include <iostream>

#if defined(_WIN32)
    #undef DrawText
#endif

#if defined(__cpp_pp_embed) && __cpp_pp_embed >= 202502L
#ifdef __has_embed
    #define HAS_EMBED 2
#else
    #define HAS_EMBED 1
#endif
#elif defined(__has_extension)
    #if __has_extension(c_embed)
        #ifdef __has_embed
            #define HAS_EMBED 2
        #else
            #define HAS_EMBED 1
        #endif
    #else
        #define HAS_EMBED 0
    #endif
#else
    #define HAS_EMBED 0
#endif

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

        InitWindow(screenWidth, screenHeight, "Xbox Startup | Rendering...");
        camera = { { 0.0f, distance, -6.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, fov, CAMERA_PERSPECTIVE };
        if (framerate == 0) framerate = 240; 
    }
    else
    {
        screenWidth = 960;
        screenHeight = 720;
        float distance = 35.0f;

        if (framerate == 0) SetConfigFlags(FLAG_VSYNC_HINT);
        InitWindow(screenWidth, screenHeight, "Xbox Startup");
        
        if (framerate > 0) SetTargetFPS(framerate);
        if (fullscreen) ToggleExclusiveFullscreen(screenWidth, screenHeight);

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

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc23-extensions"
    static constexpr unsigned char font_data[] = {
        #embed "../assets/xbox.ttf"
    };
#pragma clang diagnostic pop

    static constexpr int font_data_size = sizeof(font_data);
    font = LoadFontFromMemory(
        ".ttf",
        font_data,
        font_data_size,
        32,
        nullptr,
        0
    );

#endif
    fontSpacing = 2.0f;

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
        else if (arg == "--draw-fps") drawFps = true;
        else if (arg == "--help" || arg == "-h" || arg == "/?")
        {
            std::string program = std::filesystem::path(argv[0]).stem().string(); 
            std::puts(TextFormat("%s [OPTIONS...]\n\nOPTIONS:\n"
                        "  %s -c, --capture      Render a capture\n"
                        "  %s -fs, --fullscreen  Enter fullscreen on startup\n"
                        "  %s -na, --no-audio    Disable audio\n"
                        "  %s -f, --fps          Set framerate (VSYNC if not set)\n"
                        "  %s --draw-fps         Draw FPS to screen",
            program.c_str(), program.c_str(), program.c_str(), program.c_str(), program.c_str(), program.c_str()
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

    if (IsKeyPressed(KEY_F11) || ((IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT)) && IsKeyPressed(KEY_ENTER)))
        ToggleExclusiveFullscreen(screenWidth, screenHeight);

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        const double now = GetTime();

        if (now - lastClickTime < DOUBLE_CLICK_TIME)
        {
            ToggleExclusiveFullscreen(screenWidth, screenHeight);
            lastClickTime = 0.0;
        }
        else
        {
            lastClickTime = now;
        }
    }

    if (IsKeyPressed(KEY_F5)) noclip->Toggle(camera, homeCamera);

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

    if (currentElapsedTime >= BLOB_STATIC_END_TIME)
    {
        BeginMode3D(camera);
        blob->Render(camera, driver->GetPulseIntensity(), driver->GetIntensity(), driver->GetBaseIntensity(), currentElapsedTime);
        EndMode3D();
    }

    constexpr auto solveBottomText = [](Font font, float fontSpacing, float padding) -> float {
        return static_cast<float>(GetScreenHeight()) - (font.baseSize + fontSpacing + padding);
    };

    if (drawFps)
        DrawTextEx(
            font, 
            TextFormat("FPS: %i", GetFPS()), 
            Vector2{ 20, 20 }, 
            font.baseSize,
            fontSpacing,
            LIME
        );

    if (noclip->active)
        DrawTextEx(
            font,
            TextFormat("%.2f, %.2f, %.2f",
                camera.position.x,
                camera.position.y,
                camera.position.z),
            Vector2{ 10, solveBottomText(font, fontSpacing, 10) },
            font.baseSize,
            fontSpacing,
            LIME);

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

    if (driver->GetElapsedTime() >= BLOB_STATIC_END_TIME)
    {
        BeginMode3D(camera);
        blob->Render(camera, driver->GetPulseIntensity(), driver->GetIntensity(), driver->GetBaseIntensity(), driver->GetElapsedTime());
        EndMode3D();
    }

    EndDrawing();

    TakeScreenshot(TextFormat("frames/frame_%06d.tga", frameNumber++));
    
}