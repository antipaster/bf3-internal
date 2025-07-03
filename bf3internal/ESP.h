#pragma once
#include "FB SDK/Frostbite.h"

namespace ESP {
    void RenderESP();
    void DrawPlayerESP(fb::ClientPlayer* player, fb::ClientSoldierEntity* soldier, fb::Vec3& screenPos, float distance);
    void DrawBox2D(float x, float y, float w, float h, fb::Color32 color);
    void DrawHealthBar(float x, float y, float w, float health, float maxHealth);
    void DrawText2D(float x, float y, fb::Color32 color, const char* text, float scale = 1.0f);
    bool WorldToScreen(fb::Vec3& worldPos, fb::Vec3& screenPos);
    void DrawOffscreenPointer(const fb::Vec3& screenCenter, const fb::Vec3& targetWorld, fb::Color32 color);
    void DrawPlayerName(fb::ClientPlayer* player, float x, float y, fb::Color32 color, float scale = 1.0f);
}

struct ESPSettings {
    float boxThickness;
    // Remove initialization here
    // bool showPlayerNames = true;
    // bool showOffscreenPointers = true;
};

extern bool showPlayerNames;
extern bool showOffscreenPointers; 