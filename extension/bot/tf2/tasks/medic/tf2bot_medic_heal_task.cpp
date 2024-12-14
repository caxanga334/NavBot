#include <vector>
#include <limits>

#include <extension.h>
#include <util/helpers.h>
#include <bot/tf2/tf2bot.h>
#include <mods/tf2/teamfortress2mod.h>
#include <mods/tf2/tf2lib.h>
#include <entities/tf2/tf_entities.h>
#include "tf2bot_medic_retreat_task.h"
#include "tf2bot_medic_revive_task.h"
#include "tf2bot_medic_heal_task.h"

#undef max
#undef min
#undef clamp

CTF2BotMedicHealTask::CTF2BotMedicHealTask() :
	m_moveGoal(0.0f, 0.0f, 0.0f)
{
}

TaskResult<CTF2Bot> CTF2BotMedicHealTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	UpdateFollowTarget(bot);
	m_patientScanTimer.Invalidate();
	return Continue();
}

TaskResult<CTF2Bot> CTF2BotMedicHealTask::OnTaskUpdate(CTF2Bot* bot)
{
	UpdateFollowTarget(bot);
	UpdateHealTarget(bot);

	CBaseEntity* patient = m_healTarget.Get();

	if (patient == nullptr)
	{
		return Continue();
	}

	auto threat = bot->GetSensorInterface()->GetPrimaryKnownThreat(true);

	UpdateMovePosition(bot, threat.get());
	EquipMedigun(bot);

	Vector center = UtilHelpers::getWorldSpaceCenter(patient);
	bot->GetControlInterface()->AimAt(center, IPlayerController::LOOK_SUPPORT, 0.2f, "Looking at patient to heal them!");

	if (bot->GetControlInterface()->IsAimOnTarget())
	{
		bot->GetControlInterface()->PressAttackButton(0.3f);
	}
	else
	{
		bot->GetControlInterface()->ReleaseAllAttackButtons();
	}

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotMedicHealTask::OnTaskResume(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	return Continue();
}

QueryAnswerType CTF2BotMedicHealTask::ShouldSwitchToWeapon(CBaseBot* me, const CBotWeapon* weapon)
{
	// Only allow the bot to switch to the medigun

	if (weapon->GetWeaponInfo()->GetSlot() == static_cast<int>(TeamFortress2::TFWeaponSlot::TFWeaponSlot_Secondary))
	{
		return ANSWER_YES;
	}

	return ANSWER_NO;
}

void CTF2BotMedicHealTask::UpdateFollowTarget(CTF2Bot* bot)
{
	CBaseEntity* follow = m_followTarget.Get();

	if (follow != nullptr)
	{
		if (UtilHelpers::IsEntityAlive(gamehelpers->EntityToBCompatRef(follow)))
		{
			return; // Follow target until death
		}
	}

	CBaseEntity* nextFollowTarget = nullptr;
	float best = std::numeric_limits<float>::max();

	UtilHelpers::ForEachPlayer([&bot, &best, &nextFollowTarget](int client, edict_t* entity, SourceMod::IGamePlayer* player) {

		if (client != bot->GetIndex() && player->IsInGame() && tf2lib::GetEntityTFTeam(client) == bot->GetMyTFTeam() && UtilHelpers::IsPlayerAlive(client))
		{
			auto theirclass = tf2lib::GetPlayerClassType(client);
			float distance = bot->GetRangeToSqr(entity);

			// never follow these classes
			switch (theirclass)
			{
			case TeamFortress2::TFClass_Scout:
				[[fallthrough]];
			case TeamFortress2::TFClass_Sniper:
				[[fallthrough]];
			case TeamFortress2::TFClass_Medic:
				[[fallthrough]];
			case TeamFortress2::TFClass_Spy:
				[[fallthrough]];
			case TeamFortress2::TFClass_Engineer:
				return;
			case TeamFortress2::TFClass_Soldier:
				[[fallthrough]];
			case TeamFortress2::TFClass_Heavy:
				distance *= 0.75f; // prefer following soldiers and heavies
				break;
			default:
				break;
			}

			if (distance < best)
			{
				best = distance;
				nextFollowTarget = entity->GetIServerEntity()->GetBaseEntity();
			}
		}
	});

	if (nextFollowTarget != nullptr)
	{
		m_followTarget = nextFollowTarget;
	}
}

void CTF2BotMedicHealTask::UpdateHealTarget(CTF2Bot* bot)
{
	CBaseEntity* heal = m_healTarget.Get();

	if (heal != nullptr)
	{
		int entindex = gamehelpers->EntityToBCompatRef(heal);

		if (UtilHelpers::IsEntityAlive(entindex) && (tf2lib::GetPlayerHealthPercentage(entindex) < 0.99f || 
			tf2lib::IsPlayerInCondition(entindex, TeamFortress2::TFCond_Ubercharged)))
		{
			return;
		}
	}
	else if (heal == nullptr)
	{
		m_healTarget = m_followTarget.Get();
		return;
	}

	if (!m_patientScanTimer.IsElapsed())
	{
		return;
	}

	CBaseEntity* nextHealTarget = nullptr;
	float best = std::numeric_limits<float>::max();

	UtilHelpers::ForEachPlayer([&bot, &best, &nextHealTarget](int client, edict_t* entity, SourceMod::IGamePlayer* player) {

		if (client != bot->GetIndex() && player->IsInGame() && UtilHelpers::IsPlayerAlive(client))
		{
			auto theirclass = tf2lib::GetPlayerClassType(client);

			if (tf2lib::GetEntityTFTeam(client) == tf2lib::GetEnemyTFTeam(bot->GetMyTFTeam()))
			{
				if (theirclass != TeamFortress2::TFClassType::TFClass_Spy)
				{
					return; // not a spy, don't heal
				}

				// player is from enemy team and is a spy.

				auto& spy = bot->GetSpyMonitorInterface()->GetKnownSpy(entity);

				if (spy.GetDetectionLevel() != CTF2BotSpyMonitor::SpyDetectionLevel::DETECTION_FOOLED)
				{
					return; // only heal spies that fooled me
				}
			}


			float distance = bot->GetRangeToSqr(entity);

			if (tf2lib::IsPlayerInCondition(client, TeamFortress2::TFCond_Cloaked))
			{
				return;
			}

			if (distance >= 1024.0f * 1024.0f)
			{
				return;
			}

			if (!bot->GetSensorInterface()->IsLineOfSightClear(entity))
			{
				return;
			}

			float hp = tf2lib::GetPlayerHealthPercentage(client);

			if (tf2lib::IsPlayerInCondition(client, TeamFortress2::TFCond_OnFire) ||
				tf2lib::IsPlayerInCondition(client, TeamFortress2::TFCond_Bleeding) ||
				tf2lib::IsPlayerInCondition(client, TeamFortress2::TFCond_Jarated) ||
				tf2lib::IsPlayerInCondition(client, TeamFortress2::TFCond_Milked))
			{
				distance *= 0.25f; // priorize players in these conditions
			}
			else if (hp > 0.99f)
			{
				distance *= 5.0f;
			}
			else
			{
				float mult = std::clamp(hp, 0.1f, 1.0f);
				distance *= mult; // multiply distance based on health percentage
			}

			if (distance < best)
			{
				best = distance;
				nextHealTarget = entity->GetIServerEntity()->GetBaseEntity();
			}
		}
	});

	if (nextHealTarget == nullptr)
	{
		m_healTarget = m_followTarget.Get();
	}
	else
	{
		m_healTarget = nextHealTarget;
	}

	m_patientScanTimer.Start(0.5f);
}

void CTF2BotMedicHealTask::UpdateMovePosition(CTF2Bot* bot, const CKnownEntity* threat)
{
	tfentities::HTFBaseEntity patient(m_healTarget.Get());
	auto myweapon = bot->GetInventoryInterface()->GetActiveTFWeapon();
	float moveRange = 150.0f; // default medigun range is around 450
	
	if (myweapon.get() != nullptr)
	{
		moveRange = (myweapon->GetTF2Info()->GetAttackInfo(WeaponInfo::PRIMARY_ATTACK).GetMaxRange() * 0.35f);
	}
	else
	{
		bot->GetInventoryInterface()->RequestRefresh();
	}
	
	float rangeToGoal = moveRange * moveRange;

	Vector start = bot->GetAbsOrigin();
	Vector end = patient.GetAbsOrigin();

	// no enemies, move to the patient
	if (threat == nullptr)
	{
		m_moveGoal = end;
	}
	else // visible enemies
	{
		// direction from the patient to the threat
		Vector dir = (end - threat->GetLastKnownPosition());
		dir.NormalizeInPlace();
		m_moveGoal = end + (dir * moveRange); // position myself behind my patient in relation to the threat
		CNavArea* area = TheNavMesh->GetNearestNavArea(m_moveGoal, moveRange * 2.0f);
		
		if (area != nullptr)
		{
			area->GetClosestPointOnArea(m_moveGoal, &m_moveGoal);
		}

		rangeToGoal = 30.0f * 30.0f;
	}

	float distance = bot->GetRangeToSqr(m_moveGoal);

	if (!bot->GetSensorInterface()->IsAbleToSee(patient.WorldSpaceCenter()) || distance > rangeToGoal)
	{
		CTF2BotPathCost cost(bot);
		m_nav.Update(bot, m_moveGoal, cost);
	}
}

void CTF2BotMedicHealTask::EquipMedigun(CTF2Bot* me)
{
	auto myweapon = me->GetInventoryInterface()->GetActiveBotWeapon();

	if (myweapon && myweapon->GetWeaponInfo()->GetSlot() == static_cast<int>(TeamFortress2::TFWeaponSlot::TFWeaponSlot_Secondary))
	{
		return;
	}

	CBaseEntity* medigun = me->GetWeaponOfSlot(static_cast<int>(TeamFortress2::TFWeaponSlot::TFWeaponSlot_Secondary));

	if (medigun != nullptr)
	{
		me->SelectWeapon(medigun);
	}
}

float CTF2BotMedicHealTask::GetUbercharge(CTF2Bot* me)
{
	auto medigun = me->GetInventoryInterface()->GetActiveBotWeapon();

	if (medigun && medigun->GetWeaponInfo()->GetSlot() == static_cast<int>(TeamFortress2::TFWeaponSlot::TFWeaponSlot_Secondary))
	{
		return tf2lib::GetMedigunUberchargePercentage(medigun->GetIndex());
	}

	return 0.0f;
}