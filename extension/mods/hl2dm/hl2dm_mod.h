#ifndef NAVBOT_HALFLIFE2DEATHMATCH_MOD_H_
#define NAVBOT_HALFLIFE2DEATHMATCH_MOD_H_

#include <mods/basemod.h>

class CHalfLife2DeathMatchMod : public CBaseMod
{
public:
	CHalfLife2DeathMatchMod();
	~CHalfLife2DeathMatchMod() override;

	const char* GetModName() override { return "Half-Life 2: Deathmatch"; }
	Mods::ModType GetModType() override { return Mods::ModType::MOD_HL2DM; }

	CBaseBot* AllocateBot(edict_t* edict) override;

private:

};


#endif // !NAVBOT_HALFLIFE2DEATHMATCH_MOD_H_
