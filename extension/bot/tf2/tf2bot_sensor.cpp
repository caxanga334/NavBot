#include NAVBOT_PCH_FILE
#include <extension.h>
#include <util/helpers.h>
#include <util/entprops.h>
#include <util/sdkcalls.h>
#include <entities/tf2/tf_entities.h>
#include <mods/tf2/tf2lib.h>
#include <mods/tf2/teamfortress2mod.h>

#include "tf2bot.h"
#include "tf2bot_sensor.h"

#ifdef EXT_VPROF_ENABLED
#include <tier0/vprof.h>
#endif // EXT_VPROF_ENABLED

#if SOURCE_ENGINE == SE_TF2
// ConVar sm_navbot_tf_attack_nextbots("sm_navbot_tf_attack_nextbots", "1", FCVAR_GAMEDLL, "If enabled, allow bots to attacks NextBot entities.");
static ConVar cvar_teammates_are_enemies("sm_navbot_tf_teammates_are_enemies", "0", FCVAR_GAMEDLL, "If enabled, bots will consider players from the same team as enemies.");
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

bool CTF2BotSensor::IsIgnored(CBaseEntity* entity) const
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("CTF2BotSensor::IsIgnored", "NavBot");
#endif // EXT_VPROF_ENABLED

	int index = gamehelpers->EntityToBCompatRef(entity);
	const char* classname = entityprops::GetEntityClassname(entity);

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

bool CTF2BotSensor::IsFriendly(CBaseEntity* entity) const
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("CTF2BotSensor::IsFriendly", "NavBot");
#endif // EXT_VPROF_ENABLED

	CTF2Bot* me = GetBot<CTF2Bot>();

	TeamFortress2::TFTeam theirteam = static_cast<TeamFortress2::TFTeam>(entityprops::GetEntityTeamNum(entity));

	if (theirteam == me->GetMyTFTeam())
	{
#if SOURCE_ENGINE == SE_TF2
		if (cvar_teammates_are_enemies.GetBool())
		{
			return false;
		}
#endif // SOURCE_ENGINE == SE_TF2

		return true;
	}

	return false;
}

bool CTF2BotSensor::IsEnemy(CBaseEntity* entity) const
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("CTF2BotSensor::IsEnemy", "NavBot");
#endif // EXT_VPROF_ENABLED

	CTF2Bot* me = GetBot<CTF2Bot>();
	auto spymonitor = me->GetSpyMonitorInterface();
	TeamFortress2::TFTeam theirteam = static_cast<TeamFortress2::TFTeam>(entityprops::GetEntityTeamNum(entity));
	
	if (theirteam == me->GetMyTFTeam())
	{
#if SOURCE_ENGINE == SE_TF2
		if (cvar_teammates_are_enemies.GetBool())
		{
			return true;
		}
#endif // SOURCE_ENGINE == SE_TF2

		return false;
	}

	int index = gamehelpers->EntityToBCompatRef(entity);

	if (CTeamFortress2Mod::GetTF2Mod()->IsTruceActive())
	{
		// don't attack players on truce
		if (UtilHelpers::IsPlayerIndex(index))
		{
			return false;
		}

		const char* classname = entityprops::GetEntityClassname(entity);

		// don't attack enemy buildings on truce
		if (strncasecmp(classname, "obj_", 4) == 0)
		{
			return false;
		}
	}

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
	return entityprops::GetEntityTeamNum(known->GetEntity());
}

void CTF2BotSensor::OnTruceChanged(const bool enabled)
{
	if (enabled)
	{
		// Reset sensor because enemy players are no longer enemies
		Reset();
	}
}

bool CTF2BotSensor::IsPlayerIgnoredInternal(CBaseEntity* entity) const
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("CTF2BotSensor::IsPlayerIgnoredInternal", "NavBot");
#endif // EXT_VPROF_ENABLED

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
bool CTF2BotSensor::IgnoredConditionsInternal(CBaseEntity* player) const
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("CTF2BotSensor::IgnoredConditionsInternal", "NavBot");
#endif // EXT_VPROF_ENABLED

	// ignore halloween ghost, ignore robots inside spawn

	if (tf2lib::IsPlayerInCondition(player, TeamFortress2::TFCond::TFCond_HalloweenGhostMode) ||
		tf2lib::IsPlayerInCondition(player, TeamFortress2::TFCond::TFCond_UberchargedHidden))
	{
		return true;
	}

	return false;
}

bool CTF2BotSensor::IsClassnameIgnored(const char* classname) const
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("CTF2BotSensor::IsClassnameIgnored", "NavBot");
#endif // EXT_VPROF_ENABLED

	static std::string key;

	key.assign(classname);

	// classname filter contains a list of classnames the bot cares about, if not found on the list, ignore it
	return m_classname_filter.find(key) == m_classname_filter.end();
}
