#include NAVBOT_PCH_FILE
#include <extension.h>
#include <mods/modhelpers.h>
#include <util/helpers.h>
#include <util/entprops.h>
#include "dodslib.h"

dayofdefeatsource::DoDTeam dodslib::GetDoDTeam(CBaseEntity* entity)
{
	return static_cast<dayofdefeatsource::DoDTeam>(modhelpers->GetEntityTeamNumber(entity));
}

dayofdefeatsource::DoDClassType dodslib::GetPlayerClassType(CBaseEntity* player)
{
	return  static_cast<dayofdefeatsource::DoDClassType>(entprops->GetCachedData<int>(player, CEntPropUtils::CDODPLAYER_PLAYERCLASS));
}

const char* dodslib::GetJoinClassCommand(dayofdefeatsource::DoDClassType classtype, dayofdefeatsource::DoDTeam team)
{
	if (team == dayofdefeatsource::DoDTeam::DODTEAM_ALLIES)
	{
		switch (classtype)
		{
		case dayofdefeatsource::DODCLASS_RIFLEMAN:
			return "cls_garand";
		case dayofdefeatsource::DODCLASS_ASSAULT:
			return "cls_tommy";
		case dayofdefeatsource::DODCLASS_SUPPORT:
			return "cls_bar";
		case dayofdefeatsource::DODCLASS_SNIPER:
			return "cls_spring";
		case dayofdefeatsource::DODCLASS_MACHINEGUNNER:
			return "cls_30cal";
		case dayofdefeatsource::DODCLASS_ROCKET:
			return "cls_bazooka";
		default:
			return "cls_random";
		}
	}
	else
	{
		switch (classtype)
		{
		case dayofdefeatsource::DODCLASS_RIFLEMAN:
			return "cls_k98";
		case dayofdefeatsource::DODCLASS_ASSAULT:
			return "cls_mp40";
		case dayofdefeatsource::DODCLASS_SUPPORT:
			return "cls_mp44";
		case dayofdefeatsource::DODCLASS_SNIPER:
			return "cls_k98s";
		case dayofdefeatsource::DODCLASS_MACHINEGUNNER:
			return "cls_mg42";
		case dayofdefeatsource::DODCLASS_ROCKET:
			return "cls_pschreck";
		default:
			return "cls_random";
		}
	}
}

int dodslib::GetControlPointIndex(CBaseEntity* entity)
{
#ifdef EXT_DEBUG
	const char* classname = gamehelpers->GetEntityClassname(entity);

	if (strcasecmp(classname, "dod_control_point") != 0)
	{
		smutils->LogError(myself, "dodslib::GetControlPointIndex called with invalid entity %s", classname);
	}
#endif // EXT_DEBUG

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
			return -1;
		}

		if (!cfg->GetOffset("CControlPoint::m_iPointIndex", &offset_to))
		{
			gameconfs->CloseGameConfigFile(cfg);
			smutils->LogError(myself, "Failed to get CControlPoint::m_iPointIndex offset from NavBot's gamedata file!");
			return -1;
		}

		gameconfs->CloseGameConfigFile(cfg);
		datamap_t* datamap = gamehelpers->GetDataMap(entity);
		SourceMod::sm_datatable_info_t info;

		if (gamehelpers->FindDataMapInfo(datamap, "m_iCPGroup", &info))
		{
			offset = info.actual_offset + static_cast<unsigned int>(offset_to);
			smutils->LogMessage(myself, "Computed offset for CControlPoint::m_iPointIndex: %u", offset);
		}
	}

	int* index = entprops->GetPointerToEntData<int>(entity, offset);
	return *index;
}

dayofdefeatsource::DoDRoundState dodslib::GetRoundState()
{
	int roundstate = 0;
	entprops->GameRules_GetProp("m_iRoundState", roundstate);
	return static_cast<dayofdefeatsource::DoDRoundState>(roundstate);
}

bool dodslib::CanPlantBombAtTarget(CBaseEntity* bombTarget)
{
	int state = -1;
	entprops->GetEntProp(bombTarget, Prop_Send, "m_iState", state);
	return state == static_cast<int>(dayofdefeatsource::DoDBombTargetState::BOMB_TARGET_ACTIVE);
}

bool dodslib::CanTeamPlantBombAtTarget(CBaseEntity* bombTarget, int TeamNum)
{
	int team = -1;
	entprops->GetEntProp(bombTarget, Prop_Send, "m_iBombingTeam", team);
	return team == TEAM_UNASSIGNED || team == TeamNum;
}

void dodslib::GetTeamClassCount(dayofdefeatsource::DoDTeam team, DoDClassCountArray& classes)
{
	std::fill(std::begin(classes), std::end(classes), 0);

	for (int client = 1; client <= gpGlobals->maxClients; client++)
	{
		SourceMod::IGamePlayer* gp = playerhelpers->GetGamePlayer(client);

		if (!gp || !gp->IsInGame()) { continue; }

		CBaseEntity* pPlayer = gamehelpers->ReferenceToEntity(client);

		if (!pPlayer) { continue; }

		dayofdefeatsource::DoDTeam theirteam = GetDoDTeam(pPlayer);

		if (theirteam != team) { continue; }

		int idx = static_cast<int>(GetPlayerClassType(pPlayer));

		int c = classes[idx];
		c++;
		classes[idx] = c;
	}
}
