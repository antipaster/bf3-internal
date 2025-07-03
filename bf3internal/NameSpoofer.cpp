#include "FB SDK/Includes.h"
#include "FB SDK\Frostbite.h"
#include "NameSpoofer.h"
#include <cstring>
#include <mutex>

static bool g_enabled = false;
static char g_originalName[64] = "";
static char g_spoofedName[64] = "";
static bool g_backup = true;
static std::mutex g_mutex;

void NameSpoofer::SpoofName(const char* newName) {
    std::lock_guard<std::mutex> lock(g_mutex);
    std::strncpy(g_spoofedName, newName, sizeof(g_spoofedName) - 1);
    g_spoofedName[sizeof(g_spoofedName) - 1] = '\0';
    g_enabled = true;
}

void NameSpoofer::RestoreName() {
    std::lock_guard<std::mutex> lock(g_mutex);
    g_enabled = false;
}

void NameSpoofer::SetEnabled(bool enabled) {
    std::lock_guard<std::mutex> lock(g_mutex);
    g_enabled = enabled;
}

bool NameSpoofer::IsEnabled() {
    std::lock_guard<std::mutex> lock(g_mutex);
    return g_enabled;
}

void NameSpoofer::SetName(const char* name) {
    std::lock_guard<std::mutex> lock(g_mutex);
    std::strncpy(g_spoofedName, name, sizeof(g_spoofedName) - 1);
    g_spoofedName[sizeof(g_spoofedName) - 1] = '\0';
}

const char* NameSpoofer::GetOriginalName() {
    std::lock_guard<std::mutex> lock(g_mutex);
    return g_originalName;
}

void NameSpoofer::OnFrame() {
    std::lock_guard<std::mutex> lock(g_mutex);
    fb::ClientGameContext* game_context = fb::ClientGameContext::Singleton();
    if (!game_context) return;
    fb::ClientPlayerManager* player_manager = game_context->m_clientPlayerManager;
    if (!player_manager) return;
    fb::ClientPlayer* local_player = player_manager->m_localPlayer;
    if (!local_player) return;

    // Backup original name (one-time)
    if (g_backup && local_player->am_name[0] != '\0') {
        std::strncpy(g_originalName, local_player->am_name, sizeof(g_originalName) - 1);
        g_originalName[sizeof(g_originalName) - 1] = '\0';
        g_backup = false;
    }

    fb::ClientSoldierEntity* local_soldier = local_player->getSoldierEnt();
    if (!local_soldier) return;

    // Restore name logic
    if (!g_enabled) {
        if (strcmp(local_player->am_name, g_originalName) != 0) {
            std::strncpy(local_player->am_name, g_originalName, sizeof(local_player->am_name) - 1);
            local_player->am_name[sizeof(local_player->am_name) - 1] = '\0';
            strcpy(reinterpret_cast<char*>(local_player) + 0x0040, g_originalName);
            strcpy(reinterpret_cast<char*>(local_player) + 0x1836, g_originalName);
        }
        return;
    }

    // Apply spoof if enabled and player is alive
    if (g_enabled && local_soldier->IsAlive()) {
        if (strcmp(local_player->am_name, g_spoofedName) != 0) {
            std::strncpy(local_player->am_name, g_spoofedName, sizeof(local_player->am_name) - 1);
            local_player->am_name[sizeof(local_player->am_name) - 1] = '\0';
            strcpy(reinterpret_cast<char*>(local_player) + 0x0040, g_spoofedName);
            strcpy(reinterpret_cast<char*>(local_player) + 0x1836, g_spoofedName);
        }
    }
} 