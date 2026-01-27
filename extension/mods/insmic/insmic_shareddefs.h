#ifndef __NAVBOT_INSMIC_SHAREDDEFS_H_
#define __NAVBOT_INSMIC_SHAREDDEFS_H_

namespace insmic
{
	constexpr int UNLIMITED_SUPPLIES = -1;
	constexpr int DISABLED_SQUAD = -1;
	constexpr int MAX_SQUAD_SLOTS = 8;
	constexpr int INVALID_SLOT = -1;
	constexpr int DEFAULT_SLOT = 0;

	enum InsMICTeam
	{
		INSMICTEAM_UNASSINGED = 0,
		INSMICTEAM_USMC,
		INSMICTEAM_INSURGENTS,
		INSMICTEAM_SPECTATOR,
	};

	constexpr int TEAM_NEUTRAL = 0;

	enum InsMICSquads_t
	{
		INSMIC_INVALID_SQUAD = DISABLED_SQUAD,
		INSMIC_SQUAD_ONE = 0,
		INSMIC_SQUAD_TWO,
		INSMIC_MAX_SQUADS
	};

	enum InsMICTeamSelect_t
	{
		TEAMSELECT_INVALID = -1,
		TEAMSELECT_ONE = 0,
		TEAMSELECT_TWO,
		TEAMSELECT_AUTOASSIGN,
		TEAMSELECT_SPECTATOR,
		TEAMSELECT_COUNT
	};

	enum InsMICGameTypes_t
	{
		GAMETYPE_INVALID = -1,
		GAMETYPE_BATTLE = 0,
		GAMETYPE_FIREFIGHT,
		GAMETYPE_PUSH,
		GAMETYPE_STRIKE,
		GAMETYPE_TDM,
		GAMETYPE_POWERBALL,

		MAX_GAMETYPES
	};

	enum Stance_t
	{
		STANCE_INVALID = -1,
		STANCE_STAND = 0,
		STANCE_CROUCH,
		STANCE_PRONE,
		STANCE_COUNT
	};

	struct InsMICEncodedSquadInfoData_t
	{
		unsigned squad : 2;
		unsigned slot : 4;
	};

	// bit flags for CINSPlayer::m_iPlayerFlags
	constexpr int PLAYERFLAG_AIMING = 0x1; // player is aiming/using their weapon sights
	constexpr int PLAYERFLAG_BIPOD = 0x2; // player has deployed their bipod
	constexpr int PLAYERFLAG_SPRINTING = 0x4; // player is sprinting
	constexpr int PLAYERFLAG_WALKING = 0x8; // player is walking
}

#endif // !__NAVBOT_INSMIC_MOD_SHAREDDEFS_H_

