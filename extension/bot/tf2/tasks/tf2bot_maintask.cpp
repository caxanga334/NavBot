#include <extension.h>
#include <util/helpers.h>
#include <util/entprops.h>
#include <util/prediction.h>
#include <mods/tf2/teamfortress2mod.h>
#include "bot/tf2/tf2bot.h"
#include "tf2bot_tactical.h"
#include "tf2bot_maintask.h"


AITask<CTF2Bot>* CTF2BotMainTask::InitialNextTask()
{
	return new CTF2BotTacticalTask;
}

TaskResult<CTF2Bot> CTF2BotMainTask::OnTaskUpdate(CTF2Bot* bot)
{
	return Continue();
}

Vector CTF2BotMainTask::GetTargetAimPos(CBaseBot* me, edict_t* entity, CBaseExtPlayer* player)
{
	Vector aimat(0.0f, 0.0f, 0.0f);
	CTF2Bot* tf2bot = static_cast<CTF2Bot*>(me);

	if (player)
	{
		InternalAimAtEnemyPlayer(tf2bot, player, aimat);
	}
	else
	{
		aimat = UtilHelpers::getWorldSpaceCenter(entity);
	}

	return aimat;
}

void CTF2BotMainTask::InternalAimAtEnemyPlayer(CTF2Bot* me, CBaseExtPlayer* player, Vector& result)
{
	auto myweapon = me->GetActiveWeapon();

	if (!myweapon) // how?
	{
		result = player->WorldSpaceCenter();
		return;
	}

	int index = -1;
	std::string classname(gamehelpers->GetEntityClassname(myweapon));
	auto mod = CTeamFortress2Mod::GetTF2Mod();
	entprops->GetEntProp(gamehelpers->IndexOfEdict(myweapon), Prop_Send, "m_iItemDefinitionIndex", index);

	auto weaponinfo = [mod, &index, classname]() {
		auto& wim = mod->GetWeaponInfoManager();
		auto info = wim.GetWeaponInfoByEconIndex(index);

		if (!info)
		{
			info = wim.GetWeaponInfoByClassname(classname.c_str());
		}

		return info;
	}();

	if (!weaponinfo) // no weapon info, just aim at the center
	{
		result = player->WorldSpaceCenter();
		return;
	}

	auto id = mod->GetWeaponID(classname);
	auto sensor = me->GetSensorInterface();

	switch (id)
	{
	case TeamFortress2::TFWeaponID::TF_WEAPON_ROCKETLAUNCHER:
	case TeamFortress2::TFWeaponID::TF_WEAPON_DIRECTHIT:
	case TeamFortress2::TFWeaponID::TF_WEAPON_PARTICLE_CANNON:
	{
		InternalAimWithRocketLauncher(me, player, result, weaponinfo, sensor);
		break;
	}
	case TeamFortress2::TFWeaponID::TF_WEAPON_SNIPERRIFLE:
	case TeamFortress2::TFWeaponID::TF_WEAPON_SNIPERRIFLE_DECAP:
	case TeamFortress2::TFWeaponID::TF_WEAPON_SNIPERRIFLE_CLASSIC:
	{
		auto headpos = player->GetEyeOrigin();

		if (sensor->IsAbleToSee(headpos, false)) // if visible, go for headshots
		{
			result = headpos;
			return;
		}

		result = player->WorldSpaceCenter();
		break;
	}
	default:
		result = player->WorldSpaceCenter();
		break;
	}
}

void CTF2BotMainTask::InternalAimWithRocketLauncher(CTF2Bot* me, CBaseExtPlayer* player, Vector& result, const WeaponInfo* info, CTF2BotSensor* sensor)
{
	if (player->GetGroundEntity() == nullptr) // target is airborne
	{
		CTraceFilterNoNPCsOrPlayer filter(me->GetEdict()->GetIServerEntity(), COLLISION_GROUP_PLAYER_MOVEMENT);
		trace_t trace;
		UTIL_TraceLine(player->GetAbsOrigin(), player->GetAbsOrigin() + Vector(0.0f, 0.0f, -256.0f), MASK_PLAYERSOLID, &filter, &trace);

		if (trace.DidHit())
		{
			result = trace.endpos;
			return;
		}
	}

	// lead target

	float rangeBetween = me->GetRangeTo(player->GetAbsOrigin());

	constexpr float veryCloseRange = 150.0f;
	if (rangeBetween > veryCloseRange)
	{
		Vector targetPos = pred::SimpleProjectileLead(player->GetAbsOrigin(), player->GetAbsVelocity(), info->GetAttackInfo(WeaponInfo::PRIMARY_ATTACK).GetProjectileSpeed(), rangeBetween);

		if (sensor->IsLineOfSightClear(targetPos))
		{
			result = targetPos;
			return;
		}

		// try their head and hope
		result = pred::SimpleProjectileLead(player->GetEyeOrigin(), player->GetAbsVelocity(), info->GetAttackInfo(WeaponInfo::PRIMARY_ATTACK).GetProjectileSpeed(), rangeBetween);
		return;
	}

	result = player->GetEyeOrigin();
	return;

}

