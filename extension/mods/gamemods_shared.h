#ifndef NAVBOT_GAME_MODS_SHARED_H_
#define NAVBOT_GAME_MODS_SHARED_H_
#pragma once

// Shared data between all mods

#include <array>
#include <string_view>
#include <cstring>

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
		MOD_INSMIC, // Insurgency: Modern Infantry Combat

		LAST_MOD_TYPE
	};
	/**
	 * @brief Converts a string to a ModType enum value.
	 * @param str String to convert.
	 * @return a ModType enum value. Returns ModType::MOD_INVALID_ID if the string could not be converted.
	 */
	inline ModType GetModTypeFromString(const char* str)
	{
		using namespace std::literals::string_view_literals;

		// the array index must match the enum order
		constexpr std::array names = {
			"MOD_BASE"sv,
			"MOD_TF2"sv,
			"MOD_CSS"sv,
			"MOD_DODS"sv,
			"MOD_HL2DM"sv,
			"MOD_HL1DM"sv,
			"MOD_BLACKMESA"sv,
			"MOD_ZPS"sv,
			"MOD_INSMIC"sv
		};

		static_assert(names.size() == static_cast<std::size_t>(ModType::LAST_MOD_TYPE), "ModType Enum and names array size doesn't match!");

		for (int i = 0; i < static_cast<int>(names.size()); i++)
		{
			if (std::strcmp(str, names[i].data()) == 0)
			{
				return static_cast<ModType>(i);
			}
		}

		return MOD_INVALID_ID;
	}

	enum MapNameType
	{
		MAPNAME_RAW = 0, // Raw map name without any modifications.
		MAPNAME_CLEAN, // Clean map name without workshop stuff
		MAPNAME_UNIQUE, // Clean and unique map name with a workshop ID
	};

	inline MapNameType InvertMapNameType(MapNameType in)
	{
		switch (in)
		{
		case MapNameType::MAPNAME_CLEAN:
			return MapNameType::MAPNAME_UNIQUE;
		case MapNameType::MAPNAME_UNIQUE:
			return MapNameType::MAPNAME_CLEAN;
		default:
			return MapNameType::MAPNAME_CLEAN;
		}
	}
}


#endif // !NAVBOT_GAME_MODS_SHARED_H_