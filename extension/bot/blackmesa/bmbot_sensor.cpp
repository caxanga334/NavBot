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

void CBlackMesaBotSensor::ReportVisibleEntities()
{
	if (CBlackMesaDeathmatchMod::GetBMMod()->IsTeamPlay())
	{
		// only update the shared memory if teamplay is active
		ISensor::ReportVisibleEntities();
	}
}
