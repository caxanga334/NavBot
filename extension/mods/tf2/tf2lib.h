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
	bool IsPlayerInvulnerable(int player);
	bool IsPlayerRevealed(int player); // is player in an effect that reveals invisible players?
	bool IsPlayerInvisible(int player);
	TeamFortress2::TFClassType GetPlayerClassType(int player);
	int GetClassDefaultMaxHealth(TeamFortress2::TFClassType tfclass);
	int GetPlayerMaxHealth(int player);
	TeamFortress2::TFClassType GetClassTypeFromName(std::string name);
	const char* GetClassNameFromType(TeamFortress2::TFClassType type);
	int GetWeaponItemDefinitionIndex(edict_t* weapon);
	TeamFortress2::TFTeam GetEntityTFTeam(int entity);
	int GetNumberOfPlayersAsClass(TeamFortress2::TFClassType tfclass, TeamFortress2::TFTeam team = TeamFortress2::TFTeam::TFTeam_Unassigned, const bool ignore_bots = false);
	TeamFortress2::TFTeam GetEnemyTFTeam(TeamFortress2::TFTeam team);
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

