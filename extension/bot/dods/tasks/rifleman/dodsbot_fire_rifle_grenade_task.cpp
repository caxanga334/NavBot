#include NAVBOT_PCH_FILE
#include <mods/dods/dayofdefeatsourcemod.h>
#include <mods/dods/nav/dods_nav_mesh.h>
#include <mods/dods/nav/dods_nav_waypoint.h>
#include <bot/dods/dodsbot.h>
#include <bot/interfaces/path/meshnavigator.h>
#include "dodsbot_fire_rifle_grenade_task.h"

CDoDSBotFireRifleGrenadeTask::CDoDSBotFireRifleGrenadeTask(CDoDSWaypoint* waypoint) :
	m_waypoint(waypoint), m_aimAngle(0.0f, 0.0f, 0.0f), m_pos(0.0f, 0.0f, 0.0f), m_atPos(false)
{
}

bool CDoDSBotFireRifleGrenadeTask::IsPossible(CDoDSBot* bot, CDoDSWaypoint** waypoint, int index)
{
	const CDoDSBotWeapon* weapon = bot->GetInventoryInterface()->GetRifleGrenade();

	if (!weapon || weapon->IsOutOfAmmo(bot))
	{
		return false;
	}

	auto rg_pred = [index](CDoDSBot* bot, CDoDSWaypoint* waypoint) -> bool {
		if (waypoint->GetAssignedControlPoint() != dayofdefeatsource::INVALID_CONTROL_POINT)
		{
			if (waypoint->GetAssignedControlPoint() != index)
			{
				return false;
			}
		}

		return waypoint->HasDoDWaypointAttributes(CDoDSWaypoint::DOD_WP_ATTRIBS_USE_RIFLEGREN);
	};


	std::vector<CDoDSWaypoint*> points;
	TheNavMesh->CollectUsableWaypoints(bot, points, rg_pred);

	if (points.empty())
	{
		return false;
	}

	*waypoint = librandom::utils::GetRandomElementFromVector(points);
	return true;
}

TaskResult<CDoDSBot> CDoDSBotFireRifleGrenadeTask::OnTaskStart(CDoDSBot* bot, AITask<CDoDSBot>* pastTask)
{
	if (!m_waypoint)
	{
		return Done("NULL waypoint!");
	}

	m_waypoint->Use(bot, 90.0f);
	m_pos = m_waypoint->GetRandomPoint();

	auto angle = m_waypoint->GetRandomAngle();

	if (!angle.has_value())
	{
		return Done("Waypoint doesn't have any angles!");
	}

	m_aimAngle = angle.value();

	return Continue();
}

TaskResult<CDoDSBot> CDoDSBotFireRifleGrenadeTask::OnTaskUpdate(CDoDSBot* bot)
{
	CDoDSBotInventory* inventory = bot->GetInventoryInterface();
	const CDoDSBotWeapon* riflegren = inventory->GetRifleGrenade();

	if (!riflegren)
	{
		return Done("I don't own a rifle grenade!");
	}

	if (riflegren->IsOutOfAmmo(bot))
	{
		return Done("I'm out of rifle grenade ammo!");
	}

	if (!m_atPos)
	{
		if (!m_nav.IsValid() || m_nav.NeedsRepath())
		{
			CDoDSBotPathCost cost{ bot };
			m_nav.ComputePathToPosition(bot, m_pos, cost);
			m_nav.StartRepathTimer();
		}

		m_nav.Update(bot);
		return Continue();
	}

	bot->GetCombatInterface()->StopLookingAround();
	bot->GetCombatInterface()->DisableDodging();

	const CDoDSBotWeapon* activeWeapon = inventory->GetActiveDoDWeapon();

	if (activeWeapon != riflegren)
	{
		inventory->EquipWeapon(riflegren);
		return Continue();
	}

	if (!m_fireDelay.IsElapsed())
	{
		return Continue();
	}

	CDoDSBotPlayerController* input = bot->GetControlInterface();
	input->AimAt(m_aimAngle, IPlayerController::LOOK_CRITICAL, 1.0f, "Looking at angle to fire rifle grenade!");
	
	if (input->IsAimOnTarget())
	{
		bot->GetCombatInterface()->DoPrimaryAttack();
		m_fireDelay.Start(0.5f);
	}

	return Continue();
}

void CDoDSBotFireRifleGrenadeTask::OnTaskEnd(CDoDSBot* bot, AITask<CDoDSBot>* nextTask)
{
	if (m_waypoint)
	{
		m_waypoint->StopUsing(bot);
	}
}

TaskEventResponseResult<CDoDSBot> CDoDSBotFireRifleGrenadeTask::OnMoveToSuccess(CDoDSBot* bot, CPath* path)
{
	m_atPos = true;
	bot->GetMovementInterface()->DoCounterStrafe();

	return TryContinue();
}

QueryAnswerType CDoDSBotFireRifleGrenadeTask::ShouldAttack(CBaseBot* me, const CKnownEntity* them)
{
	if (m_atPos)
	{
		return ANSWER_NO;
	}

	return ANSWER_UNDEFINED;
}

QueryAnswerType CDoDSBotFireRifleGrenadeTask::ShouldAssistTeammate(CBaseBot* me, CBaseEntity* teammate)
{
	if (m_atPos)
	{
		return ANSWER_NO;
	}

	return ANSWER_UNDEFINED;
}

QueryAnswerType CDoDSBotFireRifleGrenadeTask::ShouldSwitchToWeapon(CBaseBot* me, const CBotWeapon* weapon)
{
	if (m_atPos)
	{
		return ANSWER_NO;
	}

	return ANSWER_UNDEFINED;
}
