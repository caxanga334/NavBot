#ifndef NAVBOT_HALFLIFE2DEATHMATCH_MOD_H_
#define NAVBOT_HALFLIFE2DEATHMATCH_MOD_H_

#include <mods/basemod.h>

class CHalfLife2DeathMatchMod : public CBaseMod
{
public:
	CHalfLife2DeathMatchMod();
	~CHalfLife2DeathMatchMod() override;

	CBaseBot* AllocateBot(edict_t* edict) override;

private:

};


#endif // !NAVBOT_HALFLIFE2DEATHMATCH_MOD_H_
