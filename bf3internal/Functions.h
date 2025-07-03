#include <io.h>
#include <fcntl.h>
#include "FB SDK/Frostbite.h"

#define M_PI 3.14159265359

// Function declarations only
void CreateConsole();
template<typename T, std::size_t N>
std::size_t array_size(T(&)[N]) {
	return N;
}
bool isalive(int i);
int GetWeaponID(fb::ClientSoldierEntity* pMySoldier);
bool ProjectToScreen(fb::Vec3* world, fb::Vec3* out);
bool GetBonePos(fb::ClientSoldierEntity* pEnt, int iBone, fb::Vec3 *vOut);
float GetDistance(fb::ClientPlayer* pLocal, fb::ClientPlayer* pPlayer);
fb::WeaponFiring* GetVehicleWeapon(fb::ClientPlayer *Player);
float Distance2D(float x1, float y1, float x2, float y2);
float Distance3D(float x1, float y1, float z1, float x2, float y2, float z2);
inline float XAngle(float x1, float y1, float x2, float y2, float myangle);
inline float YAngle(float x1, float y1, float z1, float x2, float y2, float z2, float myangle);
void RotatePointAlpha(float *outV, float x, float y, float z, float cx, float cy, float cz, float alpha);
bool IsVisible(fb::Vec3* target, fb::ClientSoldierEntity* pMySoldier);
void AimCorrection(fb::Vec3 MyVelocity, fb::Vec3 EnemyVelocity, fb::Vec3* InVec, float Distance, float Bulletspeed, float Gravity);

struct stMyVehicle
{
	// meine Version
	char* m_Game_Name;
	bool m_SeatHasWeapon[6]; // true -> VehicleWeapon
	bool m_OpenSeat[6]; // true -> Soldierweapon
};

char* GetVehicleName(fb::ClientPlayer* Player);
stMyVehicle* GetVehicleValues(char* VehicleName);
fb::ClientVehicleEntity* GetVehicle(fb::ClientPlayer* Player);
fb::FiringFunctionData* GetPlayerFFD(fb::ClientSoldierEntity* pMySoldier);
fb::Vec3 getVehicleSpeed(fb::ClientSoldierEntity* pMySoldier);
void _stdcall Bulletesp();

// No Recoil and No Spread functions
void ApplyNoRecoil(fb::ClientSoldierEntity* pMySoldier);
void ApplyNoSpread(fb::ClientSoldierEntity* pMySoldier);