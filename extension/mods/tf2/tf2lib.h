#ifndef NAVBOT_TF2LIB_H_
#define NAVBOT_TF2LIB_H_
#pragma once

#include <unordered_map>
#include <string>

#include "teamfortress2_shareddefs.h"

struct edict_t;

namespace tf2lib
{
	bool IsPlayerInCondition(int player, TeamFortress2::TFCond cond);
	bool IsPlayerInCondition(CBaseEntity* player, TeamFortress2::TFCond cond);
	bool IsPlayerInvulnerable(int player);
	bool IsPlayerRevealed(CBaseEntity* player); // is player in an effect that reveals invisible players?
	bool IsPlayerInvisible(CBaseEntity* player);
	inline bool IsPlayerDisguised(int player) { return IsPlayerInCondition(player, TeamFortress2::TFCond_Disguised); }
	inline bool IsPlayerDisguised(CBaseEntity* player) { return IsPlayerInCondition(player, TeamFortress2::TFCond_Disguised); }
	TeamFortress2::TFClassType GetPlayerClassType(int player);
	int GetClassDefaultMaxHealth(TeamFortress2::TFClassType tfclass);
	int GetPlayerMaxHealth(int player);
	TeamFortress2::TFClassType GetClassTypeFromName(std::string name);
	const char* GetClassNameFromType(TeamFortress2::TFClassType type);
	int GetWeaponItemDefinitionIndex(edict_t* weapon);
	TeamFortress2::TFTeam GetEntityTFTeam(int entity);
	TeamFortress2::TFTeam GetEntityTFTeam(CBaseEntity* entity);
	int GetNumberOfPlayersAsClass(TeamFortress2::TFClassType tfclass, TeamFortress2::TFTeam team = TeamFortress2::TFTeam::TFTeam_Unassigned, const bool ignore_bots = false);
	TeamFortress2::TFTeam GetEnemyTFTeam(TeamFortress2::TFTeam team);
	/**
	 * @brief Gets the player health as a percentage.
	 * @param player Player to get the health from.
	 * @return Player health percentage. Ranges from 0.0 to 1.0. Values higher than 1.0 means the player is overhealed.
	 */
	float GetPlayerHealthPercentage(int player);
	TeamFortress2::TFTeam GetDisguiseTeam(int player);
	TeamFortress2::TFClassType GetDisguiseClass(int player);
	edict_t* GetDisguiseTarget(int player);
	float GetMedigunUberchargePercentage(int medigun);
	bool MVM_ShouldBotsReadyUp();
	// Given a building, checks if it needs to be repaired.
	bool BuildingNeedsToBeRepaired(CBaseEntity* entity);
	// returns true if a building has already been placed
	bool IsBuildingPlaced(CBaseEntity* entity);
	CBaseEntity* GetBuildingBuilder(CBaseEntity* entity);
	bool IsBuildingAtMaxUpgradeLevel(CBaseEntity* entity);
	bool IsBuildingSapped(CBaseEntity* entity);
	CBaseEntity* GetFirstValidSpawnPointForTeam(TeamFortress2::TFTeam team);
}

namespace tf2lib::mvm
{
	// returns the most dangerous in mvm
	CBaseEntity* GetMostDangerousFlag(bool ignoreDropped = false);
	CBaseEntity* GetMostDangerousTank();
}

inline int tf2lib::GetClassDefaultMaxHealth(TeamFortress2::TFClassType tfclass)
{
	switch (tfclass)
	{
	case TeamFortress2::TFClass_Scout:
	case TeamFortress2::TFClass_Sniper:
		return 125;
	case TeamFortress2::TFClass_Soldier:
		return 200;
	case TeamFortress2::TFClass_DemoMan:
		return 175;
	case TeamFortress2::TFClass_Medic:
		return 150;
	case TeamFortress2::TFClass_Heavy:
		return 300;
	case TeamFortress2::TFClass_Pyro:
		return 175;
	case TeamFortress2::TFClass_Spy:
	case TeamFortress2::TFClass_Engineer:
		return 125;
	default:
		return 125;
	}
}

inline const char* tf2lib::GetClassNameFromType(TeamFortress2::TFClassType type)
{
	switch (type)
	{
	case TeamFortress2::TFClass_Scout:
		return "scout";
	case TeamFortress2::TFClass_Sniper:
		return "sniper";
	case TeamFortress2::TFClass_Soldier:
		return "soldier";
	case TeamFortress2::TFClass_DemoMan:
		return "demoman";
	case TeamFortress2::TFClass_Medic:
		return "medic";
	case TeamFortress2::TFClass_Heavy:
		return "heavyweapons";
	case TeamFortress2::TFClass_Pyro:
		return "pyro";
	case TeamFortress2::TFClass_Spy:
		return "spy";
	case TeamFortress2::TFClass_Engineer:
		return "engineer";
	default:
		return "unknown";
	}
}

inline TeamFortress2::TFTeam tf2lib::GetEnemyTFTeam(TeamFortress2::TFTeam team)
{
	switch (team)
	{
	case TeamFortress2::TFTeam_Red:
		return TeamFortress2::TFTeam::TFTeam_Blue;
	case TeamFortress2::TFTeam_Blue:
		return TeamFortress2::TFTeam::TFTeam_Red;
	default:
		return TeamFortress2::TFTeam::TFTeam_Unassigned;
	}
}

#endif // !NAVBOT_TF2LIB_H_

