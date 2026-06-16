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

static ConVar* s_cvar_teammates_are_enemies = nullptr;


CTF2BotSensor::CTF2BotSensor(CBaseBot* bot) : ISensor(bot)
{
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

	if (entityprops::IsEffectActiveOnEntity(entity, EF_NODRAW))
	{
		return true;
	}

	if (UtilHelpers::IsPlayerIndex(index))
	{
		return IsPlayerIgnoredInternal(entity);
	}

	if (strncasecmp(classname, "obj_", 4) == 0)
	{
		tfentities::HBaseObject baseobject(entity);

		if (baseobject.IsPlacing() || baseobject.IsBeingCarried()) // ignore objects that haven't been built yet
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
	const CTeamFortress2Mod* tf2mod = CTeamFortress2Mod::GetTF2Mod();

	TeamFortress2::TFTeam theirteam = tf2lib::GetEntityTFTeam(entity);

	if (theirteam == me->GetMyTFTeam())
	{
		if (s_cvar_teammates_are_enemies->GetBool())
		{
			return false;
		}

		if (tf2mod->GameModeIsFreeForAll())
		{
			return false;
		}

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
	const CTeamFortress2Mod* tf2mod = CTeamFortress2Mod::GetTF2Mod();
	auto spymonitor = me->GetSpyMonitorInterface();
	TeamFortress2::TFTeam theirteam = tf2lib::GetEntityTFTeam(entity);
	
	if (theirteam == me->GetMyTFTeam())
	{
		if (s_cvar_teammates_are_enemies->GetBool())
		{
			return true;
		}

		if (tf2mod->GameModeIsFreeForAll())
		{
			return true;
		}

		return false;
	}

	int index = gamehelpers->EntityToBCompatRef(entity);

	if (tf2mod->IsTruceActive())
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

void CTF2BotSensor::RegisterTF2ConVars()
{
	CServerCommandManager& manager = extmanager->GetServerCommandManager();
	s_cvar_teammates_are_enemies = manager.RegisterConVar("sm_navbot_tf_teammates_are_enemies", "If enabled, bots will consider players from the same team as enemies.", "0", FCVAR_GAMEDLL);
}