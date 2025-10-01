#include NAVBOT_PCH_FILE
#include <extension.h>
#include <util/helpers.h>
#include <util/entprops.h>
#include <mods/blackmesa/blackmesadm_mod.h>
#include "bmbot.h"
#include "bmbot_sensor.h"

#ifdef EXT_VPROF_ENABLED
#include <tier0/vprof.h>
#endif // EXT_VPROF_ENABLED

CBlackMesaBotSensor::CBlackMesaBotSensor(CBaseBot* bot) :
	ISensor(bot)
{
}

CBlackMesaBotSensor::~CBlackMesaBotSensor()
{
}

bool CBlackMesaBotSensor::IsIgnored(CBaseEntity* entity) const
{
	if (UtilHelpers::IsPlayer(entity))
	{
		return false;
	}

	return true;
}

bool CBlackMesaBotSensor::IsFriendly(CBaseEntity* entity) const
{
	if (CBlackMesaDeathmatchMod::GetBMMod()->IsTeamPlay())
	{
		return GetBot()->GetCurrentTeamIndex() == entityprops::GetEntityTeamNum(entity);
	}
	
	return false;
}

bool CBlackMesaBotSensor::IsEnemy(CBaseEntity* entity) const
{
	if (CBlackMesaDeathmatchMod::GetBMMod()->IsTeamPlay())
	{
		return GetBot()->GetCurrentTeamIndex() != entityprops::GetEntityTeamNum(entity);
	}

	// deathmatch mode
	return true;
}

void CBlackMesaBotSensor::CollectPlayers(std::vector<edict_t*>& visibleVec)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("CBlackMesaBotSensor::CollectPlayers", "NavBot");
#endif // EXT_VPROF_ENABLED

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		edict_t* edict = gamehelpers->EdictOfIndex(i);

		if (!UtilHelpers::IsValidEdict(edict))
		{
			continue;
		}

		CBaseExtPlayer* player = extmanager->GetPlayerOfEdict(edict);

		if (!player) { continue; }

		if (i == GetBot()->GetIndex())
			continue; // skip self

		if (!ISensor::IsInPVS(player->GetAbsOrigin()))
		{
			continue;
		}

		if (player->GetCurrentTeamIndex() == TEAM_SPECTATOR)
		{
			continue;
		}

		if (!player->IsAlive())
		{
			continue;
		}

		if (IsIgnored(player->GetEntity()))
		{
			continue; // Ignored player
		}

		if (IsAbleToSee(player))
		{
			visibleVec.push_back(edict);
		}
	}
}

void CBlackMesaBotSensor::CollectNonPlayerEntities(std::vector<edict_t*>& visibleVec)
{
	// empty for now
	return;
}
