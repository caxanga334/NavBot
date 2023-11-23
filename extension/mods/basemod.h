#ifndef EXT_BASE_MOD_H_
#define EXT_BASE_MOD_H_

class CBaseExtPlayer;
class CBaseBot;

#include "gamemods_shared.h"

// Base game mod class
class CBaseMod
{
public:
	CBaseMod();
	virtual ~CBaseMod();

	// Called every server frame
	virtual void Frame() {}

	// Called by the manager when allocating a new bot instance
	virtual CBaseBot* AllocateBot(edict_t* edict);

	virtual const char* GetModName() { return "CBaseMod"; }

	virtual Mods::ModType GetModType() { return Mods::ModType::MOD_BASE; }

private:

};

#endif // !EXT_BASE_MOD_H_
