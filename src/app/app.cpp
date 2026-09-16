#include "app.h"
#include "../util/fullscreen.h"
#include "../util/embed.h"
#include "imgui.h"
#include "rlImGui.h"

#include <cstdio>
#include <filesystem>
#include <iostream>
#include <cinttypes>

#if defined(_WIN32)
    #undef DrawText
#endif

XboxStartup::XboxStartup(int argc, char** argv)
{
#if !defined(__EMSCRIPTEN__) && !defined(PLATFORM_WEB)
    parseArgs(argc, argv);
#endif
    constexpr float fov = 45.0f;

    if (captureMode)
    {
        if (screenWidth == 0) screenWidth = 1280;
        if (screenHeight == 0) screenHeight = 720;
        float distance = 45.0f;

        std::filesystem::create_directories("frames");
        if (msaaEnabled)
            SetConfigFlags(FLAG_MSAA_4X_HINT);
        InitWindow(screenWidth, screenHeight, "Xbox Startup | Rendering...");
        int monitor = GetCurrentMonitor();
        int width = GetMonitorWidth(monitor);
        int height = GetMonitorHeight(monitor);
        if (width >= screenWidth || height >= screenHeight || fullscreen)
            Fullscreen::Toggle(width, height);
        camera = { { 0.0f, distance, -6.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, fov, CAMERA_PERSPECTIVE };
        if (framerate == 0) framerate = 240; 
    }
    else
    {
        if (screenWidth == 0) screenWidth = 640;
        if (screenHeight == 0) screenHeight = 480;
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
    logoRenderer = new LogoRenderer();
    logoRenderer->create();
    greenFog = new GreenFog();
    greenFog->create(seed);
    camController.init();
    camController.pickPath(cameraPath);

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

    ImGuiIO& io = ImGui::GetIO();
    ImFontConfig font_cfg;
    font_cfg.FontDataOwnedByAtlas = false;
    ImFont* customFont = io.Fonts->AddFontFromMemoryTTF(
        (void*)font_data, 
        font_data_size, 
        13.0f, 
        &font_cfg
    );
    io.FontDefault = customFont;
#else
    font = LoadFont("../assets/xbox.ttf");
    ImGuiIO& io = ImGui::GetIO();
    ImFontConfig font_cfg;
    ImFont* customFont = io.Fonts->AddFontFromFileTTF("../assets/xbox.ttf", 13.0f);
    io.FontDefault = customFont;
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
    if (sceneRenderer)
    {
        sceneRenderer->unload();
        delete sceneRenderer;
    }
    if (logoRenderer)
    {
        logoRenderer->unload();
        delete logoRenderer;
    }
    if (greenFog) {
        greenFog->unload();
        delete greenFog;
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
            if (i + 1 < argc) cameraPath = std::clamp(std::stoi(argv[++i]), -1, 3);
        }
        else if (arg == "-path" || arg == "--camera-path") {
            if (i + 1 < argc) cameraPath = std::stoi(argv[++i]);
        }
        else if (arg == "-df" || arg == "--draw-fps") drawFps = true;
        else if (arg == "-m" || arg == "--msaa") msaaEnabled = true;
        else if (arg == "-g" || arg == "--grid") gridEnabled = true;
        else if (arg == "-w" || arg == "--wireframe") wireframeMode = true;
        else if (arg == "-s" || arg == "--seed") {
            if (i + 1 < argc) seed = static_cast<int32_t>(std::stoul(argv[++i], nullptr, 16));
        }
        else if (arg == "-x") {
            if (i + 1 < argc) screenWidth = std::stoi(argv[++i]);
        }
        else if (arg == "-y") {
            if (i + 1 < argc) screenHeight = std::stoi(argv[++i]);
        }
        else if (arg == "--help" || arg == "-h" || arg == "/?")
        {
            std::string program = std::filesystem::path(argv[0]).stem().string(); 
            std::puts(TextFormat("%s [OPTIONS...]\n\nOPTIONS:\n"
                        "  %s -c, --capture      Render a capture\n"
                        "  %s -path,--camera-path      Choose camera path (0-3, -1=random,0=stock)\n"
                        "  %s -fs, --fullscreen  Enter fullscreen on startup\n"
                        "  %s -na, --no-audio    Disable audio\n"
                        "  %s -f, --fps          Set framerate (VSYNC if not set)\n"
                        "  %s -df, --draw-fps    Draw FPS to screen\n"
                        "  %s -m, --msaa         Enable MSAA (Antialiasing)\n"
                        "  %s -g, --grid         Show 3D grid\n"
                        "  %s -w, --wireframe    Enable wireframe mode on startup\n"
                        "  %s -s, --seed         Set the RNG seed (hex, e.g. %#08" PRIx32 ")\n\n"
                        "KEYBINDS:\n"
                        "  `                     Toggle UI\n"
                        "  F2                    Toggle FPS overlay\n"
                        "  F5                    Toggle wireframe\n"
                        "  F9                    Toggle freecam\n"
                        "  F11, ALT+ENTER        Toggle fullscreen\n"
                        "  F12                   Toggle manual rendering\n"
                        "  G                     Toggle grid\n",
            program.c_str(), program.c_str(), program.c_str(), program.c_str(), 
            program.c_str(), program.c_str(), program.c_str(), program.c_str(), 
            program.c_str(), program.c_str(), program.c_str(), defSeed
            ));
            std::exit(0);
        }
    }
}
#endif

