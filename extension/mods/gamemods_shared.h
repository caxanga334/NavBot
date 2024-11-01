#ifndef SMNAV_GAME_MODS_SHARED_H_
#define SMNAV_GAME_MODS_SHARED_H_
#pragma once

// Shared data between all mods

namespace Mods
{
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

		LAST_MOD_TYPE
	};
}


#endif // !SMNAV_GAME_MODS_SHARED_H_