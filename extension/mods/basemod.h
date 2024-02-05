#ifndef EXT_BASE_MOD_H_
#define EXT_BASE_MOD_H_

#include <optional>

class CBaseExtPlayer;
class CBaseBot;
class CNavMesh;

#include "gamemods_shared.h"

// Base game mod class
class CBaseMod
{
public:
	CBaseMod();
	virtual ~CBaseMod();

	// Called every server frame
	virtual void Frame() {}

	virtual void OnMapStart();
	virtual void OnMapEnd() {}

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

	// Allocates the nav mesh class used by the mod
	virtual CNavMesh* NavMeshFactory();
	// Returns the entity index of the player resource/manager entity.
	virtual std::optional<int> GetPlayerResourceEntity();

	// return false to call NavIsEntityWalkable ir order to determine if the entity is walkable. Used for nav mesh editing and generation.
	virtual bool NavIsEntityIgnored(edict_t* entity, unsigned int flags) { return true; }
	// Returns true if the entity is walkable, false if it should block nav
	virtual bool NavIsEntityWalkable(edict_t* entity, unsigned int flags) { return true; }
private:
	CBaseHandle m_playerresourceentity;

	void InternalFindPlayerResourceEntity();
};

#endif // !EXT_BASE_MOD_H_
