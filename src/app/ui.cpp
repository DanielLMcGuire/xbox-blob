#include "app.h"
#include "../util/fullscreen.h"
#include "imgui.h"
#include "rlImGui.h"
#include "xd_ui_theme.h"

#if defined(_WIN32)
    #undef DrawText
#endif

void XboxStartup::updateUI()
{
    rlImGuiBegin();

    if (XboxBegin("Options", &showGui))
    {
        XboxTitleText("Visual");
        ImGui::Separator();

        ImGui::Checkbox("Show 3D Grid (G)", &gridEnabled);
        ImGui::Checkbox("Draw FPS Overlay (F2)", &drawFps);
#if !defined(__EMSCRIPTEN__) && !defined(PLATFORM_WEB)
        ImGui::Checkbox("Wireframe Mode (F5)", &wireframeMode);
        if (XboxButton("Toggle Fullscreen (F11 / Alt+Enter)"))
            Fullscreen::Toggle(screenWidth, screenHeight);
#endif
        ImGui::SliderInt("Camera Path", &cameraPath, -1, 3);

        ImGui::Checkbox("Manual Render (F12)", &manualRender);
        ImGui::Checkbox("Render Scene", &renderSceneGeom);
        ImGui::Checkbox("Render Logo", &renderSlash);
        if (shieldsStartup)
            ImGui::Checkbox("Render Shields", &shieldsEnabled);

        ImGui::Spacing();
        XboxTitleText("Time");
        ImGui::Separator();

        ImGui::Checkbox("Pause (Space)", &isPaused);

        if (ImGui::SliderFloat("Elapsed Time", &currentElapsedTime, 0.0f, DEMO_TOTAL_TIME, "%.2f s"))
        {
            driver->Seek(currentElapsedTime, *blob);
            if (audio) audio->seek(driver->GetElapsedTime() - BLOB_STATIC_END_TIME);
        }

        ImGui::Text("FPS: %d", GetFPS());
    } ImGui::End();

    rlImGuiEnd();
}