#include "app.h"
#include "util/fullscreen.h"
#include "util/embed.h"
#include "util/grid.h"
#include "util/text_solvers.h"
#include "rlgl.h"
#include "imgui.h"
#include "rlImGui.h"

#include <cstdio>
#include <filesystem>
#include <iostream>
#include <cinttypes>

#if defined(_WIN32)
    #undef DrawText
#endif

#define TOGGLE(x) do { if (x) { x = false; } else { x = true; } } while(0)

XboxStartup::XboxStartup(int argc, char** argv)
{
#if !defined(__EMSCRIPTEN__) && !defined(PLATFORM_WEB)
    parseArgs(argc, argv);
#endif
    constexpr float fov = 45.0f;

    camController.init();
    camController.pickPath(0);

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

    rlImGuiSetup(true);

    homeCamera = camera;
    lastClickTime = -DOUBLE_CLICK_TIME;

    BlobSetRandomSeed(seed);

    blob = new Blob();
    driver = new BlobIntensityDriver(seed);
    noclip = new NoclipCamera();
    sceneRenderer = new IntroSceneRenderer();
    sceneRenderer->create(); 

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
    if (audio)
        delete blob;
    if (driver)
        delete driver;
    if (noclip)
        delete noclip;
    if (audio)
        delete audio;
    if (sceneRenderer) {
        sceneRenderer->unload();
        delete sceneRenderer;
    }

    if (doAudio && !captureMode) CloseAudioDevice();

    rlImGuiShutdown();
    CloseWindow();
}

#if !defined(__EMSCRIPTEN__) && !defined(PLATFORM_WEB)
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
        else if (arg == "-s" || arg == "--seed") {
            if (i + 1 < argc) seed = static_cast<int32_t>(std::stoul(argv[++i], nullptr, 16));
        }
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
                        "  %s -w, --wireframe    Enable wireframe mode on startup\n"
                        "  %s -s, --seed         Set the RNG seed (hex, e.g. %#08" PRIx32 ")",
            program.c_str(), program.c_str(), program.c_str(), program.c_str(), 
            program.c_str(), program.c_str(), program.c_str(), program.c_str(), 
            program.c_str(), program.c_str(), defSeed
            ));
            std::exit(0);
        }
    }
}
#endif

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

void XboxStartup::updateUI()
{
    rlImGuiBegin();

    if (ImGui::Begin("Options"))
    {
        ImGui::TextUnformatted("Visual");
        ImGui::Separator();

        ImGui::Checkbox("Show 3D Grid (G)", &gridEnabled);
        ImGui::Checkbox("Draw FPS Overlay (F2)", &drawFps);
#if !defined(__EMSCRIPTEN__) && !defined(PLATFORM_WEB)
        ImGui::Checkbox("Wireframe Mode (F5)", &wireframeMode);
        if (ImGui::Button("Toggle Fullscreen (F11 / Alt+Enter)"))
            Fullscreen::Toggle(screenWidth, screenHeight);
#endif

        ImGui::Spacing();
        ImGui::TextUnformatted("Stats");
        ImGui::Separator();

        ImGui::Text("FPS: %d", GetFPS());
        ImGui::Text("Elapsed Time: %.2f s", currentElapsedTime);
    } ImGui::End();

    rlImGuiEnd();
}

void XboxStartup::updateInteractive()
{
    const float dt = GetFrameTime();

#if !defined(__EMSCRIPTEN__) && !defined(PLATFORM_WEB)
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
    
    if (IsKeyPressed(KEY_F5)) TOGGLE(wireframeMode);
#endif

    if (IsKeyPressed(KEY_GRAVE)) TOGGLE(showGui);
    if (IsKeyPressed(KEY_F2)) TOGGLE(drawFps);
    if (IsKeyPressed(KEY_F9)) noclip->Toggle(camera, homeCamera);
    if (IsKeyPressed(KEY_G)) TOGGLE(gridEnabled);


    noclip->Update(camera, dt);

    const float previousElapsedTime = driver->GetElapsedTime();
    driver->Advance(dt, *blob);
    currentElapsedTime = driver->GetElapsedTime();

    if (currentElapsedTime < previousElapsedTime)
    {
        if (audio) audio->restart();
        camController.pickPath(0);
    }

    const bool isBlobStaticEnded = (currentElapsedTime >= BLOB_STATIC_END_TIME);

    if (audio && isBlobStaticEnded)
        audio->init();

    Vector3 splinePos, splineLook;
    camController.getPosition(currentElapsedTime, &splinePos, &splineLook, &renderSceneGeom, &renderSlash);

    if (!noclip->active) {
        camera.position = splinePos;
        camera.target = splineLook;
        camera.up = { 0.0f, 0.0f, 1.0f };
    }

    const float animProgress = std::min(currentElapsedTime / DEMO_TOTAL_TIME, 1.0f);
    sceneRenderer->advanceTime(animProgress);

    if (isBlobStaticEnded && renderSceneGeom)
        sceneRenderer->updateShadows(*blob);

    BeginDrawing();
    ClearBackground(BLACK);

#if !defined(__EMSCRIPTEN__) && !defined(PLATFORM_WEB)
    if (wireframeMode) rlEnableWireMode(); 
#endif

    if (gridEnabled || isBlobStaticEnded) {
        BeginMode3D(camera);
        
        if (gridEnabled)
            Grid::Draw3D(10, 50, { 255, 255, 255, 255 });

        if (isBlobStaticEnded) {
            blob->Render(camera, driver->GetPulseIntensity(), driver->GetIntensity(), driver->GetBaseIntensity(), currentElapsedTime);
            if (renderSceneGeom)
                sceneRenderer->render(camera, *blob, true);
        }
        
        EndMode3D();
    }

#if !defined(__EMSCRIPTEN__) && !defined(PLATFORM_WEB)
    if (wireframeMode) rlDisableWireMode();
#endif

    if (drawFps)
        DrawTextEx(font, TextFormat("FPS: %i", GetFPS()), { 10, 10 }, font.baseSize, fontSpacing, XD_TEXT_FOREGROUND);

    if (noclip->active) {
        float yPos = TextSolve::solveBottomText(font, fontSpacing, 10);
        DrawTextEx(
            font,
            TextFormat("%.2f, %.2f, %.2f", camera.position.x, camera.position.y, camera.position.z),
            { 10, yPos },
            font.baseSize,
            fontSpacing,
            XD_TEXT_FOREGROUND
        );
    }
    
    if (showGui) updateUI();

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

    currentElapsedTime = driver->GetElapsedTime();
    const bool isBlobStaticEnded = (currentElapsedTime >= BLOB_STATIC_END_TIME);

    Vector3 splinePos, splineLook;
    camController.getPosition(currentElapsedTime, &splinePos, &splineLook, &renderSceneGeom, &renderSlash);

    camera.position = splinePos;
    camera.target = splineLook;
    camera.up = { 0.0f, 0.0f, 1.0f };

    const float animProgress = std::min(currentElapsedTime / DEMO_TOTAL_TIME, 1.0f);
    sceneRenderer->advanceTime(animProgress);

    if (isBlobStaticEnded && renderSceneGeom)
        sceneRenderer->updateShadows(*blob);

    BeginDrawing();
    ClearBackground(BLACK);

#if !defined(__EMSCRIPTEN__) && !defined(PLATFORM_WEB)
    if (wireframeMode) rlEnableWireMode();
#endif

    if (isBlobStaticEnded)
    {
        BeginMode3D(camera);
        
        blob->Render(camera, driver->GetPulseIntensity(), driver->GetIntensity(), driver->GetBaseIntensity(), currentElapsedTime);

        if (renderSceneGeom)
            sceneRenderer->render(camera, *blob, true);
        
        EndMode3D();
    }
#if !defined(__EMSCRIPTEN__) && !defined(PLATFORM_WEB)
    if (wireframeMode) rlDisableWireMode();
#endif
    EndDrawing();

    TakeScreenshot(TextFormat("frames/frame_%06d.tga", frameNumber++));
}