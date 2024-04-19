/**
 * vim: set ts=4 :
 * =============================================================================
 * Copyright (C) 2004-2010 AlliedModders LLC.  All rights reserved.
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, AlliedModders LLC gives you permission to link the
 * code of this program (as well as its derivative works) to "Half-Life 2," the
 * "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
 * by the Valve Corporation.  You must obey the GNU General Public License in
 * all respects for all other code used.  Additionally, AlliedModders LLC grants
 * this exception to all derivative works.  AlliedModders LLC defines further
 * exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
 * or <http://www.sourcemod.net/license.php>.
 *
 */
#ifndef SMNAV_UTIL_ENT_PROPS_H_
#define SMNAV_UTIL_ENT_PROPS_H_
#pragma once

#include <extension.h>
#include <dt_send.h>
#include <server_class.h>

// Indicates an invalid entity reference/index. Better compat between 32 bits integers and 64 bits integers than INVALID_EHANDLE_INDEX
#define INVALID_ENT_REFERENCE -1

enum PropType
{
	Prop_Send = 0,
	Prop_Data
};

// from sourcemod
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

/**
 * @brief Utility class for reading/writing to entity properties and datamaps
*/
class CEntPropUtils
{
public:
	void Init(bool reset = false);
	bool GetEntProp(int entity, PropType proptype, const char *prop, int &result, int size = 4, int element = 0);
	bool GetEntPropBool(int entity, PropType proptype, const char *prop, bool& result, int element = 0);
	bool SetEntProp(int entity, PropType proptype, const char *prop, int value, int size = 4, int element = 0);
	bool GetEntPropFloat(int entity, PropType proptype, const char *prop, float &result, int element = 0);
	bool SetEntPropFloat(int entity, PropType proptype, const char *prop, float value, int element = 0);
	bool GetEntPropEnt(int entity, PropType proptype, const char *prop, int &result, int element = 0);
	bool SetEntPropEnt(int entity, PropType proptype, const char *prop, int other, int element = 0);
	bool GetEntPropVector(int entity, PropType proptype, const char *prop, Vector &result, int element = 0);
	bool SetEntPropVector(int entity, PropType proptype, const char *prop, Vector value, int element = 0);
	bool GetEntPropString(int entity, PropType proptype, const char* prop, char* result, int maxlen, size_t& len, int element = 0);
	bool SetEntPropString(int entity, PropType proptype, const char *prop, char *value, int element = 0);
	bool GetEntData(int entity, int offset, int &result, int size = 4);
	bool SetEntData(int entity, int offset, int value, int size = 4, bool changeState = false);
	bool GetEntDataFloat(int entity, int offset, float &result);
	bool SetEntDataFloat(int entity, int offset, float value, bool changeState = false);
	bool GetEntDataEnt(int entity, int offset, int &result);
	bool SetEntDataEnt(int entity, int offset, int value, bool changeState = false);
	bool GetEntDataVector(int entity, int offset, Vector &result);
	bool SetEntDataVector(int entity, int offset, Vector value, bool changeState = false);
	bool GetEntDataString(int entity, int offset, int maxlen, char* result, size_t &len);
	bool SetEntDataString(int entity, int offset, char *value, int maxlen, bool changeState = false);
	bool GameRules_GetProp(const char *prop, int &result, int size = 4, int element = 0);
	bool GameRules_GetPropFloat(const char *prop, float &result, int element = 0);
	bool GameRules_GetPropEnt(const char *prop, int &result, int element = 0);
	bool GameRules_GetPropVector(const char *prop, Vector &result, int element = 0);
	bool GameRules_GetPropString(const char *prop, char* result, size_t& len, int maxlen, int element = 0);
	RoundState GameRules_GetRoundState();

private:
	bool IsNetworkedEntity(CBaseEntity *pEntity);
	edict_t *BaseEntityToEdict(CBaseEntity *pEntity);
	bool FindSendProp(SourceMod::sm_sendprop_info_t *info, CBaseEntity *pEntity, const char *prop, int entity);
	int MatchTypeDescAsInteger(_fieldtypes type, int flags);
	bool IndexToAThings(int num, CBaseEntity **pEntData, edict_t **pEdictData);
	CBaseEntity *GetEntity(int entity);
	CBaseEntity *GetGameRulesProxyEntity();

	const char *grclassname; // game rules proxy net class
	bool initialized;
};

inline int CEntPropUtils::MatchTypeDescAsInteger(_fieldtypes type, int flags)
{
	switch (type)
	{
	case FIELD_TICK:
	case FIELD_MODELINDEX:
	case FIELD_MATERIALINDEX:
	case FIELD_INTEGER:
	case FIELD_COLOR32:
		return 32;
	case FIELD_CUSTOM:
		if ((flags & FTYPEDESC_OUTPUT) == FTYPEDESC_OUTPUT)
		{
			// Variant, read as int32.
			return 32;
		}
		break;
	case FIELD_SHORT:
		return 16;
	case FIELD_CHARACTER:
		return 8;
	case FIELD_BOOLEAN:
		return 1;
	default:
		return 0;
	}

	return 0;
}

/// @brief Converts a CBaseEntity to an edict_t
/// @param pEntity CBaseEntity pointer
/// @return edict_t pointer
inline edict_t *CEntPropUtils::BaseEntityToEdict(CBaseEntity *pEntity)
{
	IServerUnknown *pUnk = (IServerUnknown *)pEntity;
	IServerNetworkable *pNet = pUnk->GetNetworkable();

	if (!pNet)
	{
		return nullptr;
	}

	return pNet->GetEdict();
}

/// @brief Gets a CBaseEntity from an entity index
/// @param entity Entity/Edict index
/// @return CBaseEntity pointer
inline CBaseEntity *CEntPropUtils::GetEntity(int entity)
{
	CBaseEntity *pEntity;
	if (!IndexToAThings(entity, &pEntity, nullptr))
	{
		return nullptr;
	}

	return pEntity;
}

// Global singleton for accessing the Ent Prop Utils
extern CEntPropUtils *entprops;

#endif