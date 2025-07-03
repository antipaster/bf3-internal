#include "Aimbot.h"
#include "Functions.h"
#include "FB SDK/Includes.h"
#include "FB SDK/ClientAimingReplication.h"
#include "FB SDK/ClientSoldierAimingSimulation.h"
#include "FB SDK/AimAssist.h"
#include "FB SDK/ClientSoldierWeapon.h"
#include "FB SDK/ClientSoldierWeaponsComponent.h"
#include "FB SDK/FiringFunctionData.h"
#include "FB SDK/BulletEntityData.h"
#include "FB SDK/WeaponFiring.h"
#include "FB SDK/WeaponSway.h"
#include "FB SDK/ClientSoldierEntity.h"
#include "FB SDK/ClientPlayer.h"
#include "FB SDK/ClientGameContext.h"
#include "FB SDK/GameRenderer.h"
#include "FB SDK/DxRenderer.h"
#include "FB SDK/Offsets.h" // for INPUT_CROUCH
#include <cmath>
#include <cstdio>
#include <Windows.h> // For keybd_event

#define M_PI 3.14159265358979323846

AimbotConfig g_AimbotConfig = {
    false,  // enabled
    false,  // triggerbot
    5.0f,   // fov
    1.0f,   // smooth
    true,   // aimAtHead
    true,   // visibilityCheck
    1,      // targetBone
    200.0f, // maxDistance
    false,  // aimOnButton
    0       // aimButton
};

MiscConfig g_MiscConfig;

namespace Aimbot {
    bool IsMouseButtonPressed(int button) {
        switch (button) {
            case 0: // Left mouse
                return (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
            case 1: // Right mouse
                return (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
            case 2: // Middle mouse
                return (GetAsyncKeyState(VK_MBUTTON) & 0x8000) != 0;
            default:
                return false;
        }
    }


    float AngleBetween(const fb::Vec3& a, const fb::Vec3& b) {
        fb::Vec3 aCopy = a;
        fb::Vec3 bCopy = b;
        float dot = aCopy.Dot(bCopy);
        float lenA = aCopy.VectorLength();
        float lenB = bCopy.VectorLength();
        if (lenA == 0 || lenB == 0) return 180.0f;
        float cosTheta = dot / (lenA * lenB);
        if (cosTheta > 1.0f) cosTheta = 1.0f;
        if (cosTheta < -1.0f) cosTheta = -1.0f;
        return acosf(cosTheta) * (180.0f / M_PI);
    }

    
    void SetCrouchInternal(bool crouch) {
        fb::ClientGameContext* g_pGameContext = fb::ClientGameContext::Singleton();
        if (!POINTERCHK(g_pGameContext)) return;
        fb::ClientPlayerManager* pPlayerManager = g_pGameContext->m_clientPlayerManager;
        if (!POINTERCHK(pPlayerManager)) return;
        fb::ClientPlayer* pLocalPlayer = pPlayerManager->m_localPlayer;
        if (!POINTERCHK(pLocalPlayer)) return;

        fb::EntryInput* input = pLocalPlayer->m_input;
        if (!POINTERCHK(input)) return;

        input->m_analogInput[INPUT_CROUCH] = crouch ? 1.0f : 0.0f;
    }

    void UpdateAimbot() {
        if (!g_AimbotConfig.enabled) {
            SetCrouchInternal(false);
            return;
        }
        

        static const fb::Vec3 multipointOffsets[] = {
            {0, 0, 0}, // center
            {0.15f, 0, 0}, {-0.15f, 0, 0},
            {0, 0.15f, 0}, {0, -0.15f, 0},
            {0, 0, 0.15f}, {0, 0, -0.15f}
        };
        const int numMultipoints = sizeof(multipointOffsets) / sizeof(multipointOffsets[0]);
        

        if (g_AimbotConfig.activationMode == 0) {
            if (!IsMouseButtonPressed(g_AimbotConfig.aimButton)) {
                return;
            }
        }
        
        fb::ClientGameContext* g_pGameContext = fb::ClientGameContext::Singleton();
        if (!POINTERCHK(g_pGameContext)) {
            return;
        }

        fb::ClientPlayerManager* pPlayerManager = g_pGameContext->m_clientPlayerManager;
        if (!POINTERCHK(pPlayerManager) || pPlayerManager->m_players.empty()) {
            return;
        }

        fb::ClientPlayer* pLocalPlayer = pPlayerManager->m_localPlayer;
        if (!POINTERCHK(pLocalPlayer)) {
            return;
        }

        fb::ClientSoldierEntity* pMySoldier = pLocalPlayer->getSoldierEnt();
        if (!POINTERCHK(pMySoldier)) {
            return;
        }

        if (!pMySoldier->IsAlive()) {
            return;
        }

   
        if (g_AimbotConfig.instantKill) {
            fb::ClientSoldierWeapon* MyCSW = pMySoldier->GetCSW();
            if (POINTERCHK(MyCSW)
                && POINTERCHK(MyCSW->m_weapon)
                && POINTERCHK(MyCSW->m_predictedFiring)
                && POINTERCHK(MyCSW->m_predictedFiring->m_data->m_primaryFire)) {
                fb::FiringFunctionData* pFFD = pMySoldier->getCurrentWeaponFiringData()->m_primaryFire;
                if (POINTERCHK(pFFD)) {
                    fb::BulletEntityData* pBED = pFFD->m_shot.m_projectileData;
                    if (POINTERCHK(pBED)) {
                        pBED->m_startDamage = 999.0f;
                        pBED->m_endDamage = 999.0f;
                        pBED->m_damageFalloffStartDistance = 99999.0f;
                        pBED->m_damageFalloffEndDistance = 99999.0f;
                    }
                }
            }
        }


        {
            fb::FiringFunctionData* pFFD = pMySoldier->getCurrentWeaponFiringData()->m_primaryFire;
            if (POINTERCHK(pFFD)) {
                fb::BulletEntityData* pBED = pFFD->m_shot.m_projectileData;
                if (POINTERCHK(pBED)) {
                    //fucked for now
                    pBED->m_instantHit = true;
                }
            }
        }


        if (g_AimbotConfig.rapidFire) {
            fb::FiringFunctionData* pFFD = pMySoldier->getCurrentWeaponFiringData()->m_primaryFire;
            if (POINTERCHK(pFFD)) {
                pFFD->m_fireLogic.m_rateOfFire = 9999.0f;
                pFFD->m_fireLogic.m_rateOfFireForBurst = 9999.0f;
                pFFD->m_fireLogic.m_clientFireRateMultiplier = 10.0f;
            }
        }


        if (g_AimbotConfig.fastReload) {
            fb::FiringFunctionData* pFFD = pMySoldier->getCurrentWeaponFiringData()->m_primaryFire;
            if (POINTERCHK(pFFD)) {
                pFFD->m_fireLogic.m_reloadDelay = 0.01f;
                pFFD->m_fireLogic.m_reloadTime = 0.01f;
                pFFD->m_fireLogic.m_reloadTimeBulletsLeft = 0.01f;
                pFFD->m_ammoCrateReloadDelay = 0.01f;
            }
        }

        eastl::vector<fb::ClientPlayer*> pVecCP = pPlayerManager->m_players;
        if (pVecCP.empty()) return;


        float screenWidth = (float)fb::DxRenderer::Singleton()->m_screenInfo.m_nWidth;
        float screenHeight = (float)fb::DxRenderer::Singleton()->m_screenInfo.m_nHeight;
        float centerX = screenWidth / 2.0f;
        float centerY = screenHeight / 2.0f;
        float bestDist = g_AimbotConfig.fov;
        fb::ClientPlayer* bestPlayer = nullptr;
        fb::ClientSoldierEntity* bestSoldier = nullptr;
        fb::Vec3 bestTargetWorld;
        fb::Vec3 bestTargetScreen;

        for (int i = 0; i < pVecCP.size(); i++) {
            fb::ClientPlayer* pClientPlayer = pVecCP.at(i);
            if (!POINTERCHK(pClientPlayer)) continue;
      
            bool isTeammate = (pMySoldier->m_teamId == pClientPlayer->m_teamId);
            if (g_AimbotConfig.targetType == 0 && isTeammate) continue; // Enemies only
            if (g_AimbotConfig.targetType == 1 && !isTeammate) continue; // Teammates only
        

            fb::ClientSoldierEntity* pEnemySoldier = pClientPlayer->getSoldierEnt();
            if (!POINTERCHK(pEnemySoldier)) continue;
            if (!pEnemySoldier->IsAlive()) continue;
            fb::WeakPtr<fb::ClientSoldierEntity>* corpse = &pClientPlayer->m_corpse;
            if (corpse->GetData()) continue;

            fb::Vec3 baseBone;
            if (!GetBonePos(pEnemySoldier, g_AimbotConfig.targetBone, &baseBone)) continue;

       
            fb::Vec3 aimPos = baseBone;
            if (g_AimbotConfig.prediction) {
        
                fb::Vec3 enemyVel = pEnemySoldier->m_replicatedController->m_state.velocity;
                fb::Vec3 myVel = pMySoldier->m_replicatedController->m_state.velocity;
            
                fb::FiringFunctionData* pFFD = GetPlayerFFD(pMySoldier);
                float bulletSpeed = 600.0f;
                float bulletGravity = 9.81f;
                if (pFFD && pFFD->m_shot.m_projectileData) {
                    bulletSpeed = pFFD->m_shot.m_initialSpeed.z;
                    bulletGravity = pFFD->m_shot.m_projectileData->m_gravity;
                }
              
                AimCorrection(myVel, enemyVel, &aimPos, pMySoldier->m_replicatedController->m_state.position.DistanceToVector(baseBone), bulletSpeed, bulletGravity);
              
                aimPos = baseBone + (aimPos - baseBone) * g_AimbotConfig.predictionMultiplier;
            }
          
            for (int m = 0; m < numMultipoints; ++m) {
                fb::Vec3 Enemyvec = aimPos + multipointOffsets[m];
            if (g_AimbotConfig.visibilityCheck) {
                if (!::IsVisible(&Enemyvec, pMySoldier)) continue;
            }
        
            fb::GameRenderer::Singleton()->m_viewParams.view.Update();
            fb::Vec3 Origin = fb::GameRenderer::Singleton()->m_viewParams.view.m_viewMatrixInverse.trans;
            float worldDist = Origin.DistanceToVector(Enemyvec);
            if (worldDist > g_AimbotConfig.maxDistance) continue;
    
            fb::Vec3 screenPos;
            if (!ProjectToScreen(&Enemyvec, &screenPos)) continue;
            float distToCrosshair = sqrtf((screenPos.x - centerX) * (screenPos.x - centerX) + (screenPos.y - centerY) * (screenPos.y - centerY));
            if (distToCrosshair > bestDist) continue;
            bestDist = distToCrosshair;
            bestPlayer = pClientPlayer;
            bestSoldier = pEnemySoldier;
            bestTargetWorld = Enemyvec;
            bestTargetScreen = screenPos;
            }
        }

        if (!POINTERCHK(bestPlayer) || !POINTERCHK(bestSoldier)) {
            return;
        }

        // doesnt work for now
        if (g_AimbotConfig.magicBullet && POINTERCHK(bestSoldier)) {
            MagicBullet(bestSoldier, 999.0f);
        }

      
        fb::GameRenderer::Singleton()->m_viewParams.view.Update();
        fb::Vec3 Origin = fb::GameRenderer::Singleton()->m_viewParams.view.m_viewMatrixInverse.trans;
        fb::Vec3 vDir = bestTargetWorld - Origin;
        vDir.normalize();
        float flYaw = -atan2(vDir.x, vDir.z);
        float flPitch = atan2(vDir.y, vDir.VectorLength2());
        const float MinYaw = 0;
        const float MaxYaw = M_PI * 2;
        if (flYaw < MinYaw)
            flYaw += MaxYaw;
        if (!pMySoldier->isInVehicle()) {
            fb::ClientSoldierWeapon* pClientSoldierWeapon = pMySoldier->GetCSW();
            if (POINTERCHK(pClientSoldierWeapon)) {
                fb::ClientSoldierAimingSimulation* aimer = pClientSoldierWeapon->m_authorativeAiming;
                if (!POINTERCHK(aimer))
                    return;
                // Smoothing: 1.0 = instant, <1.0 = smooth
                float smooth = g_AimbotConfig.smooth;
                if (smooth < 0.01f) smooth = 0.01f;
                if (smooth > 1.0f) smooth = 1.0f;
                if (smooth < 1.0f) {
                    aimer->m_fpsAimer->m_yaw += (flYaw - aimer->m_fpsAimer->m_yaw) * smooth;
                    aimer->m_fpsAimer->m_pitch += (flPitch - aimer->m_fpsAimer->m_pitch) * smooth;
                } else {
                    aimer->m_fpsAimer->m_yaw = flYaw;
                    aimer->m_fpsAimer->m_pitch = flPitch;
                }
            }
        }
       
        if (g_MiscConfig.autoCrouch) {
            bool isShooting = IsMouseButtonPressed(0); // Left mouse button
            SetCrouchInternal(isShooting);
        } else {
            SetCrouchInternal(false);
        }
    }

    bool GetBestTarget(fb::ClientSoldierEntity** pTarget, fb::Vec3* targetPos) {
        fb::ClientGameContext* g_pGameContext = fb::ClientGameContext::Singleton();
        if (!POINTERCHK(g_pGameContext)) return false;

        fb::ClientPlayerManager* pPlayerManager = g_pGameContext->m_clientPlayerManager;
        if (!POINTERCHK(pPlayerManager) || pPlayerManager->m_players.empty()) return false;

        fb::ClientPlayer* pLocalPlayer = pPlayerManager->m_localPlayer;
        if (!POINTERCHK(pLocalPlayer)) return false;

        fb::ClientSoldierEntity* pMySoldier = pLocalPlayer->getSoldierEnt();
        if (!POINTERCHK(pMySoldier)) return false;

        if (!pMySoldier->IsAlive()) return false;

        eastl::vector<fb::ClientPlayer*> pVecCP = pPlayerManager->m_players;
        if (pVecCP.empty()) return false;

        float closestdistance = 9999.0f;
        fb::ClientSoldierEntity* ClosestSold = NULL;
        fb::Vec3 EnemyAimVec;

        fb::GameRenderer::Singleton()->m_viewParams.view.Update();
        fb::Vec3 Origin = fb::GameRenderer::Singleton()->m_viewParams.view.m_viewMatrixInverse.trans;
        fb::Vec3 ViewDir = fb::GameRenderer::Singleton()->m_viewParams.view.m_viewMatrixInverse.forward;

        for (int i = 0; i < pVecCP.size(); i++) {
            fb::ClientPlayer* pClientPlayer = pVecCP.at(i);
            if (!POINTERCHK(pClientPlayer)) continue;

            if (pMySoldier->m_teamId == pClientPlayer->m_teamId) continue;

            fb::ClientSoldierEntity* pEnemySoldier = pClientPlayer->getSoldierEnt();
            if (!POINTERCHK(pEnemySoldier)) continue;

            if (!pEnemySoldier->IsAlive()) continue;

            fb::WeakPtr<fb::ClientSoldierEntity>* corpse = &pClientPlayer->m_corpse;
            if (corpse->GetData()) continue;

            fb::Vec3 Enemyvec;
            if (!GetBonePos(pEnemySoldier, g_AimbotConfig.targetBone, &Enemyvec)) continue;

            if (g_AimbotConfig.visibilityCheck) {
                if (!::IsVisible(&Enemyvec, pMySoldier)) continue;
            }

            fb::Vec3 ToTarget = Enemyvec - Origin;
            ToTarget.normalize();
            float angle = AngleBetween(ViewDir, ToTarget);
            if (angle > g_AimbotConfig.fov / 2.0f) continue;
            float flDistance = Origin.DistanceToVector(Enemyvec);
            if (flDistance < closestdistance) {
                ClosestSold = pEnemySoldier;
                closestdistance = flDistance;
                EnemyAimVec = Enemyvec;
            }
        }

        if (POINTERCHK(ClosestSold)) {
            *pTarget = ClosestSold;
            *targetPos = EnemyAimVec;
            return true;
        }

        return false;
    }

    bool IsInFOV(fb::Vec3& targetPos, float fov) {
        fb::GameRenderer::Singleton()->m_viewParams.view.Update();
        fb::Vec3 Origin = fb::GameRenderer::Singleton()->m_viewParams.view.m_viewMatrixInverse.trans;
        fb::Vec3 ViewDir = fb::GameRenderer::Singleton()->m_viewParams.view.m_viewMatrixInverse.forward;
        fb::Vec3 ToTarget = targetPos - Origin;
        ToTarget.normalize();
        float angle = AngleBetween(ViewDir, ToTarget);
        return angle <= fov / 2.0f;
    }

    bool IsVisible(fb::Vec3& targetPos) {
        fb::ClientGameContext* g_pGameContext = fb::ClientGameContext::Singleton();
        if (!POINTERCHK(g_pGameContext)) return false;

        fb::ClientPlayerManager* pPlayerManager = g_pGameContext->m_clientPlayerManager;
        if (!POINTERCHK(pPlayerManager)) return false;

        fb::ClientPlayer* pLocalPlayer = pPlayerManager->m_localPlayer;
        if (!POINTERCHK(pLocalPlayer)) return false;

        fb::ClientSoldierEntity* pMySoldier = pLocalPlayer->getSoldierEnt();
        if (!POINTERCHK(pMySoldier)) return false;

        return ::IsVisible(&targetPos, pMySoldier);
    }

    void SmoothAim(fb::Vec3& currentAim, fb::Vec3& targetAim, float smooth) {
        currentAim.x += (targetAim.x - currentAim.x) * smooth;
        currentAim.y += (targetAim.y - currentAim.y) * smooth;
        currentAim.z += (targetAim.z - currentAim.z) * smooth;
    }

    fb::Vec3 GetBonePosition(fb::ClientSoldierEntity* soldier, int boneId) {
        fb::Vec3 pos;
        if (GetBonePos(soldier, boneId, &pos)) {
            return pos;
        }
        pos.x = 0; pos.y = 0; pos.z = 0;
        return pos;
    }

    float GetDistance(fb::Vec3& pos1, fb::Vec3& pos2) {
        return pos1.DistanceToVector(pos2);
    }

    void ApplyAim(float pitch, float yaw) {
        fb::ClientGameContext* g_pGameContext = fb::ClientGameContext::Singleton();
        if (!POINTERCHK(g_pGameContext)) return;

        fb::ClientPlayerManager* pPlayerManager = g_pGameContext->m_clientPlayerManager;
        if (!POINTERCHK(pPlayerManager)) return;

        fb::ClientPlayer* pLocalPlayer = pPlayerManager->m_localPlayer;
        if (!POINTERCHK(pLocalPlayer)) return;

        fb::ClientSoldierEntity* pMySoldier = pLocalPlayer->getSoldierEnt();
        if (!POINTERCHK(pMySoldier)) return;

        if (!pMySoldier->IsAlive()) return;

        if (!pMySoldier->isInVehicle()) {
            fb::ClientSoldierWeapon* pClientSoldierWeapon = pMySoldier->GetCSW();
            if (POINTERCHK(pClientSoldierWeapon)) {
                fb::ClientSoldierAimingSimulation* aimer = pClientSoldierWeapon->m_authorativeAiming;
                if (POINTERCHK(aimer) && POINTERCHK(aimer->m_fpsAimer)) {
                    aimer->m_fpsAimer->m_yaw = yaw;
                    aimer->m_fpsAimer->m_pitch = pitch;
                }
            }
        }
    }

    int GetWeaponID(fb::ClientSoldierEntity* soldier) {
        if (!soldier) return -1;
        
        fb::ClientSoldierWeapon* weapon = soldier->GetCSW();
        if (!weapon || !weapon->m_weapon) return -1;

        return 0;
    }

    bool GetBonePos(fb::ClientSoldierEntity* soldier, int boneId, fb::Vec3* pos) {
        if (!soldier || !pos) return false;
        
    
        if (soldier->m_replicatedController) {
            *pos = soldier->m_replicatedController->m_state.position;
            

            if (boneId == 45) { // fb::Head
                pos->y += 1.8f;
            }
            
            return true;
        }
        
        return false;
    }

    bool IsVisible(fb::Vec3* targetPos, fb::ClientSoldierEntity* soldier) {
        if (!targetPos || !soldier) return false;

        return true;
    }

    bool ProjectToScreen(fb::Vec3* worldPos, fb::Vec3* screenPos) {
        if (!worldPos || !screenPos) return false;
        

        
        fb::GameRenderer* renderer = fb::GameRenderer::Singleton();
        if (!renderer) return false;
        

        float screenWidth = (float)fb::DxRenderer::Singleton()->m_screenInfo.m_nWidth;
        float screenHeight = (float)fb::DxRenderer::Singleton()->m_screenInfo.m_nHeight;
  
        screenPos->x = (worldPos->x / worldPos->z) * screenWidth + screenWidth / 2;
        screenPos->y = (worldPos->y / worldPos->z) * screenHeight + screenHeight / 2;
        screenPos->z = worldPos->z;
        
        return true;
    }

    void MagicBullet(fb::ClientSoldierEntity* pTarget, float damage) {
        if (!POINTERCHK(pTarget) || !POINTERCHK(pTarget->m_healthModule)) return;

        fb::ClientGameContext* g_pGameContext = fb::ClientGameContext::Singleton();
        if (!POINTERCHK(g_pGameContext)) return;
        fb::ClientPlayerManager* pPlayerManager = g_pGameContext->m_clientPlayerManager;
        if (!POINTERCHK(pPlayerManager)) return;
        fb::ClientPlayer* pLocalPlayer = pPlayerManager->m_localPlayer;
        if (!POINTERCHK(pLocalPlayer)) return;

        fb::ClientSoldierEntity* pMySoldier = pLocalPlayer->getSoldierEnt();
        if (!POINTERCHK(pMySoldier)) return;

    
        fb::Vec3 targetHead = GetBonePosition(pTarget, 1); // 1 = head
        fb::Vec3 myHead = GetBonePosition(pMySoldier, 1);

        fb::Vec3 dir = targetHead - myHead;
        dir.normalize();

        fb::ClientDamageInfo dmgInfo;
        dmgInfo.m_position = targetHead;
        dmgInfo.m_direction = dir;
        dmgInfo.m_origin = myHead;
        dmgInfo.m_damage = damage;
        dmgInfo.m_distributeDamageOverTime = 0.0f;
        dmgInfo.m_boneIndex = 1; // head
        dmgInfo.m_hitDirection = 0;
        dmgInfo.m_isBulletDamage = true;

        fb::ClientDamageGiverInfo dmgGiver;
        dmgGiver.m_damageGiver = pLocalPlayer;
        dmgGiver.m_weaponUnlockAsset = nullptr;

        pTarget->m_healthModule->onDamage(dmgInfo, dmgGiver);
    }
}

namespace Triggerbot {
    void UpdateTriggerbot() {
        if (!g_AimbotConfig.triggerbot) {
            return;
        }
        fb::ClientSoldierEntity* pTarget = nullptr;
        fb::Vec3 targetPos;
        if (!Aimbot::GetBestTarget(&pTarget, &targetPos)) {
            return;
        }

        if (!Aimbot::IsInFOV(targetPos, g_AimbotConfig.fov)) return;
        if (g_AimbotConfig.visibilityCheck && !Aimbot::IsVisible(targetPos)) return;

        fb::ClientSoldierWeapon* pWeapon = pTarget->GetCSW();
        if (pWeapon && !pWeapon->m_weaponStates1p.empty() && pWeapon->m_weaponStates1p[0].m_animation) {
            auto anim = pWeapon->m_weaponStates1p[0].m_animation;
            byte* firing = (byte*)&anim->m_isFiring;
            firing[0] = 1; // m_value
            firing[1] = 1; // m_changedThisFrame
        }
    }
} 