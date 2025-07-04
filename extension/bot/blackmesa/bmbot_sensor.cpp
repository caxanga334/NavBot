#include NAVBOT_PCH_FILE
#include <extension.h>
#include <util/helpers.h>
#include <util/entprops.h>
#include <mods/blackmesa/blackmesadm_mod.h>
#include "bmbot.h"
#include "bmbot_sensor.h"

CBlackMesaBotSensor::CBlackMesaBotSensor(CBaseBot* bot) :
	ISensor(bot)
{
}

CBlackMesaBotSensor::~CBlackMesaBotSensor()
{
}

bool CBlackMesaBotSensor::IsIgnored(CBaseEntity* entity)
{
	if (UtilHelpers::IsPlayer(entity))
	{
		return false;
	}

	return true;
}

bool CBlackMesaBotSensor::IsFriendly(CBaseEntity* entity)
{
	if (CBlackMesaDeathmatchMod::GetBMMod()->IsTeamPlay())
	{
		return GetBot()->GetCurrentTeamIndex() == entityprops::GetEntityTeamNum(entity);
	}
	
	return false;
}

bool CBlackMesaBotSensor::IsEnemy(CBaseEntity* entity)
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
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		auto edict = gamehelpers->EdictOfIndex(i);

		if (!UtilHelpers::IsValidEdict(edict))
		{
			continue;
		}

		if (i == GetBot()->GetIndex())
			continue; // skip self

		auto gp = playerhelpers->GetGamePlayer(i);

		if (!gp->IsInGame())
		{
			continue; // Client must be fully connected
		}

		auto info = gp->GetPlayerInfo();

		if (info && info->GetTeamIndex() == TEAM_SPECTATOR) {
			continue; // On deathmatch mode, all players are on team unnasigned
		}

		if (info && info->IsDead()) {
			continue; // Ignore dead players
		}

		if (IsIgnored(edict->GetIServerEntity()->GetBaseEntity())) {
			continue; // Ignored player
		}

		CBaseExtPlayer player(edict);

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
