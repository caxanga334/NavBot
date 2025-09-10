#ifndef __NAVBOT_DODS_SHAREDDEFS_H_
#define __NAVBOT_DODS_SHAREDDEFS_H_

#if SOURCE_ENGINE == SE_EPISODEONE

//-----------------------------------------------------------------------------
// Multiplayer specific defines
//-----------------------------------------------------------------------------
#define MAX_CONTROL_POINTS			8
#define MAX_CONTROL_POINT_GROUPS	8

// Maximum number of points that a control point may need owned to be cappable
#define MAX_PREVIOUS_POINTS			3

// The maximum number of teams the control point system knows how to deal with
#define MAX_CONTROL_POINT_TEAMS		8

// Maximum length of the cap layout string
#define MAX_CAPLAYOUT_LENGTH		32

// Maximum length of the current round printname
#define MAX_ROUND_NAME				32

// Maximum length of the current round name
#define MAX_ROUND_IMAGE_NAME		64

// Score added to the team score for a round win
#define TEAMPLAY_ROUND_WIN_SCORE	1

#endif // SOURCE_ENGINE == SE_EPISODEONE

namespace dayofdefeatsource
{
	enum DoDTeam : int
	{
		DODTEAM_UNASSIGNED = 0,
		DODTEAM_SPECTATOR,
		DODTEAM_ALLIES,
		DODTEAM_AXIS
	};

	enum DoDClassType : int
	{
		DODCLASS_INVALID = -1,
		DODCLASS_RIFLEMAN = 0,
		DODCLASS_ASSAULT,
		DODCLASS_SUPPORT,
		DODCLASS_SNIPER,
		DODCLASS_MACHINEGUNNER,
		DODCLASS_ROCKET,

		NUM_DOD_CLASSES
	};

	enum DoDBombTargetState : int
	{
		BOMB_TARGET_INACTIVE = 0, // disabled and invisible
		BOMB_TARGET_ACTIVE, // visible, bomb can be planted here
		BOMB_TARGET_ARMED, // visible, bomb is planted and ticking down

		NUM_BOMB_TARGET_STATES
	};

	enum DoDRoundState : int
	{
		DODROUNDSTATE_INIT = 0,
		DODROUNDSTATE_PREGAME,
		DODROUNDSTATE_STARTGAME,
		DODROUNDSTATE_PREROUND,
		DODROUNDSTATE_ROUNDRUNNING,
		DODROUNDSTATE_ALLIES_WIN,
		DODROUNDSTATE_AXIS_WIN,
		DODROUNDSTATE_RESTART,
		DODROUNDSTATE_GAME_OVER,

		NUM_DOD_ROUND_STATES
	};

	constexpr int INVALID_CONTROL_POINT = -1;
	constexpr float DOD_PLAYER_STANDING_HEIGHT = 72.0f;
	constexpr float DOD_PLAYER_CROUCHING_HEIGHT = 45.0f;
	constexpr float DOD_PLAYER_PRONE_HEIGHT = 24.0f;
}

#endif // !__NAVBOT_DODS_SHAREDDEFS_H_
