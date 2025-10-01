#ifndef __NAVBOT_UTIL_GAMEDATA_CONSTS_H_
#define __NAVBOT_UTIL_GAMEDATA_CONSTS_H_

#include <string>

/* Global gamedata constants */

class GamedataConstants
{
public:
	// sizeof(COutputEvent)
	inline static std::size_t s_sizeof_COutputEvent{ 0U };
	// Name of the bone bots should aim at when going for headshots
	inline static std::string s_head_aim_bone{};

	/**
	 * @brief Initialize gamedata during the load process.
	 * @param gamedata Gameconfig pointer to NavBot's gamedata file.
	 * @return False if the extension should fail to load.
	 */
	static bool Initialize(SourceMod::IGameConfig* gamedata);
	/**
	 * @brief Initialize during map load for entity access.
	 * @param gamedata
	 * @param force
	 */
	static void OnMapStart(SourceMod::IGameConfig* gamedata, const bool force = false);
};

#endif // !__NAVBOT_UTIL_GAMEDATA_CONSTS_H_
