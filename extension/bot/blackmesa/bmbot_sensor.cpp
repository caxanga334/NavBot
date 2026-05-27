#include NAVBOT_PCH_FILE
#include <extension.h>
#include <util/helpers.h>
#include <util/entprops.h>
#include <mods/blackmesa/blackmesadm_mod.h>
#include <mods/modhelpers.h>
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
		return GetBot()->GetCurrentTeamIndex() == modhelpers->GetEntityTeamNumber(entity);
	}
	
	return false;
}

bool CBlackMesaBotSensor::IsEnemy(CBaseEntity* entity) const
{
	if (CBlackMesaDeathmatchMod::GetBMMod()->IsTeamPlay())
	{
		return GetBot()->GetCurrentTeamIndex() != modhelpers->GetEntityTeamNumber(entity);
	}

	// deathmatch mode
	return true;
}

void CBlackMesaBotSensor::CollectPlayers(std::vector<edict_t*>& visibleVec)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("CBlackMesaBotSensor::CollectPlayers", "NavBot");
#endif // EXT_VPROF_ENABLED

	const int myindex = GetBot<CBaseBot>()->GetIndex();

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		// skip self
		if (i == myindex)
		{
			continue;
		}

		CBaseEntity* entity = gamehelpers->ReferenceToEntity(i);

		if (!entity || modhelpers->IsDead(entity))
		{
			continue;
		}
		
		// BM uses TEAM_UNASSIGNED for free for all deathmatch.
		if (modhelpers->GetEntityTeamNumber(entity) == TEAM_SPECTATOR)
		{
			continue;
		}

		if (!ISensor::IsInPVS(UtilHelpers::getEntityOrigin(entity)))
		{
			continue;
		}

		if (IsAbleToSee(entity))
		{
			visibleVec.push_back(UtilHelpers::BaseEntityToEdict(entity));
		}
	}
}

void CBlackMesaBotSensor::CollectNonPlayerEntities(std::vector<edict_t*>& visibleVec)
{
	// empty for now
	return;
}

void CBlackMesaBotSensor::ReportVisibleEntities()
{
	if (CBlackMesaDeathmatchMod::GetBMMod()->IsTeamPlay())
	{
		// only update the shared memory if teamplay is active
		ISensor::ReportVisibleEntities();
	}
}
