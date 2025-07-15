#pragma once
#include "FB SDK/Frostbite.h"
#include <imgui.h>

namespace ESP {
    void RenderESP();
    void RenderVehicleESP();
    void DrawPlayerESP(fb::ClientPlayer* player, fb::ClientSoldierEntity* soldier, fb::Vec3& screenPos, float distance);
    void DrawBox2D(float x, float y, float w, float h, fb::Color32 color);
    void DrawHealthBar(float x, float y, float w, float health, float maxHealth, fb::Color32 color);
    void DrawText2D(float x, float y, fb::Color32 color, const char* text, float scale = 1.0f);
    bool WorldToScreen(fb::Vec3& worldPos, fb::Vec3& screenPos);
    void DrawOffscreenPointer(const fb::Vec3& screenCenter, const fb::Vec3& targetWorld, fb::Color32 color);
    void DrawPlayerName(fb::ClientPlayer* player, float x, float y, fb::Color32 color, float scale = 1.0f);
}

struct ESPConfig {
    bool enabled = false;
    bool showBoxes = true;
    bool showHealthBars = true;
    bool showNames = true;
    bool showDistance = true;
    bool showHealthText = true;
    bool showTeammates = false;
    bool showEnemies = true;
    float maxDistance = 300.0f;
    float boxThickness = 2.0f;
    bool showPlayerNames = false;
    bool showOffscreenPointers = false;
    bool showWatermark = true;
    bool watermarkRight = true;
    ImVec4 boxColorEnemy = ImVec4(1.0f, 0.0f, 0.0f, 1.0f); // Red
    ImVec4 boxColorTeammate = ImVec4(0.0f, 1.0f, 0.0f, 1.0f); // Green
    ImVec4 boxColorOccluded = ImVec4(1.0f, 1.0f, 0.0f, 1.0f); // Yellow
    ImVec4 nameColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // White
    ImVec4 healthBarColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f); // Green
    ImVec4 offscreenPointerColor = ImVec4(1.0f, 0.0f, 1.0f, 1.0f); // tmobile ukraina (magenta)
};

struct VehicleESPConfig {
    bool enabled = false;
    bool showBoxes = true;
    bool showHealthBars = true;
    bool showNames = true;
    bool showDistance = true;
    bool showHealthText = true;
    float maxDistance = 400.0f;
    float boxThickness = 2.0f;
    ImVec4 boxColor = ImVec4(1.0f, 1.0f, 0.0f, 1.0f); // Yellow
    ImVec4 nameColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // White
    ImVec4 healthBarColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f); // Green
};

extern ESPConfig g_ESPConfig;
extern VehicleESPConfig g_VehicleESPConfig; 