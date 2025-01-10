#include <extension.h>
#include <util/helpers.h>
#include <util/entprops.h>
#include <util/sdkcalls.h>
#include <entities/tf2/tf_entities.h>
#include <mods/tf2/tf2lib.h>

#include "tf2bot.h"
#include "tf2bot_sensor.h"

#if SOURCE_ENGINE == SE_TF2
// ConVar sm_navbot_tf_attack_nextbots("sm_navbot_tf_attack_nextbots", "1", FCVAR_GAMEDLL, "If enabled, allow bots to attacks NextBot entities.");
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

bool CTF2BotSensor::IsIgnored(CBaseEntity* entity)
{
	int index = gamehelpers->EntityToBCompatRef(entity);
	auto classname = entityprops::GetEntityClassname(entity);

	if (classname == nullptr)
		return true;

	if (UtilHelpers::IsPlayerIndex(index))
	{
		return IsPlayerIgnoredInternal(entity);
	}

#if SOURCE_ENGINE == SE_TF2
/* Causes bad CPU perf, look for a better solution (probably an SDK call)
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
*/
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

bool CTF2BotSensor::IsFriendly(CBaseEntity* entity)
{
	CTF2Bot* me = static_cast<CTF2Bot*>(GetBot());

	TeamFortress2::TFTeam theirteam = static_cast<TeamFortress2::TFTeam>(entityprops::GetEntityTeamNum(entity));

	if (theirteam == me->GetMyTFTeam())
	{
		return true;
	}

	return false;
}

bool CTF2BotSensor::IsEnemy(CBaseEntity* entity)
{
	CTF2Bot* me = static_cast<CTF2Bot*>(GetBot());
	auto spymonitor = me->GetSpyMonitorInterface();
	TeamFortress2::TFTeam theirteam = static_cast<TeamFortress2::TFTeam>(entityprops::GetEntityTeamNum(entity));
	
	if (theirteam == me->GetMyTFTeam())
	{
		return false;
	}

	int index = gamehelpers->EntityToBCompatRef(entity);
	if (UtilHelpers::IsPlayerIndex(index))
	{
		TeamFortress2::TFClassType theirclass = static_cast<TeamFortress2::TFClassType>(entprops->GetCachedData<int>(entity, CEntPropUtils::CacheIndex::CTFPLAYER_CLASSTYPE));

		if (theirclass == TeamFortress2::TFClass_Spy && tf2lib::IsPlayerDisguised(entity))
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

bool CTF2BotSensor::IsPlayerIgnoredInternal(CBaseEntity* entity)
{
	if (IgnoredConditionsInternal(entity))
	{
		return true;
	}

	if (!IsFriendly(entity) && tf2lib::IsPlayerInvisible(entity))
	{
		return true; // Don't see invisible enemies
	}

	return false;
}

// TFConds that the bots should always ignore
bool CTF2BotSensor::IgnoredConditionsInternal(CBaseEntity* player)
{
	// ignore halloween ghost, ignore robots inside spawn

	if (tf2lib::IsPlayerInCondition(player, TeamFortress2::TFCond::TFCond_HalloweenGhostMode) ||
		tf2lib::IsPlayerInCondition(player, TeamFortress2::TFCond::TFCond_UberchargedHidden))
	{
		return true;
	}

	return false;
}

bool CTF2BotSensor::IsClassnameIgnored(const char* classname)
{
	static std::string key;

	key.assign(classname);

	// classname filter contains a list of classnames the bot cares about, if not found on the list, ignore it
	return m_classname_filter.find(key) == m_classname_filter.end();
}
