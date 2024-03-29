#include <algorithm>

#include <extension.h>
#include <util/helpers.h>
#include <util/entprops.h>
#include <util/prediction.h>
#include <mods/tf2/teamfortress2mod.h>
#include <mods/tf2/tf2lib.h>
#include "bot/tf2/tf2bot.h"
#include "tf2bot_tactical.h"
#include "tf2bot_maintask.h"

#undef max
#undef min
#undef clamp

AITask<CTF2Bot>* CTF2BotMainTask::InitialNextTask()
{
	return new CTF2BotTacticalTask;
}

TaskResult<CTF2Bot> CTF2BotMainTask::OnTaskUpdate(CTF2Bot* bot)
{
	auto sensor = bot->GetSensorInterface();
	auto threat = sensor->GetPrimaryKnownThreat();

	if (threat != nullptr) // I have an enemy
	{
		SelectBestWeaponForEnemy(bot, threat);
		FireWeaponAtEnemy(bot, threat);
	}

	UpdateLook(bot, threat);

	return Continue();
}

CKnownEntity* CTF2BotMainTask::SelectTargetThreat(CBaseBot* me, CKnownEntity* threat1, CKnownEntity* threat2)
{
	// Handle cases where one of them is NULL
	if (threat1 && !threat2)
	{
		return threat1;
	}
	else if (!threat1 && threat2)
	{
		return threat2;
	}
	else if (threat1 == threat2)
	{
		return threat1; // if both are the same, return threat1
	}

	return InternalSelectTargetThreat(static_cast<CTF2Bot*>(me), threat1, threat2);
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

void CTF2BotMainTask::FireWeaponAtEnemy(CTF2Bot* me, CKnownEntity* threat)
{
	if (me->GetPlayerInfo()->IsDead())
		return;

	if (!me->WantsToShootAtEnemies())
		return;

	if (tf2lib::IsPlayerInCondition(me->GetIndex(), TeamFortress2::TFCond::TFCond_Taunting))
		return;

	if (me->GetActiveWeapon() == nullptr)
		return;

	if (me->GetBehaviorInterface()->ShouldAttack(me, threat) == ANSWER_NO)
		return;

	if (threat->GetEdict() == nullptr)
		return;

	auto mod = CTeamFortress2Mod::GetTF2Mod();
	std::string classname(gamehelpers->GetEntityClassname(me->GetActiveWeapon()));
	auto id = mod->GetWeaponID(classname);

	if (me->GetMyClassType() == TeamFortress2::TFClassType::TFClass_Medic)
	{
		if (id == TeamFortress2::TFWeaponID::TF_WEAPON_MEDIGUN)
		{
			return; // medigun is handled in the medic behavior
		}
	}

	if (me->GetMyClassType() == TeamFortress2::TFClassType::TFClass_Heavy && me->GetAmmoOfIndex(TeamFortress2::TF_AMMO_PRIMARY) <= 50 &&
		me->GetBehaviorInterface()->ShouldHurry(me) != ANSWER_YES)
	{
		constexpr auto THREAT_SPIN_TIME = 3.0f;
		if (me->GetSensorInterface()->GetTimeSinceVisibleThreat() <= THREAT_SPIN_TIME)
		{
			me->GetControlInterface()->PressSecondaryAttackButton(0.25f); // spin the minigun
		}
	}

	if (!threat->WasRecentlyVisible(1.0f)) // stop firing after losing LOS for more than 1 second
		return;

	auto& origin = threat->GetEdict()->GetCollideable()->GetCollisionOrigin();
	auto center = UtilHelpers::getWorldSpaceCenter(threat->GetEdict());
	auto& max = threat->GetEdict()->GetCollideable()->OBBMaxs();
	Vector top = origin;
	top.z += max.z - 1.0f;

	if (!me->IsLineOfFireClear(origin))
	{
		if (!me->IsLineOfFireClear(center))
		{
			if (!me->IsLineOfFireClear(top))
			{
				return;
			}
		}
	}

	auto info = mod->GetWeaponInfo(me->GetActiveWeapon());
	auto threat_range = me->GetRangeTo(origin);

	if (threat_range < info->GetAttackInfo(WeaponInfo::PRIMARY_ATTACK).GetMinRange())
	{
		return; // Don't fire
	}

	if (threat_range > info->GetAttackInfo(WeaponInfo::PRIMARY_ATTACK).GetMaxRange())
	{
		return; // Don't fire
	}

	if (me->GetControlInterface()->IsAimOnTarget())
	{
		me->GetControlInterface()->PressAttackButton();
	}

	if (id == TeamFortress2::TFWeaponID::TF_WEAPON_PIPEBOMBLAUNCHER)
	{
		me->GetControlInterface()->PressSecondaryAttackButton(); // make stickies detonate as soon as possible
	}
}

void CTF2BotMainTask::SelectBestWeaponForEnemy(CTF2Bot* me, CKnownEntity* threat)
{
	if (!AllowedToSwitchWeapon())
		return;

	if (!threat->IsVisibleNow())
		return; // Don't bother with weapon switches if we can't see it
	
	auto allweapons = me->GetAllWeapons();
	auto mod = CTeamFortress2Mod::GetTF2Mod();
	std::vector<std::pair<edict_t*, const WeaponInfo*>> infovec;
	infovec.reserve(MAX_WEAPONS);

	for (auto weaponedict : allweapons)
	{
		auto info = mod->GetWeaponInfoManager().GetWeaponInfoByEconIndex(tf2lib::GetWeaponItemDefinitionIndex(weaponedict));

		if (info)
		{
			std::pair<edict_t*, const WeaponInfo*> pair(weaponedict, info);
			infovec.push_back(pair);
			continue;
		}

		info = mod->GetWeaponInfoManager().GetWeaponInfoByClassname(gamehelpers->GetEntityClassname(weaponedict));

		if (info)
		{
			std::pair<edict_t*, const WeaponInfo*> pair(weaponedict, info);
			infovec.push_back(pair);
		}
	}

	if (infovec.size() == 0) // no weapons?
		return;

	const float threat_range = me->GetRangeTo(threat->GetEdict());

	edict_t* best = [me, threat, &infovec, &threat_range]() -> edict_t* {
		edict_t* current_best = nullptr;
		int current_priority = -99999999;

		for (auto& pair : infovec)
		{
			auto info = pair.second;
			bool can_use = false;

			if (!info->IsCombatWeapon())
				continue; // Skip non-combat weapons

			// most tf2 weapons only deals damage with the primary attack
			if (info->HasPrimaryAttack())
			{
				auto& attack = info->GetAttackInfo(WeaponInfo::AttackFunctionType::PRIMARY_ATTACK);

				if (attack.InRangeTo(threat_range)) // in range to primary attrack
				{
					can_use = true;
				}

				// TO-DO: Ammo Check
			}

			if (!can_use)
				continue;

			if (info->GetPriority() > current_priority) // found usable weapon with a higher priority
			{
				current_priority = info->GetPriority();
				current_best = pair.first;
			}
		}

		return current_best;
	}();

	if (best != me->GetActiveWeapon())
	{
		me->SelectWeaponByCommand(gamehelpers->GetEntityClassname(best)); // switch!
	}
}

void CTF2BotMainTask::UpdateLook(CTF2Bot* me, CKnownEntity* threat)
{
	if (threat)
	{
		if (threat->IsVisibleNow())
		{
			me->GetControlInterface()->AimAt(threat->GetEdict(), IPlayerController::LOOK_COMBAT, 1.0f, "Looking at current threat!"); // look at my enemies
			return;
		}

		if (threat->WasRecentlyVisible(1.5f))
		{
			me->GetControlInterface()->AimAt(threat->GetLastKnownPosition(), IPlayerController::LOOK_ALERT, 1.0f, "Looking at current threat LKP!");
			return;
		}

		// TO-DO: Handle look when the bot has a threat but it's not visible
	}

	// TO-DO: Handle look at potential enemy locations
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
	case TeamFortress2::TFWeaponID::TF_WEAPON_GRENADELAUNCHER:
	{
		const float rangeTo = me->GetRangeTo(player->WorldSpaceCenter());
		const float speed = weaponinfo->GetAttackInfo(WeaponInfo::PRIMARY_ATTACK).GetProjectileSpeed();
		const float time = pred::GetProjectileTravelTime(speed, rangeTo);
		Vector velocity = player->GetAbsVelocity();
		const float velocitymod = RemapValClamped(rangeTo, 600.0f, 1500.0f, 1.0f, 1.5f);
		velocity *= velocitymod;
		Vector targetPos = pred::SimpleProjectileLead(player->GetAbsOrigin(), velocity, speed, rangeTo);
		const float gravmod = RemapValClamped(rangeTo, 600.0f, 1750.0f, 0.9f, 1.2f);
		const float z = pred::SimpleGravityCompensation(time, gravmod);
		targetPos.z += z;

		result = std::move(targetPos);
		return;
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

CKnownEntity* CTF2BotMainTask::InternalSelectTargetThreat(CTF2Bot* me, CKnownEntity* threat1, CKnownEntity* threat2)
{
	// TO-DO: Add threat selection

	auto range1 = me->GetRangeTo(threat1->GetEdict());
	auto range2 = me->GetRangeTo(threat2->GetEdict());

	// temporary distance based for testing
	if (range1 < range2)
	{
		return threat1;
	}

	return threat2;
}

