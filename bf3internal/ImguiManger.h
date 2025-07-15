#include "imgui.h"

class ImGuiManager {
public:
    static void darktheme(float scale = 1.0f) {
        ImGuiStyle& style = ImGui::GetStyle();
        ImVec4* colors = style.Colors;

        // bg
        colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.07f, 1.00f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.05f, 0.05f, 0.06f, 1.00f);
        colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.09f, 0.94f);

        // bord
        colors[ImGuiCol_Border] = ImVec4(0.18f, 0.18f, 0.2f, 0.4f);
        colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

        // text
        colors[ImGuiCol_Text] = ImVec4(0.93f, 0.94f, 0.96f, 1.00f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.45f, 0.45f, 0.50f, 1.00f);

        // buttons
        colors[ImGuiCol_Button] = ImVec4(0.15f, 0.15f, 0.17f, 1.00f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.20f, 0.20f, 0.23f, 1.00f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.25f, 0.25f, 0.30f, 1.00f);

        // headers
        colors[ImGuiCol_Header] = ImVec4(0.16f, 0.16f, 0.20f, 1.00f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.26f, 0.30f, 1.00f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.20f, 0.20f, 0.25f, 1.00f);

        // frame
        colors[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.15f, 0.15f, 0.18f, 1.00f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.20f, 0.20f, 0.23f, 1.00f);

        // tab
        colors[ImGuiCol_Tab] = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);
        colors[ImGuiCol_TabHovered] = ImVec4(0.25f, 0.25f, 0.30f, 1.00f);
        colors[ImGuiCol_TabActive] = ImVec4(0.18f, 0.18f, 0.22f, 1.00f);

        // sl , cb
        colors[ImGuiCol_SliderGrab] = ImVec4(0.60f, 0.60f, 0.70f, 1.00f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.80f, 0.80f, 0.90f, 1.00f);
        colors[ImGuiCol_CheckMark] = ImVec4(0.95f, 0.95f, 1.00f, 1.00f);

        // title
        colors[ImGuiCol_TitleBg] = ImVec4(0.08f, 0.08f, 0.09f, 1.00f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.13f, 0.13f, 0.14f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.04f, 0.04f, 0.05f, 1.00f);

        // grips
        colors[ImGuiCol_ResizeGrip] = ImVec4(0.44f, 0.44f, 0.5f, 0.2f);
        colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.56f, 0.56f, 0.6f, 0.4f);
        colors[ImGuiCol_ResizeGripActive] = ImVec4(0.66f, 0.66f, 0.7f, 0.6f);

        // scroll
        colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.03f, 1.00f);
        colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.25f, 0.25f, 0.30f, 0.7f);
        colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.3f, 0.3f, 0.35f, 0.8f);
        colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.4f, 0.4f, 0.45f, 0.9f);

        // layout
        style.WindowRounding = 8.0f * scale;
        style.FrameRounding = 6.0f * scale;
        style.GrabRounding = 6.0f * scale;
        style.ScrollbarRounding = 6.0f * scale;
        style.FramePadding = ImVec2(8, 5);
        style.ItemSpacing = ImVec2(10, 8);
        style.WindowPadding = ImVec2(15, 12);
    }
};
