#include <extension.h>
#include <util/helpers.h>
#include <util/entprops.h>
#include <entities/tf2/tf_entities.h>
#include <mods/tf2/tf2lib.h>

#include "tf2bot.h"
#include "tf2bot_sensor.h"

#if SOURCE_ENGINE == SE_TF2
ConVar sm_navbot_tf_attack_nextbots("sm_navbot_tf_attack_nextbots", "1", FCVAR_GAMEDLL, "If enabled, allow bots to attacks NextBot entities.");
#endif // SOURCE_ENGINE == SE_TF2


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
	AddClassnametoFilter("base_boss");
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

#if SOURCE_ENGINE == SE_TF2
	if (sm_navbot_tf_attack_nextbots.GetBool())
	{
		CBaseEntity* be = reinterpret_cast<CBaseEntity*>(entity->GetIServerEntity());
		ServerClass* sc = gamehelpers->FindEntityServerClass(be);

		if (sc != nullptr)
		{
			if (UtilHelpers::HasDataTable(sc->m_pTable, "DT_NextBot"))
			{
				return false; // Don't ignore NextBot entities.
			}
		}
	}
#endif // SOURCE_ENGINE == SE_TF2

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
	auto theirteam = tf2lib::GetEntityTFTeam(index);

	if (theirteam == me->GetMyTFTeam())
	{
		return true;
	}

	return false;
}

bool CTF2BotSensor::IsEnemy(edict_t* entity)
{
	CTF2Bot* me = static_cast<CTF2Bot*>(GetBot());
	auto spymonitor = me->GetSpyMonitorInterface();
	int index = gamehelpers->IndexOfEdict(entity);
	auto theirteam = tf2lib::GetEntityTFTeam(index);
	
	if (theirteam == me->GetMyTFTeam())
	{
		return false;
	}

	if (UtilHelpers::IsPlayerIndex(index))
	{
		auto theirclass = tf2lib::GetPlayerClassType(index);

		if (theirclass == TeamFortress2::TFClass_Spy && tf2lib::IsPlayerDisguised(index))
		{
			auto& knownspy = spymonitor->GetKnownSpy(entity);
			return knownspy.IsHostile();
		}
	}

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

	if (!IsFriendly(entity) && tf2lib::IsPlayerInvisible(player))
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
