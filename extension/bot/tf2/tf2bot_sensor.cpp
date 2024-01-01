#include <extension.h>
#include <util/helpers.h>
#include <util/entprops.h>

#include "tf2bot.h"
#include "tf2bot_sensor.h"

CTF2BotSensor::CTF2BotSensor(CBaseBot* bot) : ISensor(bot)
{
	m_classname_filter.reserve(12);

	// Build the list of entities classname that the sensor system should care about
	// Entities listed here will be checked and tracked
	// Note that this is used for tracking entities that are enemies to the bot. There is no need to add health/ammo packs to this list for example
	AddClassnametoFilter("player");
	AddClassnametoFilter("obj_dispenser");
	AddClassnametoFilter("obj_sentrygun");
	AddClassnametoFilter("obj_teleporter");
	AddClassnametoFilter("tank_boss");
	AddClassnametoFilter("tf_merasmus_trick_or_treat_prop");
	AddClassnametoFilter("merasmus");
	AddClassnametoFilter("eyeball_boss");
	AddClassnametoFilter("tf_zombie");
	AddClassnametoFilter("headless_hatman");
	AddClassnametoFilter("tf_robot_destruction_robot");
}

CTF2BotSensor::~CTF2BotSensor()
{
}

bool CTF2BotSensor::IsIgnored(edict_t* entity)
{
	auto classname = gamehelpers->GetEntityClassname(entity);

	if (!classname)
		return true;

	int index = gamehelpers->IndexOfEdict(entity);

	if (UtilHelpers::IsPlayerIndex(index))
		return false; // don't ignore players

	if (IsClassnameIgnored(classname))
		return true;

	return false;
}

bool CTF2BotSensor::IsFriendly(edict_t* entity)
{
	CTF2Bot* me = static_cast<CTF2Bot*>(GetBot());

	int index = gamehelpers->IndexOfEdict(entity);
	int myteam = me->GetCurrentTeamIndex();
	int theirteam = TEAM_UNASSIGNED;

	entprops->GetEntProp(index, Prop_Data, "m_iTeamNum", theirteam);

	if (theirteam == myteam)
		return true;

	return false;
}

bool CTF2BotSensor::IsEnemy(edict_t* entity)
{
	CTF2Bot* me = static_cast<CTF2Bot*>(GetBot());

	int index = gamehelpers->IndexOfEdict(entity);
	int myteam = me->GetCurrentTeamIndex();
	int theirteam = TEAM_UNASSIGNED;

	entprops->GetEntProp(index, Prop_Data, "m_iTeamNum", theirteam);

	if (theirteam == myteam)
		return false;

	return true;
}

int CTF2BotSensor::GetKnownEntityTeamIndex(CKnownEntity* known)
{
	auto entity = known->GetEdict();
	auto index = gamehelpers->IndexOfEdict(entity);
	int theirteam = TEAM_UNASSIGNED;

	entprops->GetEntProp(index, Prop_Data, "m_iTeamNum", theirteam);

	return theirteam;
}
