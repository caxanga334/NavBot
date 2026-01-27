#ifndef __NAVBOT_UTIL_GAMEDATA_CONSTS_H_
#define __NAVBOT_UTIL_GAMEDATA_CONSTS_H_

#include <string>

/* Global gamedata constants */

class GamedataConstants
{
public:
	//
	struct UserButtons
	{
		int attack1;
		int attack2;
		int attack3;
		int forward;
		int backward;
		int moveleft;
		int moveright;
		int crouch;
		int jump;
		int speed;
		int run;
		int walk;
		int use;
		int reload;
		int zoom;
		int alt1;
		int modcustom1;
		int modcustom2;
		int modcustom3;
		int modcustom4;

		void InitFromSDK();
		void InitFromGamedata(SourceMod::IGameConfig* gamedata);
	};

	// sizeof(COutputEvent)
	inline static std::size_t s_sizeof_COutputEvent{ 0U };
	// Name of the bone bots should aim at when going for headshots
	inline static std::string s_head_aim_bone{};
	// Struct with values of user buttons to be used with CUserCmd
	inline static UserButtons s_user_buttons;

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
