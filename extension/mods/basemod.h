#ifndef EXT_BASE_MOD_H_
#define EXT_BASE_MOD_H_

class CBaseExtPlayer;
class CBaseBot;

namespace Mods
{
	// Game/Mod IDs
	enum ModType
	{
		MOD_INVALID_ID = -1, // Invalid/Unknown mod
		MOD_BASE = 0, // Generic mod
		MOD_TF2, // Team Fortress 2
		MOD_CSS, // Counter-Strike: Source
		MOD_DODS, // Day of Defeat: Source
		MOD_HL2DM, // Half-Life 2: Deathmatch

		LAST_MOD_TYPE
	};
}

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
