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

	// Called for mods to register event listeners
	virtual void RegisterGameEvents();

	virtual const Vector& GetPlayerHullMins();
	virtual const Vector& GetPlayerHullMaxs();

	// if this is true, then we need to hook runplayercommand
	// for example: DoDs already run player commands on bots
	virtual bool UserCommandNeedsHook() { return false; }

private:

};

#endif // !EXT_BASE_MOD_H_
