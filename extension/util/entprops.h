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

// Indicates an invalid entity reference/index. Better compat between 32 bits integers and 64 bits integers than INVALID_EHANDLE_INDEX
#define INVALID_ENT_REFERENCE -1

class CStudioHdr;

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
	CEntPropUtils();

	enum CacheIndex : int
	{
		CTFPLAYER_PLAYERCOND, // m_nPlayerCond
		CTFPLAYER_PLAYERCONDBITS, // _condition_bits
		CTFPLAYER_PLAYERCONDEX1, // m_nPlayerCondEx
		CTFPLAYER_PLAYERCONDEX2, // m_nPlayerCondEx2
		CTFPLAYER_PLAYERCONDEX3, // m_nPlayerCondEx3
		CTFPLAYER_PLAYERCONDEX4, // m_nPlayerCondEx4
		CTFPLAYER_CLASSTYPE, // m_iClass
		CBASECOMBATCHARACTER_ACTIVEWEAPON, // m_hActiveWeapon
		CBASECOMBATCHARACTER_MYWEAPONS, // m_hMyWeapons
		CBASECOMBATWEAPON_CLIP1, // m_iClip1
		CBASECOMBATWEAPON_CLIP2, // m_iClip2
		CBASECOMBATWEAPON_PRIMARYAMMOTYPE, // m_iPrimaryAmmoType
		CBASECOMBATWEAPON_SECONDARYAMMOTYPE, // m_iSecondaryAmmoType
		CBASECOMBATWEAPON_STATE, // m_iState
		CBASECOMBATWEAPON_OWNER, // m_hOwner
		CBASEPLAYER_WATERLEVEL, // m_nWaterLevel
		CDODPLAYER_PLAYERCLASS, // CDODPlayer::m_iPlayerClass

		CACHEINDEX_SIZE
	};

	void Init(bool reset = false);

	// The functions below are a little bit slow for constant calls.
	// If there is a variable that you need to read frequently (IE: on a think/game frame hook), you have to cache the offset
	// Note: SM does have it's own cache however the profiler (VS) reported some high self CPU time on Sourcemod's functions related to the offset search.
	// It's faster to just cache locally.

	bool HasEntProp(int entity, PropType proptype, const char* prop, unsigned int* offset = nullptr, const bool logerror = false);
	bool HasEntProp(edict_t* entity, PropType proptype, const char* prop, unsigned int* offset = nullptr, const bool logerror = false);
	bool HasEntProp(CBaseEntity* entity, PropType proptype, const char* prop, unsigned int* offset = nullptr, const bool logerror = false);
	bool GetEntProp(int entity, PropType proptype, const char *prop, int &result, int size = 4, int element = 0);
	bool GetEntProp(edict_t* entity, PropType proptype, const char* prop, int& result, int size = 4, int element = 0);
	bool GetEntProp(CBaseEntity* entity, PropType proptype, const char* prop, int& result, int size = 4, int element = 0);
	bool GetEntPropBool(int entity, PropType proptype, const char *prop, bool& result, int element = 0);
	bool GetEntPropBool(edict_t* entity, PropType proptype, const char* prop, bool& result, int element = 0);
	bool GetEntPropBool(CBaseEntity* entity, PropType proptype, const char* prop, bool& result, int element = 0);
	bool SetEntProp(int entity, PropType proptype, const char *prop, int value, int size = 4, int element = 0);
	bool GetEntPropFloat(int entity, PropType proptype, const char *prop, float &result, int element = 0);
	bool GetEntPropFloat(edict_t* entity, PropType proptype, const char* prop, float& result, int element = 0);
	bool GetEntPropFloat(CBaseEntity* entity, PropType proptype, const char* prop, float& result, int element = 0);
	bool SetEntPropFloat(int entity, PropType proptype, const char *prop, float value, int element = 0);
	bool GetEntPropEnt(int entity, PropType proptype, const char *prop, int* result, CBaseEntity** pOut = nullptr, int element = 0);
	bool GetEntPropEnt(edict_t* entity, PropType proptype, const char* prop, int* result, CBaseEntity** pOut = nullptr, int element = 0);
	bool GetEntPropEnt(CBaseEntity* entity, PropType proptype, const char* prop, int* result, CBaseEntity** pOut = nullptr, int element = 0);
	bool SetEntPropEnt(int entity, PropType proptype, const char *prop, int other, int element = 0);
	bool GetEntPropVector(int entity, PropType proptype, const char *prop, Vector &result, int element = 0);
	bool GetEntPropVector(edict_t* entity, PropType proptype, const char* prop, Vector& result, int element = 0);
	bool GetEntPropVector(CBaseEntity* entity, PropType proptype, const char* prop, Vector& result, int element = 0);
	bool SetEntPropVector(int entity, PropType proptype, const char *prop, Vector value, int element = 0);
	bool GetEntPropString(int entity, PropType proptype, const char* prop, char* result, int maxlen, size_t& len, int element = 0);
	bool GetEntPropString(edict_t* entity, PropType proptype, const char* prop, char* result, int maxlen, size_t& len, int element = 0);
	bool GetEntPropString(CBaseEntity* entity, PropType proptype, const char* prop, char* result, int maxlen, size_t& len, int element = 0);
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
	/**
	 * @brief Look ups the array size of an entity property.
	 * @param entity Entity index.
	 * @param proptype Property type.
	 * @param prop Property name.
	 * @return Array size or 0 if not an array or if the look up failed.
	 */
	std::size_t GetEntPropArraySize(int entity, PropType proptype, const char* prop);
	bool GameRules_GetProp(const char *prop, int &result, int size = 4, int element = 0);
	bool GameRules_GetPropBool(const char* prop, bool& result, int element = 0);
	bool GameRules_GetPropFloat(const char *prop, float &result, int element = 0);
	bool GameRules_GetPropEnt(const char *prop, int &result, int element = 0);
	bool GameRules_GetPropVector(const char *prop, Vector &result, int element = 0);
	bool GameRules_GetPropString(const char *prop, char* result, size_t& len, int maxlen, int element = 0);
	RoundState GameRules_GetRoundState();

	/**
	 * @brief Gets the address to an entity property. Very raw and very unsafe.
	 * @tparam T Variable type to cast to.
	 * @param entity Entity that contains the property.
	 * @param proptype Property type.
	 * @param prop Property name.
	 * @return Pointer to property or NULL on failure.
	 * @note Does not check for bit count for integer types, make sure you know what you are doing!
	 */
	template <typename T>
	T* GetPointerToEntData(CBaseEntity* entity, PropType proptype, const char* prop);

	/**
	 * @brief Gets the address to an entity property at the given offset. No safety checks!
	 * @tparam T Variable type to cast to.
	 * @param entity Entity that contains the property
	 * @param offset Offset to the property (this + offset)
	 * @return 
	 */
	template <typename T>
	T* GetPointerToEntData(CBaseEntity* entity, unsigned int offset);

	/**
	 * @brief Gets the address to an entity property using the cached offset. Only supports SendProps (Networked Properties)
	 * @tparam T Variable type at the offset.
	 * @param entity Entity to read.
	 * @param index Cache index.
	 * @return Pointer to entity property.
	 */
	template <typename T>
	T* GetCachedDataPtr(CBaseEntity* entity, CacheIndex index);

	/**
	 * @brief Gets the value to an entity property using the cached offset. Only supports SendProps (Networked Properties)
	 * @tparam T Variable type at the offset.
	 * @param entity Entity to read.
	 * @param index Cache index.
	 * @return Value of entity property.
	 */
	template <typename T>
	T GetCachedData(CBaseEntity* entity, CacheIndex index);
private:
	bool IsNetworkedEntity(CBaseEntity *pEntity);
	edict_t *BaseEntityToEdict(CBaseEntity *pEntity);
	bool FindSendProp(SourceMod::sm_sendprop_info_t *info, CBaseEntity *pEntity, const char *prop);
	bool FindDataMap(CBaseEntity* pEntity, SourceMod::sm_datatable_info_t& dinfo, const char* prop);
	int MatchTypeDescAsInteger(_fieldtypes type, int flags);
	ServerClass* FindEntityServerClass(CBaseEntity* pEntity);
	bool IndexToAThings(int num, CBaseEntity **pEntData, edict_t **pEdictData);
	CBaseEntity *GetEntity(int entity);
	CBaseEntity *GetGameRulesProxyEntity();

	std::string grclassname; // game rules proxy net class
	bool initialized;

	std::array<unsigned int, static_cast<size_t>(CACHEINDEX_SIZE)> cached_offsets;

	void BuildCache();
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

inline ServerClass* CEntPropUtils::FindEntityServerClass(CBaseEntity* pEntity)
{
	IServerNetworkable* pNet = reinterpret_cast<IServerUnknown*>(pEntity)->GetNetworkable();

	if (pNet == nullptr)
	{
		return nullptr;
	}

	return pNet->GetServerClass();
}

/// @brief Converts a CBaseEntity to an edict_t
/// @param pEntity CBaseEntity pointer
/// @return edict_t pointer
inline edict_t *CEntPropUtils::BaseEntityToEdict(CBaseEntity *pEntity)
{
	IServerNetworkable* pNet = reinterpret_cast<IServerUnknown*>(pEntity)->GetNetworkable();

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

inline bool CEntPropUtils::GameRules_GetPropBool(const char* prop, bool& result, int element)
{
	int value = 0;
	const bool found = this->GameRules_GetProp(prop, value, 1, element);
	result = (value != 0);
	return found;
}

template<typename T>
inline T* CEntPropUtils::GetPointerToEntData(CBaseEntity* entity, PropType proptype, const char* prop)
{
	if (entity == nullptr)
	{
		return nullptr;
	}

	switch (proptype)
	{
	case Prop_Send:
	{
		ServerClass* pClass = gamehelpers->FindEntityServerClass(entity);
		SourceMod::sm_sendprop_info_t info;

		if (!pClass)
		{
			return nullptr;
		}

		if (gamehelpers->FindSendPropInfo(pClass->GetName(), prop, &info))
		{
			return (T*)((uint8_t*)entity + info.actual_offset);
		}

		break;
	}
	case Prop_Data:
	{
		datamap_t* map = gamehelpers->GetDataMap(entity);
		SourceMod::sm_datatable_info_t info;

		if (map == nullptr)
		{
			return nullptr;
		}

		if (gamehelpers->FindDataMapInfo(map, prop, &info))
		{
			return (T*)((uint8_t*)entity + info.actual_offset);
		}

		break;
	}
	default:
		break;
	}

	return nullptr;
}

template<typename T>
inline T* CEntPropUtils::GetPointerToEntData(CBaseEntity* entity, unsigned int offset)
{
	if (entity == nullptr)
	{
		return nullptr;
	}

	return (T*)((uint8_t*)entity + offset);
}

template<typename T>
inline T* CEntPropUtils::GetCachedDataPtr(CBaseEntity* entity, CacheIndex index)
{
	return (T*)((uint8_t*)entity + cached_offsets[static_cast<std::size_t>(index)]);
}

template<typename T>
inline T CEntPropUtils::GetCachedData(CBaseEntity* entity, CacheIndex index)
{
	return *(T*)((uint8_t*)entity + cached_offsets[static_cast<std::size_t>(index)]);
}

// Global singleton for accessing the Ent Prop Utils
extern CEntPropUtils *entprops;

namespace entityprops
{
	// Water level values for m_nWaterLevel, from src/game/shared/imovehelper.h
	enum WaterLevel : std::int8_t
	{
		WL_NotInWater = 0,
		WL_Feet,
		WL_Waist,
		WL_Eyes
	};

	// Gets an entity classname, offset is cached localy
	const char* GetEntityClassname(CBaseEntity* entity);
	// Gets an entity team number (m_iTeamNum) via datamaps
	int GetEntityTeamNum(CBaseEntity* entity);
	// Gets an entity life state (m_lifeState) via datamaps
	std::int8_t GetEntityLifeState(CBaseEntity* entity);
	// Gets an entity health (m_iHealth) via datamaps
	int GetEntityHealth(CBaseEntity* entity);
	std::int8_t GetEntityWaterLevel(CBaseEntity* entity);
	// Gets an entity targetname (name given by the level designer for I/O)
	const char* GetEntityTargetname(CBaseEntity* entity);
	// Gets an entity velocity
	void GetEntityAbsVelocity(CBaseEntity* entity, Vector& result);
	/**
	 * @brief Returns an entity CStudioHdr pointer.
	 * 
	 * Currently broken, do not use.
	 * @param entity Entity to get the CStudioHdr from.
	 * @param validate If true, validates that the given entity derives from CBaseAnimating (Where the CStudioHdr *m_pStudioHdr variable is)
	 * @return Entity model pointer.
	 */
	CStudioHdr* GetEntityModelPtr(CBaseEntity* entity, const bool validate = true);
	// Checks if the given effect (See EF_ enum) is active on the entity.
	bool IsEffectActiveOnEntity(CBaseEntity* entity, int effects);
}

#endif
