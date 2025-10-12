#include NAVBOT_PCH_FILE
#include <extension.h>
#include <util/helpers.h>
#include <util/entprops.h>
#include <mods/tf2/teamfortress2mod.h>
#include <entities/tf2/tf_entities.h>
#include "tf2lib.h"

bool tf2lib::IsPlayerInCondition(int player, TeamFortress2::TFCond cond)
{
	CBaseEntity* entity = gamehelpers->ReferenceToEntity(player);
	int iCond = static_cast<int>(cond);
	int value = 0;

	switch (iCond / 32)
	{
	case 0:
	{
		int bit = 1 << iCond;
		value = entprops->GetCachedData<int>(entity, CEntPropUtils::CacheIndex::CTFPLAYER_PLAYERCOND);
		if ((value & bit) == bit)
		{
			return true;
		}

		value = entprops->GetCachedData<int>(entity, CEntPropUtils::CacheIndex::CTFPLAYER_PLAYERCONDBITS);
		if ((value & bit) == bit)
		{
			return true;
		}
		break;
	}
	case 1:
	{
		int bit = (1 << (iCond - 32));
		value = entprops->GetCachedData<int>(entity, CEntPropUtils::CacheIndex::CTFPLAYER_PLAYERCONDEX1);
		if ((value & bit) == bit)
		{
			return true;
		}
		break;
	}
	case 2:
	{
		int bit = (1 << (iCond - 64));
		value = entprops->GetCachedData<int>(entity, CEntPropUtils::CacheIndex::CTFPLAYER_PLAYERCONDEX2);
		if ((value & bit) == bit)
		{
			return true;
		}
		break;
	}
	case 3:
	{
		int bit = (1 << (iCond - 96));
		value = entprops->GetCachedData<int>(entity, CEntPropUtils::CacheIndex::CTFPLAYER_PLAYERCONDEX3);
		if ((value & bit) == bit)
		{
			return true;
		}
		break;
	}
	case 4:
	{
		int bit = (1 << (iCond - 128));
		value = entprops->GetCachedData<int>(entity, CEntPropUtils::CacheIndex::CTFPLAYER_PLAYERCONDEX4);
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

bool tf2lib::IsPlayerInCondition(CBaseEntity* player, TeamFortress2::TFCond cond)
{
	int iCond = static_cast<int>(cond);
	int value = 0;

	switch (iCond / 32)
	{
	case 0:
	{
		int bit = 1 << iCond;
		value = entprops->GetCachedData<int>(player, CEntPropUtils::CacheIndex::CTFPLAYER_PLAYERCOND);
		if ((value & bit) == bit)
		{
			return true;
		}

		value = entprops->GetCachedData<int>(player, CEntPropUtils::CacheIndex::CTFPLAYER_PLAYERCONDBITS);
		if ((value & bit) == bit)
		{
			return true;
		}
		break;
	}
	case 1:
	{
		int bit = (1 << (iCond - 32));
		value = entprops->GetCachedData<int>(player, CEntPropUtils::CacheIndex::CTFPLAYER_PLAYERCONDEX1);
		if ((value & bit) == bit)
		{
			return true;
		}
		break;
	}
	case 2:
	{
		int bit = (1 << (iCond - 64));
		value = entprops->GetCachedData<int>(player, CEntPropUtils::CacheIndex::CTFPLAYER_PLAYERCONDEX2);
		if ((value & bit) == bit)
		{
			return true;
		}
		break;
	}
	case 3:
	{
		int bit = (1 << (iCond - 96));
		value = entprops->GetCachedData<int>(player, CEntPropUtils::CacheIndex::CTFPLAYER_PLAYERCONDEX3);
		if ((value & bit) == bit)
		{
			return true;
		}
		break;
	}
	case 4:
	{
		int bit = (1 << (iCond - 128));
		value = entprops->GetCachedData<int>(player, CEntPropUtils::CacheIndex::CTFPLAYER_PLAYERCONDEX4);
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

bool tf2lib::IsPlayerRevealed(CBaseEntity* player)
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

bool tf2lib::IsPlayerInvisible(CBaseEntity* player)
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

int tf2lib::GetWeaponItemDefinitionIndex(CBaseEntity* weapon)
{
	int entity = reinterpret_cast<IServerEntity*>(weapon)->GetRefEHandle().GetEntryIndex();
	int index = -1;
	entprops->GetEntProp(entity, Prop_Send, "m_iItemDefinitionIndex", index);
	return index;
}

TeamFortress2::TFTeam tf2lib::GetEntityTFTeam(int entity)
{
	int team = 0;
	entprops->GetEntProp(entity, Prop_Data, "m_iTeamNum", team);
	return static_cast<TeamFortress2::TFTeam>(team);
}

TeamFortress2::TFTeam tf2lib::GetEntityTFTeam(CBaseEntity* entity)
{
	int* teamNum = entprops->GetPointerToEntData<int>(entity, Prop_Data, "m_iTeamNum");

	if (teamNum)
	{
		return static_cast<TeamFortress2::TFTeam>(*teamNum);
	}

	return TeamFortress2::TFTeam::TFTeam_Unassigned;
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

TeamFortress2::TFTeam tf2lib::TFTeamFromString(const char* string)
{
	if (strncasecmp(string, "red", 3) == 0 || strncasecmp(string, "defenders", 9) == 0)
	{
		return TeamFortress2::TFTeam::TFTeam_Red;
	}

	if (strncasecmp(string, "blu", 3) == 0 || strncasecmp(string, "blue", 4) == 0 || strncasecmp(string, "invaders", 8) == 0)
	{
		return TeamFortress2::TFTeam::TFTeam_Blue;
	}

	if (strncasecmp(string, "spec", 4) == 0 || strncasecmp(string, "spectator", 9) == 0)
	{
		return TeamFortress2::TFTeam::TFTeam_Spectator;
	}

	return TeamFortress2::TFTeam::TFTeam_Unassigned;
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
	entprops->GetEntPropEnt(player, Prop_Send, "m_hDisguiseTarget", &target);
	return gamehelpers->EdictOfIndex(target);
}

float tf2lib::GetMedigunUberchargePercentage(int medigun)
{
	float ubercharge = 0.0f;
	entprops->GetEntPropFloat(medigun, Prop_Send, "m_flChargeLevel", ubercharge);
	return ubercharge;
}

bool tf2lib::MVM_ShouldBotsReadyUp()
{
	bool readyup = false; // true if the bots should ready up
	bool anyhumans = false;

	auto func = [&readyup, &anyhumans](int client, edict_t* entity, SourceMod::IGamePlayer* player) {
		if (!player->IsFakeClient())
		{
			anyhumans = true;

			if (tf2lib::GetEntityTFTeam(client) == TeamFortress2::TFTeam::TFTeam_Red)
			{
				int isready = 0;
				entprops->GameRules_GetProp("m_bPlayerReady", isready, 4, client);

				if (isready != 0)
				{
					// We have at least 1 human player on RED team that is not ready
					readyup = true;
				}
			}
		}
	};

	UtilHelpers::ForEachPlayer(func);

	if (!anyhumans)
	{
		// no humans on RED team
		return true;
	}

	return readyup;
}

bool tf2lib::BuildingNeedsToBeRepaired(CBaseEntity* entity)
{
	float* percentconstructed = entprops->GetPointerToEntData<float>(entity, Prop_Send, "m_flPercentageConstructed");
	int* health = entprops->GetPointerToEntData<int>(entity, Prop_Send, "m_iHealth");
	int* maxhealth = entprops->GetPointerToEntData<int>(entity, Prop_Send, "m_iMaxHealth");

	if (health && maxhealth && percentconstructed)
	{
		if (*percentconstructed >= 0.99f && *health < *maxhealth)
		{
			return true;
		}
	}

	bool* sapper = entprops->GetPointerToEntData<bool>(entity, Prop_Send, "m_bHasSapper");

	if (sapper)
	{
		return *sapper;
	}

	return false;
}

bool tf2lib::IsBuildingPlaced(CBaseEntity* entity)
{
	bool* placing = entprops->GetPointerToEntData<bool>(entity, Prop_Send, "m_bPlacing");
	bool* carried = entprops->GetPointerToEntData<bool>(entity, Prop_Send, "m_bCarried");

	if (placing && *placing == true)
	{
		return false;
	}

	if (carried && *carried == true)
	{
		return false;
	}

	return true;
}

CBaseEntity* tf2lib::GetBuildingBuilder(CBaseEntity* entity)
{
	CHandle<CBaseEntity>* handle = entprops->GetPointerToEntData<CHandle<CBaseEntity>>(entity, Prop_Send, "m_hBuilder");

	if (handle != nullptr)
	{
		return handle->Get();
	}

	return nullptr;
}

bool tf2lib::IsBuildingAtMaxUpgradeLevel(CBaseEntity* entity)
{
	int level = 3; // default to 3 as a fail safe if look up fails
	constexpr int maxlevel = 3; // only way to get max level is via SDK Call
	int index = UtilHelpers::IndexOfEntity(entity);
	entprops->GetEntProp(index, Prop_Send, "m_iUpgradeLevel", level);
	
	bool mini = false;
	entprops->GetEntPropBool(index, Prop_Send, "m_bMiniBuilding", mini);

	if (mini)
	{
		return true;
	}

	bool disposable = false;
	entprops->GetEntPropBool(index, Prop_Send, "m_bDisposableBuilding", disposable);

	if (disposable)
	{
		return true;
	}

	return level >= maxlevel;
}

bool tf2lib::IsBuildingSapped(CBaseEntity* entity)
{
	bool* sapper = entprops->GetPointerToEntData<bool>(entity, Prop_Send, "m_bHasSapper");

	if (sapper)
	{
		return *sapper;
	}

	return false;
}

CBaseEntity* tf2lib::GetFirstValidSpawnPointForTeam(TeamFortress2::TFTeam team)
{
	CBaseEntity* spawn = nullptr;
	auto func = [&spawn, &team](int index, edict_t* edict, CBaseEntity* entity) {
		if (entity)
		{
			bool disabled = false;
			entprops->GetEntPropBool(index, Prop_Data, "m_bDisabled", disabled);

			if (disabled)
			{
				return true; // keep loop
			}

			if (tf2lib::GetEntityTFTeam(index) == team)
			{
				spawn = entity;
				return false; // end loop
			}
		}

		return true;
	};

	UtilHelpers::ForEachEntityOfClassname("info_player_teamspawn", func);

	return spawn;
}

CBaseEntity* tf2lib::GetNearestValidSpawnPointForTeam(TeamFortress2::TFTeam team, const Vector& origin)
{
	CBaseEntity* spawn = nullptr;
	float best = FLT_MAX;
	auto func = [&spawn, &best, &team, &origin](int index, edict_t* edict, CBaseEntity* entity) {
		if (entity)
		{
			bool disabled = false;
			entprops->GetEntPropBool(index, Prop_Data, "m_bDisabled", disabled);

			if (disabled)
			{
				return true; // keep loop
			}

			if (tf2lib::GetEntityTFTeam(index) != team)
			{
				return true;
			}

			const float distance = (origin - UtilHelpers::getEntityOrigin(entity)).Length();

			if (distance < best)
			{
				best = distance;
				spawn = entity;
			}
		}

		return true;
	};

	UtilHelpers::ForEachEntityOfClassname("info_player_teamspawn", func);

	return spawn;
}

const Vector& tf2lib::GetFlagPosition(CBaseEntity* flag)
{
	CBaseEntity* owner = nullptr;
	entprops->GetEntPropEnt(flag, Prop_Data, "m_hOwnerEntity", nullptr, &owner);

	if (!owner)
	{
		return UtilHelpers::getEntityOrigin(flag);
	}
	
	return UtilHelpers::getEntityOrigin(owner);
}

CBaseEntity* tf2lib::GetMatchingTeleporter(CBaseEntity* teleporter)
{
	static unsigned int offset = 0;

	if (offset == 0)
	{
		// initialize

		SourceMod::IGameConfig* cfg = nullptr;
		char buffer[256];
		int offset_to = 0;

		if (!gameconfs->LoadGameConfigFile("navbot.games", &cfg, buffer, sizeof(buffer)))
		{
			smutils->LogError(myself, "%s", buffer);
			return nullptr;
		}

		if (!cfg->GetOffset("CObjectTeleporter::m_hMatchingTeleporter", &offset_to))
		{
			gameconfs->CloseGameConfigFile(cfg);
			smutils->LogError(myself, "Failed to get CObjectTeleporter::m_hMatchingTeleporter offset from NavBot's gamedata file!");
			return nullptr;
		}

		gameconfs->CloseGameConfigFile(cfg);
		SourceMod::sm_sendprop_info_t info;
		ServerClass* clss = gamehelpers->FindEntityServerClass(teleporter);

		if (!clss)
		{
			return nullptr;
		}

		if (gamehelpers->FindSendPropInfo(clss->GetName(), "m_bMatchBuilding", &info))
		{
			offset = info.actual_offset + static_cast<unsigned int>(offset_to);
			smutils->LogMessage(myself, "Computed offset for CObjectTeleporter::m_hMatchingTeleporter: %u", offset);
		}
	}

	CHandle<CBaseEntity>* hMatchingTeleporter = entprops->GetPointerToEntData<CHandle<CBaseEntity>>(teleporter, offset);
	return hMatchingTeleporter->Get();
}

int tf2lib::GetSentryKills(CBaseEntity* sentry)
{
	int* value = entprops->GetPointerToEntData<int>(sentry, Prop_Send, "m_iKills");
	return *value;
}

int tf2lib::GetSentryAssists(CBaseEntity* sentry)
{
	int* value = entprops->GetPointerToEntData<int>(sentry, Prop_Send, "m_iAssists");
	return *value;
}

int tf2lib::GetTeleporterUses(CBaseEntity* teleporter)
{
	int* value = entprops->GetPointerToEntData<int>(teleporter, Prop_Send, "m_iTimesUsed");
	return *value;
}

bool tf2lib::IsPlayerCarryingAFlag(CBaseEntity* player)
{
	int item = INVALID_EHANDLE_INDEX;

	if (entprops->GetEntPropEnt(player, Prop_Send, "m_hItem", &item))
	{
		return item != INVALID_EHANDLE_INDEX;
	}

	return false;
}

CBaseEntity* tf2lib::mvm::GetMostDangerousFlag(bool ignoreDropped)
{
	const Vector& hatch = CTeamFortress2Mod::GetTF2Mod()->GetMvMBombHatchPosition();

	// find the flag closests to the bomb hatch position

	CBaseEntity* flag = nullptr;
	float distance = 1e10f;
	auto func = [&hatch, &flag, &distance, &ignoreDropped](int index, edict_t* edict, CBaseEntity* entity) {

		if (entity != nullptr)
		{
			tfentities::HCaptureFlag captureflag(entity);

			if (captureflag.IsDisabled() || captureflag.GetTFTeam() != TeamFortress2::TFTeam::TFTeam_Blue || captureflag.IsHome())
			{
				return true;
			}

			if (ignoreDropped && captureflag.IsDropped())
			{
				return true;
			}

			CBaseEntity* carrier = captureflag.GetOwnerEntity();

			if (carrier && UtilHelpers::IsPlayer(carrier))
			{
				if (tf2lib::IsPlayerInCondition(carrier, TeamFortress2::TFCond::TFCond_UberchargedHidden))
				{
					return true; // skip this one, carrier is inside spawn
				}
			}

			Vector origin = captureflag.GetPosition();
			float range = (hatch - origin).Length();

			if (range < distance)
			{
				distance = range;
				flag = entity;
			}
		}

		return true;
	};

	UtilHelpers::ForEachEntityOfClassname("item_teamflag", func);

	return flag;
}

CBaseEntity* tf2lib::mvm::GetMostDangerousTank()
{
	const Vector& hatch = CTeamFortress2Mod::GetTF2Mod()->GetMvMBombHatchPosition();

	// find the tank closests to the bomb hatch position
	// not the best metric due to map design, we could follow the tank's path and see which tank is closests to the last path node

	CBaseEntity* tank = nullptr;
	float distance = 1e10f;
	auto func = [&hatch, &tank, &distance](int index, edict_t* edict, CBaseEntity* entity) {

		if (entity != nullptr)
		{
			const Vector& origin = UtilHelpers::getEntityOrigin(entity);
			float range = (hatch - origin).Length();

			if (range < distance)
			{
				distance = range;
				tank = entity;
			}
		}

		return true;
	};

	UtilHelpers::ForEachEntityOfClassname("tank_boss", func);

	return tank;
}

// these are written based on the official SD map

CBaseEntity* tf2lib::sd::GetSpecialDeliveryFlag()
{
	return gamehelpers->ReferenceToEntity(UtilHelpers::FindEntityByClassname(INVALID_EHANDLE_INDEX, "item_teamflag"));
}

CBaseEntity* tf2lib::sd::GetSpecialDeliveryCaptureZone()
{
	return gamehelpers->ReferenceToEntity(UtilHelpers::FindEntityByClassname(INVALID_EHANDLE_INDEX, "func_capturezone"));
}

std::string tf2lib::maps::GetMapName()
{
	// TF2 workshop names follows this format: workshop/ctf_harbine.ugc3067683041
	// this method should be fine for most maps
	std::string mapname(gamehelpers->GetCurrentMap());

	auto n1 = mapname.find("workshop/");
	auto n2 = mapname.find(".ugc");

	if (n1 == std::string::npos)
	{
		return mapname; // not a workshop map
	}

	constexpr size_t WORKSHOP_STR_SIZE = 9U;
	constexpr size_t UGC_STR_SIZE = 4U;
	auto count = n2 - (n1 + WORKSHOP_STR_SIZE);
	auto sub1 = mapname.substr(n1 + WORKSHOP_STR_SIZE, count);
	auto sub2 = mapname.substr(n2 + UGC_STR_SIZE);
	std::string finalname = sub1 + "_ugc" + sub2; // becomes something like this: ctf_harbine_ugc3067683041
	return finalname;
}

CBaseEntity* tf2lib::pd::GetTeamLeader(TeamFortress2::TFTeam team)
{
	CBaseEntity* leader = nullptr;
	int entindex = UtilHelpers::FindEntityByClassname(INVALID_EHANDLE_INDEX, "tf_logic_player_destruction");

	if (entindex != INVALID_EHANDLE_INDEX)
	{
		if (team == TeamFortress2::TFTeam::TFTeam_Red)
		{
			int iLeader = INVALID_EHANDLE_INDEX; 
			entprops->GetEntPropEnt(entindex, Prop_Send, "m_hRedTeamLeader", &iLeader);

			if (iLeader != INVALID_EHANDLE_INDEX)
			{
				leader = gamehelpers->ReferenceToEntity(iLeader);
			}
		}
		else
		{
			int iLeader = INVALID_EHANDLE_INDEX;
			entprops->GetEntPropEnt(entindex, Prop_Send, "m_hBlueTeamLeader", &iLeader);

			if (iLeader != INVALID_EHANDLE_INDEX)
			{
				leader = gamehelpers->ReferenceToEntity(iLeader);
			}
		}
	}

	return leader;
}

CBaseEntity* tf2lib::passtime::GetJack()
{
	int index = UtilHelpers::FindEntityByClassname(INVALID_EHANDLE_INDEX, "passtime_ball");
	edict_t* edict = gamehelpers->EdictOfIndex(index);

	if (UtilHelpers::IsValidEdict(edict))
	{
		return UtilHelpers::EdictToBaseEntity(edict);
	}

	return nullptr;
}

CBaseEntity* tf2lib::passtime::GetJackCarrier(CBaseEntity* jack)
{
	CBaseEntity* pCarrier = nullptr;
	entprops->GetEntPropEnt(jack, Prop_Send, "m_hCarrier", nullptr, &pCarrier);
	return pCarrier;
}

bool tf2lib::passtime::IsJackActive()
{
	int index = UtilHelpers::FindEntityByClassname(INVALID_EHANDLE_INDEX, "passtime_ball");

	if (index == INVALID_EHANDLE_INDEX)
	{
		return false;
	}

	TeamFortress2::TFTeam team = tf2lib::GetEntityTFTeam(index);

	if (team >= TeamFortress2::TFTeam::TFTeam_Red)
	{
		return true;
	}

	int effects = 0;
	entprops->GetEntProp(index, Prop_Send, "m_fEffects", effects);
	
	if (team == TeamFortress2::TFTeam::TFTeam_Unassigned && (effects & EF_NODRAW) != 0)
	{
		return false;
	}

	return true;
}

bool tf2lib::passtime::GetJackSpawnPosition(Vector* spawnPos)
{
	edict_t* spawnent = gamehelpers->EdictOfIndex(UtilHelpers::FindEntityByClassname(INVALID_EHANDLE_INDEX, "info_passtime_ball_spawn"));

	if (UtilHelpers::IsValidEdict(spawnent))
	{
		const Vector& origin = UtilHelpers::getEntityOrigin(spawnent);
		*spawnPos = origin;
		return true;
	}

	return false;
}

CBaseEntity* tf2lib::passtime::GetGoalEntity(TeamFortress2::PassTimeGoalType goaltype, TeamFortress2::TFTeam team)
{
	CBaseEntity* goal = nullptr;
	auto func = [&goal, &goaltype, &team](int index, edict_t* edict, CBaseEntity* entity) {
		if (entity)
		{
			bool disabled = false;
			entprops->GetEntPropBool(index, Prop_Send, "m_bTriggerDisabled", disabled);

			if (disabled)
			{
				return true;
			}

			if (tf2lib::GetEntityTFTeam(entity) != team)
			{
				return true;
			}

			int type = -1;
			entprops->GetEntProp(index, Prop_Send, "m_iGoalType", type);

			if (type == static_cast<int>(goaltype))
			{
				goal = entity;
				return false;
			}
		}

		return true;
	};
	UtilHelpers::ForEachEntityOfClassname("func_passtime_goal", func);

	return goal;
}

class FooBar
{
public:

	virtual ~FooBar() = default;

	char pad[64];
	bool a;
	int b;
	bool c;
};

bool tf2lib::rd::IsRobotInvulnerable(CBaseEntity* robot)
{
	static int offset = 0;

	if (offset == 0)
	{
		SourceMod::IGameConfig* cfg = nullptr;
		char error[512];
		int temp = 0;
		
		if (!gameconfs->LoadGameConfigFile("navbot.games", &cfg, error, sizeof(error)))
		{
			smutils->LogError(myself, "%s", error);
			return false;
		}

		if (!cfg->GetOffset("CTFRobotDestruction_Robot::m_bShielded", &temp))
		{
			smutils->LogError(myself, "Failed to get relative offset of CTFRobotDestruction_Robot::m_bShielded from NavBot's gamedata!");
			return false;
		}
		else
		{
			SourceMod::sm_sendprop_info_t info;
			
			if (!gamehelpers->FindSendPropInfo("CTFRobotDestruction_Robot", "m_eType", &info))
			{
				smutils->LogError(myself, "Failed to find offset for networked property CTFRobotDestruction_Robot::m_eType");
				return false;
			}
			else
			{
				offset = static_cast<int>(info.actual_offset) + temp;
				smutils->LogMessage(myself, "Loaded offset %i (%u + %i) for CTFRobotDestruction_Robot::m_bShielded", offset, info.actual_offset, temp);
			}
		}
	}

	bool* shielded = entprops->GetPointerToEntData<bool>(robot, static_cast<unsigned int>(offset));
	return *shielded;
}

bool tf2lib::vsh::IsPlayerTheSaxtonHale(CBaseEntity* player)
{
	return tf2lib::GetEntityTFTeam(player) == CTeamFortress2Mod::GetTF2Mod()->GetTF2ModSettings()->GetVSHSaxtonHaleTeam();
}

tf2lib::TeamClassData::TeamClassData()
{
	players_on_team = 0;
	std::fill(players_as_class.begin(), players_as_class.end(), 0);
	team = TeamFortress2::TFTeam::TFTeam_Unassigned;
}

void tf2lib::TeamClassData::operator()(int client, edict_t* entity, SourceMod::IGamePlayer* player)
{
	if (player && player->IsInGame())
	{
		if (tf2lib::GetEntityTFTeam(client) == this->team)
		{
			players_on_team += 1;

			TeamFortress2::TFClassType theirclass = tf2lib::GetPlayerClassType(client);

			if (TeamFortress2::IsValidTFClass(theirclass))
			{
				players_as_class[static_cast<int>(theirclass)] += 1;
			}
		}
	}
}