#include <extension.h>
#include <util/helpers.h>
#include <util/entprops.h>
#include <mods/tf2/teamfortress2mod.h>
#include "tf2lib.h"

bool tf2lib::IsPlayerInCondition(int player, TeamFortress2::TFCond cond)
{
	int iCond = static_cast<int>(cond);
	int value = 0;

	switch (iCond / 32)
	{
	case 0:
	{
		int bit = 1 << iCond;
		entprops->GetEntProp(player, Prop_Send, "m_nPlayerCond", value);
		if ((value & bit) == bit)
		{
			return true;
		}

		entprops->GetEntProp(player, Prop_Send, "_condition_bits", value);
		if ((value & bit) == bit)
		{
			return true;
		}
		break;
	}
	case 1:
	{
		int bit = (1 << (iCond - 32));
		entprops->GetEntProp(player, Prop_Send, "m_nPlayerCondEx", value);
		if ((value & bit) == bit)
		{
			return true;
		}
		break;
	}
	case 2:
	{
		int bit = (1 << (iCond - 64));
		entprops->GetEntProp(player, Prop_Send, "m_nPlayerCondEx2", value);
		if ((value & bit) == bit)
		{
			return true;
		}
		break;
	}
	case 3:
	{
		int bit = (1 << (iCond - 96));
		entprops->GetEntProp(player, Prop_Send, "m_nPlayerCondEx3", value);
		if ((value & bit) == bit)
		{
			return true;
		}
		break;
	}
	case 4:
	{
		int bit = (1 << (iCond - 128));
		entprops->GetEntProp(player, Prop_Send, "m_nPlayerCondEx4", value);
		if ((value & bit) == bit)
		{
			return true;
		}
		break;
	}
	default:
	{
		return false;
		break;
	}
	}

	return false;
}

bool tf2lib::IsPlayerInvulnerable(int player)
{
	if (IsPlayerInCondition(player, TeamFortress2::TFCond_Ubercharged) ||
		IsPlayerInCondition(player, TeamFortress2::TFCond_UberchargedHidden) ||
		IsPlayerInCondition(player, TeamFortress2::TFCond_UberchargedCanteen))
	{
		return true;
	}

	return false;
}

bool tf2lib::IsPlayerRevealed(int player)
{
	if (IsPlayerInCondition(player, TeamFortress2::TFCond_CloakFlicker) ||
		IsPlayerInCondition(player, TeamFortress2::TFCond_OnFire) ||
		IsPlayerInCondition(player, TeamFortress2::TFCond_Bleeding) ||
		IsPlayerInCondition(player, TeamFortress2::TFCond_Milked) ||
		IsPlayerInCondition(player, TeamFortress2::TFCond_Gas) ||
		IsPlayerInCondition(player, TeamFortress2::TFCond_Jarated))
	{
		return true;
	}

	return false;
}

bool tf2lib::IsPlayerInvisible(int player)
{
	if (IsPlayerRevealed(player))
		return false;

	if (IsPlayerInCondition(player, TeamFortress2::TFCond_Cloaked) ||
		IsPlayerInCondition(player, TeamFortress2::TFCond_StealthedUserBuffFade) ||
		IsPlayerInCondition(player, TeamFortress2::TFCond_Stealthed))
	{
		return true;
	}

	return false;
}

TeamFortress2::TFClassType tf2lib::GetPlayerClassType(int player)
{
	int iclass = 0;
	entprops->GetEntProp(player, Prop_Send, "m_iClass", iclass);
	return static_cast<TeamFortress2::TFClassType>(iclass);
}

/**
 * @brief Returns the current maximum amount of health that a player can have.
 * This includes modifiers due to attributes.
 * 
 * The value may be inaccurate if the value was changed recently; updates are not instant.
 *       If you need instant changes, it'd be better to call the game's
 *       `CTFPlayerShared::GetMaxBuffedHealth()` function directly.
 * 
 * @note See https://github.com/nosoop/stocksoup/blob/master/tf/player.inc#L62
 * @param player Player entity index
 * @return Integer value of player's current maximum health.
*/
int tf2lib::GetPlayerMaxHealth(int player)
{
	auto mod = CTeamFortress2Mod::GetTF2Mod();
	auto resent = mod->GetPlayerResourceEntity();
	int maxhealth = GetClassDefaultMaxHealth(GetPlayerClassType(player));
	
	if (!resent.has_value())
	{
		return maxhealth;
	}

	entprops->GetEntProp(resent.value(), Prop_Send, "m_iMaxHealth", maxhealth, 4, player);

	return maxhealth;
}

TeamFortress2::TFClassType tf2lib::GetClassTypeFromName(std::string name)
{
	std::unordered_map<std::string, TeamFortress2::TFClassType> classmap =
	{
		{"scout", TeamFortress2::TFClass_Scout},
		{"soldier", TeamFortress2::TFClass_Soldier},
		{"pyro", TeamFortress2::TFClass_Pyro},
		{"demoman", TeamFortress2::TFClass_DemoMan},
		{"demo", TeamFortress2::TFClass_DemoMan},
		{"heavyweapons", TeamFortress2::TFClass_Heavy},
		{"heavyweap", TeamFortress2::TFClass_Heavy},
		{"heavy", TeamFortress2::TFClass_Heavy},
		{"hwg", TeamFortress2::TFClass_Heavy},
		{"engineer", TeamFortress2::TFClass_Engineer},
		{"engy", TeamFortress2::TFClass_Engineer},
		{"medic", TeamFortress2::TFClass_Medic},
		{"sniper", TeamFortress2::TFClass_Sniper},
		{"spy", TeamFortress2::TFClass_Spy},
	};

	auto it = classmap.find(name);

	if (it == classmap.end())
	{
		return TeamFortress2::TFClass_Unknown;
	}

	return it->second;
}

int tf2lib::GetWeaponItemDefinitionIndex(edict_t* weapon)
{
	int entity = gamehelpers->IndexOfEdict(weapon);
	int index = -1;
	entprops->GetEntProp(entity, Prop_Send, "m_iItemDefinitionIndex", index);
	return index;
}

TeamFortress2::TFTeam tf2lib::GetEntityTFTeam(int entity)
{
	int team = 0;
	entprops->GetEntProp(entity, Prop_Send, "m_iTeamNum", team);
	return static_cast<TeamFortress2::TFTeam>(team);
}

int tf2lib::GetNumberOfPlayersAsClass(TeamFortress2::TFClassType tfclass, TeamFortress2::TFTeam team, const bool ignore_bots)
{
	int count = 0;

	for (int client = 1; client <= gpGlobals->maxClients; client++)
	{
		auto player = playerhelpers->GetGamePlayer(client);

		if (!player)
			continue;

		if (!player->IsInGame()) // must be in-game
			continue;

		if (player->IsReplay() || player->IsSourceTV())
			continue; // always ignore STV bots

		if (ignore_bots && player->IsFakeClient())
			continue;

		if (team > TeamFortress2::TFTeam::TFTeam_Spectator)
		{
			if (team != GetEntityTFTeam(client))
				continue;
		}

		if (GetPlayerClassType(client) == tfclass)
		{
			count++;
		}
	}

	return count;
}

float tf2lib::GetPlayerHealthPercentage(int player)
{
	return static_cast<float>(UtilHelpers::GetEntityHealth(player)) / static_cast<float>(GetPlayerMaxHealth(player));
}

TeamFortress2::TFTeam tf2lib::GetDisguiseTeam(int player)
{
	int team = TEAM_UNASSIGNED;
	entprops->GetEntProp(player, Prop_Send, "m_nDisguiseTeam", team);
	return static_cast<TeamFortress2::TFTeam>(team);
}

TeamFortress2::TFClassType tf2lib::GetDisguiseClass(int player)
{
	int classtype = 0;
	entprops->GetEntProp(player, Prop_Send, "m_nDisguiseClass", classtype);
	return static_cast<TeamFortress2::TFClassType>(classtype);
}

edict_t* tf2lib::GetDisguiseTarget(int player)
{
	int target = INVALID_EHANDLE_INDEX;
	entprops->GetEntPropEnt(player, Prop_Send, "m_hDisguiseTarget", target);
	return gamehelpers->EdictOfIndex(target);
}

float tf2lib::GetMedigunUberchargePercentage(int medigun)
{
	float ubercharge = 0.0f;
	entprops->GetEntPropFloat(medigun, Prop_Send, "m_flChargeLevel", ubercharge);
	return ubercharge;
}
