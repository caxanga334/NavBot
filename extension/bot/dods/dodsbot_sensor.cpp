#include NAVBOT_PCH_FILE
#include <extension.h>
#include <util/helpers.h>
#include <mods/dods/dodslib.h>
#include "dodsbot.h"
#include "dodsbot_sensor.h"

CDoDSBotSensor::CDoDSBotSensor(CBaseBot* bot) :
	ISensor(bot)
{
}

CDoDSBotSensor::~CDoDSBotSensor()
{
}

bool CDoDSBotSensor::IsIgnored(CBaseEntity* entity) const
{
	// ignore everything but player entities
	return !(UtilHelpers::IsPlayer(entity));
}

bool CDoDSBotSensor::IsFriendly(CBaseEntity* entity) const
{
	CDoDSBot* me = GetBot<CDoDSBot>();

	return dodslib::GetDoDTeam(entity) == me->GetMyDoDTeam();
}

bool CDoDSBotSensor::IsEnemy(CBaseEntity* entity) const
{
	CDoDSBot* me = GetBot<CDoDSBot>();

	return dodslib::GetDoDTeam(entity) != me->GetMyDoDTeam();
}

bool CDoDSBotSensor::IsEntityHidden(CBaseEntity* entity) const
{
	// TO-DO: Implement smoke grenades vision blocking

	return false;
}

bool CDoDSBotSensor::IsPositionObscured(const Vector& pos) const
{
	return false;
}

void CDoDSBotSensor::CollectNonPlayerEntities(std::vector<edict_t*>& visibleVec)
{
	// DoD bots don't need to care about non players, saves perf
}
