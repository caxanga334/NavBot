#ifndef NAVBOT_GAME_MODS_SHARED_H_
#define NAVBOT_GAME_MODS_SHARED_H_
#pragma once

// Shared data between all mods

namespace Mods
{
	/* If you're adding a new mod and it doesn't exists, just add it to the bottom of the enum, before LAST_MOD_TYPE */

	// Game/Mod IDs
	enum ModType : int
	{
		MOD_ALL = -2, // Indicate that something should apply for all mods
		MOD_INVALID_ID = -1, // Invalid/Unknown mod
		MOD_BASE = 0, // Generic mod
		MOD_TF2, // Team Fortress 2
		MOD_CSS, // Counter-Strike: Source
		MOD_DODS, // Day of Defeat: Source
		MOD_HL2DM, // Half-Life 2: Deathmatch
		MOD_HL1DM, // Half-Life Deathmatch Source
		MOD_BLACKMESA, // Black Mesa Deathmatch
		MOD_ZPS, // Zombie Panic Source

		LAST_MOD_TYPE
	};
}


#endif // !NAVBOT_GAME_MODS_SHARED_H_