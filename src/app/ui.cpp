#include "app.h"
#include "../util/fullscreen.h"
#include "imgui.h"
#include "rlImGui.h"

#if defined(_WIN32)
    #undef DrawText
#endif

void XboxStartup::updateUI()
{
    rlImGuiBegin();

    if (ImGui::Begin("Options", &showGui))
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
        ImGui::SliderInt("Camera Path", &cameraPath, -1, 3);

        ImGui::Checkbox("Manual Render (F12)", &manualRender);
        ImGui::Checkbox("Render Scene", &renderSceneGeom);
        ImGui::Checkbox("Render Logo", &renderSlash);

        ImGui::Spacing();
        ImGui::TextUnformatted("Time");
        ImGui::Separator();

        ImGui::Checkbox("Pause (Space)", &isPaused);

        if (ImGui::SliderFloat("Elapsed Time", &currentElapsedTime, 0.0f, DEMO_TOTAL_TIME, "%.2f s"))
        {
            driver->ScrubTo(currentElapsedTime, *blob);
        }

        ImGui::Text("FPS: %d", GetFPS());
    } ImGui::End();

    rlImGuiEnd();
}