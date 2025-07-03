#pragma once
#include "FB SDK/Frostbite.h"


struct AimbotConfig {
    bool enabled = false;
    bool triggerbot = false;
    float fov = 5.0f;
    float smooth = 1.0f;
    bool aimAtHead = true;
    bool visibilityCheck = true;
    int targetBone = 1; // 1 = head, 2 = chest, etc.
    float maxDistance = 200.0f;
    bool aimOnButton = false;
    int aimButton = 0; // 0 = left mouse, 1 = right mouse, 2 = middle mouse
    int activationMode = 0; // 0 = Hold Key, 1 = Always On
    int targetType = 0; // 0 = Enemies only, 1 = Teammates only, 2 = Both
    bool magicBullet = false;
    bool instantKill = false;
    bool rapidFire = false;
    bool fastReload = false;
    // Prediction options
    bool prediction = false;
    float predictionMultiplier = 1.0f;
} extern g_AimbotConfig;

struct MiscConfig {
    bool autoCrouch = false;
};
extern MiscConfig g_MiscConfig;

namespace Aimbot {
    void UpdateAimbot();
    bool GetBestTarget(fb::ClientSoldierEntity** pTarget, fb::Vec3* targetPos);
    bool IsInFOV(fb::Vec3& targetPos, float fov);
    bool IsVisible(fb::Vec3& targetPos);
    void SmoothAim(fb::Vec3& currentAim, fb::Vec3& targetAim, float smooth);
    fb::Vec3 GetBonePosition(fb::ClientSoldierEntity* soldier, int boneId);
    float GetDistance(fb::Vec3& pos1, fb::Vec3& pos2);
    void ApplyAim(float pitch, float yaw);
    void MagicBullet(fb::ClientSoldierEntity* pTarget, float damage = 100.0f);
}

namespace Triggerbot {
    void UpdateTriggerbot();
} 