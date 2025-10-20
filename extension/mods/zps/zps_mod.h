#ifndef __NAVBOT_ZPS_MOD_H_
#define __NAVBOT_ZPS_MOD_H_

#include <mods/basemod.h>
#include "zps_lib.h"

class CZombiePanicSourceMod : public CBaseMod
{
public:
	CZombiePanicSourceMod();

	CBaseBot* AllocateBot(edict_t* edict) override;

	// Mod name (IE: Team Fortress 2)
	const char* GetModName() override { return "Zombie Panic! Source"; }
	// Mod ID
	Mods::ModType GetModType() override { return Mods::ModType::MOD_ZPS; }
};


#endif // !__NAVBOT_ZPS_MOD_H_
