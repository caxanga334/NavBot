#include <extension.h>
#include <util/helpers.h>
#include <util/entprops.h>
#include <util/EntityUtils.h>
#include <entities/tf2/tf_entities.h>
#include <mods/tf2/tf2lib.h>

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
	{
		return IsPlayerIgnoredInternal(entity);
	}

	if (IsClassnameIgnored(classname))
		return true;

	if (strncasecmp(classname, "obj_", 4) == 0)
	{
		tfentities::HBaseObject baseobject(entity);

		if (baseobject.IsPlacing()) // ignore objects that haven't been built yet
			return true;
	}

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

bool CTF2BotSensor::IsPlayerIgnoredInternal(edict_t* entity)
{
	int player = gamehelpers->IndexOfEdict(entity);

	if (IgnoredConditionsInternal(player))
	{
		return true;
	}

	if (IsEnemy(entity) && tf2lib::IsPlayerInvisible(player))
	{
		return true; // Don't see invisible enemies
	}

	return false;
}

// TFConds that the bots should always ignore
bool CTF2BotSensor::IgnoredConditionsInternal(int player)
{
	if (tf2lib::IsPlayerInCondition(player, TeamFortress2::TFCond::TFCond_HalloweenGhostMode))
	{
		return true;
	}

	return false;
}
