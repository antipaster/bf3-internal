#ifndef _ClientDamageGiverInfo_H
#define _ClientDamageGiverInfo_H
#include "FB SDK/Frostbite_Classes.h"
namespace fb
{
	class ClientDamageGiverInfo
	{
	public:
		const class fb::ClientPlayer * m_damageGiver;                     // this+0x0
		void* m_weaponUnlockAsset;                     // this+0x4

	}; // fb::ClientDamageGiverInfo

};

#endif