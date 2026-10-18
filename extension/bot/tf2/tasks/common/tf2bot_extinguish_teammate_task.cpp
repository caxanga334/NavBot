#include NAVBOT_PCH_FILE
#include <mods/basemod.h>
#include <mods/tf2/tf2lib.h>
#include <bot/tf2/tf2bot.h>
#include <bot/interfaces/path/chasenavigator.h>
#include "tf2bot_extinguish_teammate_task.h"

static bool IsPlayerCriticallyOnFire(CBaseEntity* entity)
{
	// not on fire
	if (!tf2lib::IsPlayerInCondition(entity, TeamFortress2::TFCond::TFCond_OnFire))
	{
		return false;
	}

	int idx = UtilHelpers::IndexOfEntity(entity);

	// medics on fire are always critical
	if (tf2lib::GetPlayerClassType(idx) == TeamFortress2::TFClassType::TFClass_Medic)
	{
		return true;
	}

	return UtilHelpers::GetEntityHealth(idx) < 75;
}

bool CTF2BotExtinguishTeammateTask::IsPossible(CTF2Bot* bot, CBaseEntity** teammate, CBaseEntity** weapon)
{
	const CKnownEntity* threat = bot->GetSensorInterface()->GetPrimaryKnownThreat(ISensor::ANY_THREATS);

	// if the enemy is too close, priorize attacking them.
	if (threat != nullptr && bot->GetRangeTo(threat->GetLastKnownPosition()) <= 512.0f)
	{
		return false;
	}

	const CTF2BotWeapon* botweapon = bot->GetInventoryInterface()->GetExtinguishWeapon();

	if (botweapon == nullptr)
	{
		return false;
	}

	CBaseEntity* closest = nullptr;
	float bestdist = std::numeric_limits<float>::max();
	bool foundvisible = false;

	auto func = [&closest, &bestdist, &bot, &foundvisible](const CKnownEntity* known) {
		
		CBaseEntity* entity = known->GetEntity();
		float dist = bot->GetRangeTo(entity);

		if (dist > 750.0f)
		{
			// only help nearby teammates
			return;
		}

		if (bot->GetBehaviorInterface()->ShouldAssistTeammate(bot, entity) == ANSWER_NO)
		{
			return;
		}

		if (!IsPlayerCriticallyOnFire(entity))
		{
			return;
		}

		bool isvisible = bot->GetSensorInterface()->IsLineOfSightClear(UtilHelpers::getWorldSpaceCenter(entity));

		if (foundvisible && !isvisible)
		{
			// priorize visible teammates if I found one.
			return;
		}

		if (dist < bestdist)
		{
			bestdist = dist;
			closest = entity;

			if (isvisible)
			{
				foundvisible = true;
			}
		}
	};

	bot->GetSensorInterface()->ForEveryKnownAlly(func);

	if (closest == nullptr)
	{
		return false;
	}

	*teammate = closest;
	*weapon = botweapon->GetEntity();
	return true;
}

CTF2BotExtinguishTeammateTask::CTF2BotExtinguishTeammateTask(CBaseEntity* teammate, CBaseEntity* weapon) :
	m_teammate(teammate), m_weapon(weapon), m_isAirblast(false)
{
}

TaskResult<CTF2Bot> CTF2BotExtinguishTeammateTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	const CBotWeapon* weapon = GetWeapon(bot);

	if (!weapon)
	{
		return Done("I don't have the needed weapon anymore!");
	}

	m_isAirblast = weapon->GetWeaponInfo()->HasTag("can_airblast");
	m_timeout.Start(7.0f);
	return Continue();
}

TaskResult<CTF2Bot> CTF2BotExtinguishTeammateTask::OnTaskUpdate(CTF2Bot* bot)
{
	if (m_timeout.IsElapsed())
	{
		return Done("Task timedout!");
	}

	CBaseEntity* teammate = m_teammate.Get();

	if (teammate == nullptr || modhelpers->IsDead(teammate) || !bot->GetSensorInterface()->IsFriendly(teammate))
	{
		return Done("Teammate is invalid!");
	}

	if (!tf2lib::IsPlayerInCondition(teammate, TeamFortress2::TFCond::TFCond_OnFire))
	{
		return Done("Teammate no longer on fire!");
	}

	const CBotWeapon* weapon = GetWeapon(bot);

	if (weapon == nullptr)
	{
		return Done("I don't have the needed weapon anymore!");
	}

	bool equipped = true;

	if (bot->GetInventoryInterface()->GetActiveBotWeapon() != weapon)
	{
		bot->GetInventoryInterface()->EquipWeapon(weapon);
		equipped = false;
	}

	const float range = bot->GetRangeTo(teammate);
	const bool visible = bot->GetSensorInterface()->IsLineOfSightClear(UtilHelpers::getWorldSpaceCenter(teammate));

	if (equipped)
	{
		auto input = bot->GetControlInterface();

		if (m_isAirblast)
		{
			if (range <= 300.0f && visible)
			{
				input->SetDesiredAimSpot(IDecisionQuery::DesiredAimSpot::AIMSPOT_CENTER);
				input->AimAt(teammate, IPlayerController::LOOK_SUPPORT, 1.0f, "Looking at teammate to extinguish!");

				if (input->IsAimOnTarget())
				{
					input->PressSecondaryAttackButton();
					return Done("Airblasted teammate!");
				}
			}
		}
		else
		{
			if (weapon->IsInAttackRange(range, botweapons::AttackType::PRIMARY) && visible)
			{
				input->SetDesiredAimSpot(IDecisionQuery::DesiredAimSpot::AIMSPOT_ABSORIGIN);
				input->AimAt(teammate, IPlayerController::LOOK_SUPPORT, 1.0f, "Looking at teammate to extinguish!");

				if (input->IsAimOnTarget())
				{
					input->PressAttackButton();
					return Done("Threw jarate/milk at teammate!");
				}
			}
		}
	}

	// avoid walking into them
	if (range >= 96.0f || !visible)
	{
		CTF2BotPathCost cost(bot, RouteType::FASTEST_ROUTE);
		m_nav.Update(bot, teammate, cost);
	}


	return Continue();
}

const CBotWeapon* CTF2BotExtinguishTeammateTask::GetWeapon(const CTF2Bot* bot) const
{
	CBaseEntity* entity = m_weapon.Get();

	if (entity != nullptr)
	{
		const CBotWeapon* weapon = bot->GetInventoryInterface()->GetWeaponOfEntity(entity);
		return weapon;
	}

	return nullptr;
}
