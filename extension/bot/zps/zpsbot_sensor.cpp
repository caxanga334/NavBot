#include NAVBOT_PCH_FILE
#include <mods/zps/zps_mod.h>
#include "zpsbot.h"
#include "zpsbot_sensor.h"

CZPSBotSensor::CZPSBotSensor(CZPSBot* bot) :
	ISensor(bot)
{
}

CZPSBotSensor::~CZPSBotSensor()
{
}

bool CZPSBotSensor::IsIgnored(CBaseEntity* entity) const
{
	zps::ZPSTeam team = zpslib::GetEntityZPSTeam(entity);

	if (team <= zps::ZPSTeam::ZPS_TEAM_SPECTATOR)
	{
		return true;
	}

	if (UtilHelpers::FClassnameIs(entity, "player"))
	{
		return false;
	}

	return true;
}

bool CZPSBotSensor::IsFriendly(CBaseEntity* entity) const
{
	CZPSBot* me = GetBot<CZPSBot>();

	return me->GetMyZPSTeam() == zpslib::GetEntityZPSTeam(entity);
}

bool CZPSBotSensor::IsEnemy(CBaseEntity* entity) const
{
	CZPSBot* me = GetBot<CZPSBot>();

	return me->GetMyZPSTeam() != zpslib::GetEntityZPSTeam(entity);
}

void CZPSBotSensor::CollectNonPlayerEntities(std::vector<edict_t*>& visibleVec)
{
	// no-op for now
}
