#ifndef __NAVBOT_UTIL_ENTPROP_CONSTS_H_
#define __NAVBOT_UTIL_ENTPROP_CONSTS_H_

// common constants for entity properties stuff

// Values for m_toggle_state property
enum TOGGLE_STATE
{
	TS_AT_TOP,
	TS_AT_BOTTOM,
	TS_GOING_UP,
	TS_GOING_DOWN
};

enum PropType
{
	Prop_Send = 0,
	Prop_Data
};

/**
 * @brief Round states for CTeamplayRoundBasedRules based gamerules.
 * See teamplayroundbased_gamerules.h in the Source SDK 2013
 */
enum RoundState
{
	// initialize the game, create teams
	RoundState_Init,
	//Before players have joined the game. Periodically checks to see if enough players are ready
	//to start a game. Also reverts to this when there are no active players
	RoundState_Pregame,
	//The game is about to start, wait a bit and spawn everyone
	RoundState_StartGame,
	//All players are respawned, frozen in place
	RoundState_Preround,
	//Round is on, playing normally
	RoundState_RoundRunning,
	//Someone has won the round
	RoundState_TeamWin,
	//Noone has won, manually restart the game, reset scores
	RoundState_Restart,
	//Noone has won, restart the game
	RoundState_Stalemate,
	//Game is over, showing the scoreboard etc
	RoundState_GameOver,
	//Game is over, doing bonus round stuff
	RoundState_Bonus,
	//Between rounds
	RoundState_BetweenRounds,
};

#endif // !__NAVBOT_UTIL_ENTPROP_CONSTS_H_
