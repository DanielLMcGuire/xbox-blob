#include "app.h"

#include "raylib.h"
#include "imgui.h"

#include <filesystem>

Font XboxStartup::loadFont()
{
#ifdef HAS_EMBED
    #ifdef __clang__
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wc23-extensions"
    #endif

    static constexpr unsigned char font_data[] = {
        #embed "../../assets/xbox.ttf"
    };

    #ifdef __clang__
    #pragma clang diagnostic pop
    #endif

    const int font_data_size = sizeof(font_data);

    Font font = LoadFontFromMemory(
        ".ttf",
        font_data,
        font_data_size,
        32,
        nullptr,
        0
    );

    if (!IsFontValid(font))
        return GetFontDefault();

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

    return font;
#else
    constexpr const char* fontFile = "assets/xbox.ttf";

    if (!std::filesystem::exists(fontFile))
        return GetFontDefault();

    Font font = LoadFont(fontFile);

    if (!IsFontValid(font))
        return GetFontDefault();

    ImGuiIO& io = ImGui::GetIO();
    ImFont* customFont = io.Fonts->AddFontFromFileTTF(fontFile, 13.0f);
    io.FontDefault = customFont;

    return font;
#endif
}