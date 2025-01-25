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

void CBlackMesaBotSensor::CollectNonPlayerEntities(std::vector<edict_t*>& visibleVec)
{
	// empty for now
}
