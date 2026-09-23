#pragma once
#include "imgui.h"
#include <cstdarg>

namespace XboxUI
{

namespace XboxPalette
{
    static const ImVec4 Green        = ImVec4(139.f/255.f, 200.f/255.f,  24.f/255.f, 1.00f);
    static const ImVec4 GreenBright  = ImVec4(182.f/255.f, 245.f/255.f,  96.f/255.f, 1.00f);
    static const ImVec4 GreenNav     = ImVec4(190.f/255.f, 250.f/255.f,  94.f/255.f, 178.f/255.f);
    static const ImVec4 GreenYellow  = ImVec4(178.f/255.f, 208.f/255.f,   0.f/255.f, 1.00f);
    static const ImVec4 GreenMuted   = ImVec4(100.f/255.f, 200.f/255.f,  25.f/255.f, 1.00f);
    static const ImVec4 GreenDark    = ImVec4(  6.f/255.f,  33.f/255.f,   0.f/255.f, 1.00f);
    static const ImVec4 Wireframe    = ImVec4(125.f/255.f, 198.f/255.f,  34.f/255.f, 100.f/255.f);

    static const ImVec4 PanelBg      = ImVec4(  4.f/255.f,  20.f/255.f,   0.f/255.f, 1.00f);
    static const ImVec4 PanelBgLight = ImVec4( 14.f/255.f,  46.f/255.f,   7.f/255.f, 1.00f);
    static const ImVec4 PanelDarken  = ImVec4(  4.f/255.f,  50.f/255.f,   0.f/255.f, 1.00f);
    static const ImVec4 Black        = ImVec4(  0.f/255.f,   0.f/255.f,   0.f/255.f, 1.00f);
    static const ImVec4 Black80      = ImVec4(  0.f/255.f,   0.f/255.f,   0.f/255.f, 204.f/255.f);
    static const ImVec4 PanelSubtle  = ImVec4( 11.f/255.f,  18.f/255.f,  10.f/255.f, 1.00f);

    static const ImVec4 White        = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    static const ImVec4 Gray         = ImVec4(128.f/255.f, 128.f/255.f, 128.f/255.f, 1.00f);
    static const ImVec4 Red          = ImVec4(200.f/255.f,  30.f/255.f,  30.f/255.f, 1.00f);

    static const ImVec4 TitleText    = ImVec4(190.f/255.f, 250.f/255.f,  94.f/255.f, 178.f/255.f);
    static const ImVec4 ButtonText   = ImVec4(139.f/255.f, 200.f/255.f,  24.f/255.f, 1.00f);
    static const ImVec4 BodyText     = ImVec4(182.f/255.f, 245.f/255.f,  96.f/255.f, 1.00f);
}

inline void ApplyXboxDashboardTheme(ImGuiStyle* dst = nullptr)
{
    using namespace XboxPalette;
    ImGuiStyle& style = dst ? *dst : ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    colors[ImGuiCol_Text]                  = BodyText;
    colors[ImGuiCol_TextDisabled]          = Gray;
    colors[ImGuiCol_TextSelectedBg]        = ImVec4(Green.x, Green.y, Green.z, 0.35f);

    colors[ImGuiCol_WindowBg]              = PanelBg;
    colors[ImGuiCol_ChildBg]               = ImVec4(Black.x, Black.y, Black.z, 0.00f);
    colors[ImGuiCol_PopupBg]               = ImVec4(Black.x, Black.y, Black.z, 0.96f);
    colors[ImGuiCol_Border]                = Wireframe;
    colors[ImGuiCol_BorderShadow]          = ImVec4(0, 0, 0, 0);

    colors[ImGuiCol_FrameBg]               = PanelBgLight;
    colors[ImGuiCol_FrameBgHovered]        = ImVec4(Green.x, Green.y, Green.z, 0.35f);
    colors[ImGuiCol_FrameBgActive]         = ImVec4(Green.x, Green.y, Green.z, 0.55f);

    colors[ImGuiCol_TitleBg]               = PanelBg;
    colors[ImGuiCol_TitleBgActive]         = PanelDarken;
    colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(PanelBg.x, PanelBg.y, PanelBg.z, 0.75f);
    colors[ImGuiCol_MenuBarBg]             = PanelBg;

    colors[ImGuiCol_ScrollbarBg]           = Black;
    colors[ImGuiCol_ScrollbarGrab]         = ImVec4(Green.x, Green.y, Green.z, 0.40f);
    colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(Green.x, Green.y, Green.z, 0.70f);
    colors[ImGuiCol_ScrollbarGrabActive]   = Green;

    colors[ImGuiCol_CheckMark]             = Green;
    colors[ImGuiCol_SliderGrab]            = ImVec4(Green.x, Green.y, Green.z, 0.75f);
    colors[ImGuiCol_SliderGrabActive]      = GreenBright;

    colors[ImGuiCol_Button]                = PanelDarken;
    colors[ImGuiCol_ButtonHovered]         = ImVec4(Green.x, Green.y, Green.z, 0.55f);
    colors[ImGuiCol_ButtonActive]          = Green;

    colors[ImGuiCol_Header]                = ImVec4(Green.x, Green.y, Green.z, 0.30f);
    colors[ImGuiCol_HeaderHovered]         = ImVec4(Green.x, Green.y, Green.z, 0.55f);
    colors[ImGuiCol_HeaderActive]          = ImVec4(Green.x, Green.y, Green.z, 0.80f);

    colors[ImGuiCol_Separator]             = Wireframe;
    colors[ImGuiCol_SeparatorHovered]      = ImVec4(Green.x, Green.y, Green.z, 0.60f);
    colors[ImGuiCol_SeparatorActive]       = ImVec4(Green.x, Green.y, Green.z, 0.90f);

    colors[ImGuiCol_ResizeGrip]            = ImVec4(Green.x, Green.y, Green.z, 0.25f);
    colors[ImGuiCol_ResizeGripHovered]     = ImVec4(Green.x, Green.y, Green.z, 0.55f);
    colors[ImGuiCol_ResizeGripActive]      = ImVec4(Green.x, Green.y, Green.z, 0.85f);

    colors[ImGuiCol_Tab]                   = PanelBgLight;
    colors[ImGuiCol_TabHovered]            = ImVec4(Green.x, Green.y, Green.z, 0.55f);
    colors[ImGuiCol_TabActive]             = PanelDarken;
    colors[ImGuiCol_TabUnfocused]          = PanelBg;
    colors[ImGuiCol_TabUnfocusedActive]    = PanelBgLight;

    colors[ImGuiCol_PlotLines]             = GreenMuted;
    colors[ImGuiCol_PlotLinesHovered]      = GreenBright;
    colors[ImGuiCol_PlotHistogram]         = Green;
    colors[ImGuiCol_PlotHistogramHovered]  = GreenBright;

    colors[ImGuiCol_TableHeaderBg]         = PanelBgLight;
    colors[ImGuiCol_TableBorderStrong]     = ImVec4(Wireframe.x, Wireframe.y, Wireframe.z, 0.60f);
    colors[ImGuiCol_TableBorderLight]      = ImVec4(Wireframe.x, Wireframe.y, Wireframe.z, 0.25f);
    colors[ImGuiCol_TableRowBg]            = ImVec4(0, 0, 0, 0);
    colors[ImGuiCol_TableRowBgAlt]         = ImVec4(PanelSubtle.x, PanelSubtle.y, PanelSubtle.z, 0.50f);

    colors[ImGuiCol_DragDropTarget]        = ImVec4(GreenBright.x, GreenBright.y, GreenBright.z, 0.90f);
    colors[ImGuiCol_NavHighlight]          = Green;
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0f, 1.0f, 1.0f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg]     = Black80;
    colors[ImGuiCol_ModalWindowDimBg]      = Black80;

    style.WindowRounding    = 3.0f;
    style.ChildRounding     = 3.0f;
    style.FrameRounding     = 2.0f;
    style.PopupRounding     = 3.0f;
    style.ScrollbarRounding = 6.0f;
    style.GrabRounding      = 2.0f;
    style.TabRounding       = 2.0f;

    style.WindowBorderSize  = 1.0f;
    style.ChildBorderSize   = 1.0f;
    style.PopupBorderSize   = 1.0f;
    style.FrameBorderSize   = 1.0f;

    style.WindowPadding     = ImVec2(10, 10);
    style.FramePadding      = ImVec2(8, 4);
    style.ItemSpacing       = ImVec2(8, 6);
    style.ItemInnerSpacing  = ImVec2(6, 4);
    style.IndentSpacing     = 20.0f;
}

inline void XboxTitleText(const char* fmt, ...)
{
    ImGui::PushStyleColor(ImGuiCol_Text, XboxPalette::TitleText);
    va_list args;
    va_start(args, fmt);
    ImGui::TextV(fmt, args);
    va_end(args);
    ImGui::PopStyleColor();
}

inline bool XboxButton(const char* label, const ImVec2& size = ImVec2(0, 0))
{
    ImGui::PushStyleColor(ImGuiCol_Text, XboxPalette::ButtonText);
    bool pressed = ImGui::Button(label, size);
    ImGui::PopStyleColor();
    return pressed;
}

inline bool XboxBegin(const char* name, bool* p_open = nullptr, ImGuiWindowFlags flags = 0)
{
    ImGui::PushStyleColor(ImGuiCol_Text, XboxPalette::TitleText);
    bool visible = ImGui::Begin(name, p_open, flags);
    ImGui::PopStyleColor();
    return visible;
}

}