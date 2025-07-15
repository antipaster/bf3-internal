#include "ESP.h"
#include "FB SDK/Frostbite.h"
#include "Functions.h"
#include <cstdio>
#include <imgui.h>
#include <cmath>
#include <set>

ESPConfig g_ESPConfig;
VehicleESPConfig g_VehicleESPConfig;


static inline fb::Color32 ImVec4ToColor32(const ImVec4& v) {
    return fb::Color32((int)(v.x * 255.0f), (int)(v.y * 255.0f), (int)(v.z * 255.0f), (int)(v.w * 255.0f));
}

bool showPlayerNames = true;
bool showOffscreenPointers = true;

namespace ESP {

    // fucked something here
    void RenderVehicleESP() {
        if (!g_VehicleESPConfig.enabled) return;
        fb::ClientGameContext* g_pGameContext = fb::ClientGameContext::Singleton();
        if (!g_pGameContext) return;
        fb::ClientPlayerManager* pPlayerManager = g_pGameContext->m_clientPlayerManager;
        if (!pPlayerManager || pPlayerManager->m_players.empty()) return;
        fb::ClientPlayer* pLocalPlayer = pPlayerManager->m_localPlayer;
        if (!pLocalPlayer) return;
        eastl::vector<fb::ClientPlayer*> pVecCP = pPlayerManager->m_players;
        std::set<fb::ClientVehicleEntity*> renderedVehicles;
        for (int i = 0; i < pVecCP.size(); i++) {
            fb::ClientPlayer* pClientPlayer = pVecCP.at(i);
            if (!pClientPlayer) continue;
            fb::ClientVehicleEntity* pVehicle = GetVehicle(pClientPlayer);
            if (!pVehicle || renderedVehicles.count(pVehicle)) continue;
            renderedVehicles.insert(pVehicle);
            fb::PhysicsEntity* pPE = pVehicle->m_physicsEntity;
            fb::Vec3 vehiclePos = {0,0,0};
            if (pPE) {
                vehiclePos = pPE->position();
            }
            fb::Vec3 screenPos;
            if (!ProjectToScreen(&vehiclePos, &screenPos)) continue;
            float distance = 0.0f;
            if (pLocalPlayer->m_soldier.GetData()) {
                distance = pLocalPlayer->m_soldier.GetData()->m_replicatedController->m_state.position.DistanceFrom(vehiclePos);
            }
            if (distance > g_VehicleESPConfig.maxDistance) continue;
            float baseDistance = 10.0f;
            float scale = baseDistance / (distance > 0.1f ? distance : 0.1f);
            if (scale < 0.1f) scale = 0.1f;
            if (scale > 2.0f) scale = 2.0f;
            float baseWidth = 120.0f;
            float baseHeight = 60.0f;
            float boxWidth = baseWidth * scale;
            float boxHeight = baseHeight * scale;
            float x = screenPos.x - boxWidth / 2;
            float y = screenPos.y - boxHeight / 2;
            float w = boxWidth;
            float h = boxHeight;
            fb::Color32 color = ImVec4ToColor32(g_VehicleESPConfig.boxColor);

            if (g_VehicleESPConfig.showBoxes) {
                DrawBox2D(x, y, w, h, color);
            }
            if (g_VehicleESPConfig.showHealthBars && pVehicle->m_vehicleHealth && !pVehicle->m_vehicleHealth->m_healthZones.empty()) {
                float health = pVehicle->m_vehicleHealth->m_healthZones[0].health;
                float maxHealth = pVehicle->m_vehicleHealth->m_healthZones[0].data ? pVehicle->m_vehicleHealth->m_healthZones[0].data->m_maxHealth : 100.0f;
                DrawHealthBar(x, y - 10 * scale, w, health, maxHealth, ImVec4ToColor32(g_VehicleESPConfig.healthBarColor));
            }
   
            float textX = x + w + 5 * scale;
            float textY = y;
            float textSpacing = 15 * scale;
            if (g_VehicleESPConfig.showNames) {
                char* vName = GetVehicleName(pClientPlayer);
                char nameText[64] = "Vehicle";
                if (vName && vName[0]) snprintf(nameText, sizeof(nameText), "%s", vName);
                DrawText2D(textX, textY, ImVec4ToColor32(g_VehicleESPConfig.nameColor), nameText, scale);
                textY += textSpacing;
            }
   
            if (g_VehicleESPConfig.showDistance) {
                char distText[32];
                snprintf(distText, sizeof(distText), "%.0fm", distance);
                DrawText2D(textX, textY, color, distText, scale);
                textY += textSpacing;
            }
     
            if (g_VehicleESPConfig.showHealthText && pVehicle->m_vehicleHealth && !pVehicle->m_vehicleHealth->m_healthZones.empty()) {
                float health = pVehicle->m_vehicleHealth->m_healthZones[0].health;
                float maxHealth = pVehicle->m_vehicleHealth->m_healthZones[0].data ? pVehicle->m_vehicleHealth->m_healthZones[0].data->m_maxHealth : 100.0f;
                char healthText[32];
                snprintf(healthText, sizeof(healthText), "%.0f/%.0f", health, maxHealth);
                DrawText2D(textX, textY, color, healthText, scale);
            }
        }
    }

    void RenderESP() {
        fb::ClientGameContext* g_pGameContext = fb::ClientGameContext::Singleton();
        if (!g_pGameContext) {
            return;
        }

        fb::ClientPlayerManager* pPlayerManager = g_pGameContext->m_clientPlayerManager;
        if (!pPlayerManager || pPlayerManager->m_players.empty()) {
            return;
        }

        fb::ClientPlayer* pLocalPlayer = pPlayerManager->m_localPlayer;
        if (!pLocalPlayer) {
            return;
        }

        fb::ClientSoldierEntity* pMySoldier = pLocalPlayer->getSoldierEnt();
        if (!pMySoldier || !pMySoldier->IsAlive()) {
            return;
        }

        eastl::vector<fb::ClientPlayer*> pVecCP = pPlayerManager->m_players;
        float screenWidth = ImGui::GetIO().DisplaySize.x;
        float screenHeight = ImGui::GetIO().DisplaySize.y;
        fb::Vec3 screenCenter = { screenWidth / 2.0f, screenHeight / 2.0f, 0.0f };
        int espCount = 0;
        for (int i = 0; i < pVecCP.size(); i++) {
            fb::ClientPlayer* pClientPlayer = pVecCP.at(i);
            if (!pClientPlayer) continue;
            if (pClientPlayer == pLocalPlayer) {
                continue;
            }
            bool isTeammate = (pLocalPlayer->m_teamId == pClientPlayer->m_teamId);
            if (isTeammate && !g_ESPConfig.showTeammates) continue;
            if (!isTeammate && !g_ESPConfig.showEnemies) continue;
            fb::ClientSoldierEntity* pEnemySoldier = pClientPlayer->getSoldierEnt();
            if (!pEnemySoldier || !pEnemySoldier->IsAlive()) {
                continue;
            }
            float distance = pMySoldier->m_replicatedController->m_state.position.DistanceToVector(pEnemySoldier->m_replicatedController->m_state.position);
            if (distance > g_ESPConfig.maxDistance) {
                continue;
            }
            fb::Vec3 screenPos;
            if (WorldToScreen(pEnemySoldier->m_replicatedController->m_state.position, screenPos)) {
                DrawPlayerESP(pClientPlayer, pEnemySoldier, screenPos, distance);
                espCount++;
            } else {
                // pointer
                fb::Color32 color = isTeammate 
                    ? fb::Color32(0, 255, 0, 255) 
                    : (pEnemySoldier->m_isOccluded ? fb::Color32(255, 255, 0, 255) : fb::Color32(255, 0, 0, 255));
                if (g_ESPConfig.showOffscreenPointers) {
                    DrawOffscreenPointer(screenCenter, pEnemySoldier->m_replicatedController->m_state.position, color);
                }
            }
        }
        if (g_VehicleESPConfig.enabled) {
            ESP::RenderVehicleESP();
        }
    }

    void DrawPlayerESP(fb::ClientPlayer* player, fb::ClientSoldierEntity* soldier, fb::Vec3& screenPos, float distance) {

        fb::ClientGameContext* g_pGameContext = fb::ClientGameContext::Singleton();
        fb::ClientPlayer* pLocalPlayer = g_pGameContext->m_clientPlayerManager->m_localPlayer;
        
     
        float baseDistance = 10.0f;
        float scale = baseDistance / distance;
   
        if (scale < 0.1f) scale = 0.1f;
        if (scale > 2.0f) scale = 2.0f;
        
  
        float baseWidth = 50.0f;
        float baseHeight = 100.0f;
        
 
        float boxWidth = baseWidth * scale;
        float boxHeight = baseHeight * scale;
        
        float x = screenPos.x - boxWidth / 2;
        float y = screenPos.y - boxHeight;
        float w = boxWidth;
        float h = boxHeight;

        bool isTeammate = (pLocalPlayer->m_teamId == player->m_teamId);
        fb::Color32 color = isTeammate
            ? ImVec4ToColor32(g_ESPConfig.boxColorTeammate)
            : (soldier->m_isOccluded ? ImVec4ToColor32(g_ESPConfig.boxColorOccluded) : ImVec4ToColor32(g_ESPConfig.boxColorEnemy));

  
        if (g_ESPConfig.showBoxes) {
            DrawBox2D(x, y, w, h, color);
        }

        if (g_ESPConfig.showHealthBars) {
            float health = soldier->m_health;
            float maxHealth = soldier->maxHealth();
            if (maxHealth > 0) {
                DrawHealthBar(x, y - 10 * scale, w, health, maxHealth, ImVec4ToColor32(g_ESPConfig.healthBarColor));
            }
        }

      
        float textX = x + w + 5 * scale;
        float textY = y;
        float textSpacing = 15 * scale;

      
        if (g_ESPConfig.showNames) {
            char nameText[64];
            sprintf_s(nameText, "%s", player->m_name.c_str());
            DrawText2D(textX, textY, ImVec4ToColor32(g_ESPConfig.nameColor), nameText, scale);
            textY += textSpacing;
        }

    
        if (g_ESPConfig.showDistance) {
            char distText[32];
            sprintf_s(distText, "%.0fm", distance);
            DrawText2D(textX, textY, color, distText, scale);
            textY += textSpacing;
        }

        if (g_ESPConfig.showHealthText) {
            float health = soldier->m_health;
            float maxHealth = soldier->maxHealth();
            char healthText[32];
            sprintf_s(healthText, "%.0f/%.0f", health, maxHealth);
            DrawText2D(textX, textY, color, healthText, scale);
        }

        if (g_ESPConfig.showPlayerNames) {
            DrawPlayerName(player, x + w / 2, y - 20.0f, ImVec4ToColor32(g_ESPConfig.nameColor), 1.0f);
        }
    }

    void DrawBox2D(float x, float y, float w, float h, fb::Color32 color) {
        fb::DebugRenderer2::Singleton()->DrawBox2D(x, y, w, h, color);
    }

    void DrawHealthBar(float x, float y, float w, float health, float maxHealth, fb::Color32 color) {
        if (maxHealth <= 0) return;
        
        float healthPercent = health / maxHealth;
        float barWidth = w * healthPercent;
        
        fb::DebugRenderer2::Singleton()->DrawBox2D(x, y, w, 5, fb::Color32(255, 0, 0, 255));
        

        if (barWidth > 0) {
            fb::DebugRenderer2::Singleton()->DrawBox2D(x, y, barWidth, 5, color);
        }
    }

    void DrawText2D(float x, float y, fb::Color32 color, const char* text, float scale) {
        fb::DebugRenderer2::Singleton()->drawText((int)x, (int)y, color, (char*)text, scale);
    }

    bool WorldToScreen(fb::Vec3& worldPos, fb::Vec3& screenPos) {
        fb::GameRenderer::Singleton()->m_viewParams.view.Update();
        return ProjectToScreen(&worldPos, &screenPos);
    }

    void DrawOffscreenPointer(const fb::Vec3& screenCenter, const fb::Vec3& targetWorld, fb::Color32 /*color*/) {
    
        float screenWidth = ImGui::GetIO().DisplaySize.x;
        float screenHeight = ImGui::GetIO().DisplaySize.y;
        float cx = screenCenter.x;
        float cy = screenCenter.y;

        fb::GameRenderer::Singleton()->m_viewParams.view.Update();
        fb::Vec3 camPos = fb::GameRenderer::Singleton()->m_viewParams.view.m_viewMatrixInverse.trans;
        fb::Vec3 toTarget = targetWorld;
        toTarget -= camPos;
        toTarget.normalize();
        fb::Vec3 camForward = fb::GameRenderer::Singleton()->m_viewParams.view.m_viewMatrixInverse.forward;
        camForward.normalize();
        float angle = atan2f(toTarget.x, toTarget.z) - atan2f(camForward.x, camForward.z);
  
        while (angle < -M_PI) angle += 2 * M_PI;
        while (angle > M_PI) angle -= 2 * M_PI;

   
        float margin = 32.0f;
        float pointerRadius = std::min(cx, cy) - margin;
        float px = cx + pointerRadius * sinf(angle);
        float py = cy - pointerRadius * cosf(angle);

        px = std::max(margin, std::min(px, screenWidth - margin));
        py = std::max(margin, std::min(py, screenHeight - margin));


        float size = 24.0f;
        ImVec2 tip(px, py);
        ImVec2 left(px - size * 0.5f, py + size);
        ImVec2 right(px + size * 0.5f, py + size);

        float s = sinf(angle);
        float c = cosf(angle);
        auto rotate = [&](ImVec2 p) -> ImVec2 {
            float x = p.x - px;
            float y = p.y - py;
            float rx = x * c - y * s;
            float ry = x * s + y * c;
            return ImVec2(rx + px, ry + py);
        };
        left = rotate(left);
        right = rotate(right);

        ImDrawList* draw = ImGui::GetForegroundDrawList();
        fb::Color32 col = ImVec4ToColor32(g_ESPConfig.offscreenPointerColor);
        ImU32 imCol = IM_COL32(col.R, col.G, col.B, col.A);
        draw->AddTriangleFilled(tip, left, right, imCol);
        draw->AddTriangle(tip, left, right, IM_COL32(0,0,0,180), 2.0f);
    }

    void DrawPlayerName(fb::ClientPlayer* player, float x, float y, fb::Color32 color, float /*scale*/) {
        if (!player) return;
        char nameText[64];
        sprintf_s(nameText, "%s", player->m_name.c_str());

        DrawText2D(x, y - 20.0f, color, nameText, 1.0f);
    }
} 