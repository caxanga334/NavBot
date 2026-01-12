#include NAVBOT_PCH_FILE
#include <mods/dods/dayofdefeatsourcemod.h>
#include <mods/dods/nav/dods_nav_mesh.h>
#include <mods/dods/nav/dods_nav_waypoint.h>
#include <bot/dods/dodsbot.h>
#include <bot/interfaces/path/meshnavigator.h>
#include <bot/tasks_shared/bot_shared_roam.h>
#include "dodsbot_deploy_mg_task.h"

CDoDSBotDeployMachineGunTask::CDoDSBotDeployMachineGunTask() :
	m_pos(0.0f, 0.0f, 0.0f), m_deployAngle(0.0f, 0.0f, 0.0f), m_atPos(false)
{
	m_waypoint = nullptr;
}

bool CDoDSBotDeployMachineGunTask::IsPossible(CDoDSBot* bot)
{
	return bot->GetInventoryInterface()->HasMachineGun();
}

TaskResult<CDoDSBot> CDoDSBotDeployMachineGunTask::OnTaskStart(CDoDSBot* bot, AITask<CDoDSBot>* pastTask)
{
	// only collect mg waypoints
	auto mg_pred = [](CDoDSBot* bot, CDoDSWaypoint* waypoint) -> bool {
		return waypoint->HasDoDWaypointAttributes(CDoDSWaypoint::DOD_WP_ATTRIBS_MGNEST);
	};

	std::vector<CDoDSWaypoint*> waypoints;
	TheNavMesh->CollectUsableWaypoints<CDoDSBot, CDoDSWaypoint>(bot, waypoints, mg_pred);

	if (waypoints.empty())
	{
		return SwitchTo(new CBotSharedRoamTask<CDoDSBot, CDoDSBotPathCost>(bot, 10000.0f, true), "Failed to find a MG nest position, roaming!");
	}

	m_waypoint = librandom::utils::GetRandomElementFromVector(waypoints);
	m_waypoint->Use(bot, 60.0f); // don't let other bots use this waypoint for a while
	m_pos = m_waypoint->GetRandomPoint();

	if (m_waypoint->GetNumOfAvailableAngles() > 0)
	{
		m_deployAngle = m_waypoint->GetRandomAngle().value();
	}

	return Continue();
}

TaskResult<CDoDSBot> CDoDSBotDeployMachineGunTask::OnTaskUpdate(CDoDSBot* bot)
{
	const CDoDSBotWeapon* mg = bot->GetInventoryInterface()->GetMachineGun();

	if (!mg)
	{
		return Done("I don't have a machine gun!");
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
	}
	else
	{
		if (m_campTimer.HasStarted())
		{
			// the mg is fixed while deployed, don't look around
			bot->GetCombatInterface()->StopLookingAround();
			bot->GetCombatInterface()->DisableDodging();

			if (m_campTimer.IsElapsed())
			{
				bot->GetControlInterface()->PressSecondaryAttackButton();
				return Done("Camp timer elapsed!");
			}
		}
	}

	if (!m_deployingTimer.IsElapsed())
	{
		const CDoDSBotWeapon* activeWeapon = bot->GetInventoryInterface()->GetActiveDoDWeapon();

		if (activeWeapon != mg)
		{
			bot->GetInventoryInterface()->EquipWeapon(mg);
			m_deployingTimer.Start(2.0f);
			return Continue();
		}

		if (m_waypoint->HasDoDWaypointAttributes(CDoDSWaypoint::DOD_WP_ATTRIBS_PRONE))
		{
			if (!bot->IsProne())
			{
				bot->GetControlInterface()->PressProneButton();
			}
		}
		else if (m_waypoint->HasFlags(CWaypoint::BaseFlags::BASEFLAGS_CROUCH))
		{
			bot->GetControlInterface()->PressCrouchButton();
		}

		bot->GetControlInterface()->AimAt(m_deployAngle, IPlayerController::LOOK_CRITICAL, 2.0f, "Looking at MG deploy angle!");

		if (bot->GetControlInterface()->IsAimOnTarget())
		{
			if (!mg->IsMachineGunDeployed())
			{
				bot->GetControlInterface()->PressSecondaryAttackButton();
			}
			else
			{
				const CModSettings* settings = extmanager->GetMod()->GetModSettings();
				m_campTimer.StartRandom(settings->GetCampMinTime(), settings->GetCampMaxTime());
				m_deployingTimer.Invalidate();
				return Continue();
			}
		}
		else
		{
			m_deployingTimer.Start(2.0f);
		}
	}

	return Continue();
}

void CDoDSBotDeployMachineGunTask::OnTaskEnd(CDoDSBot* bot, AITask<CDoDSBot>* nextTask)
{
	if (m_waypoint)
	{
		m_waypoint->StopUsing(bot);
	}
}

TaskEventResponseResult<CDoDSBot> CDoDSBotDeployMachineGunTask::OnMoveToSuccess(CDoDSBot* bot, CPath* path)
{
	m_atPos = true;
	m_deployingTimer.Start(2.0f);
	bot->GetMovementInterface()->DoCounterStrafe();

	return TryContinue();
}

QueryAnswerType CDoDSBotDeployMachineGunTask::ShouldAttack(CBaseBot* me, const CKnownEntity* them)
{
	// Don't attack while deploying
	if (!m_deployingTimer.IsElapsed())
	{
		return ANSWER_NO;
	}

	return ANSWER_UNDEFINED;
}

QueryAnswerType CDoDSBotDeployMachineGunTask::ShouldAssistTeammate(CBaseBot* me, CBaseEntity* teammate)
{
	return ANSWER_NO;
}

QueryAnswerType CDoDSBotDeployMachineGunTask::ShouldSwitchToWeapon(CBaseBot* me, const CBotWeapon* weapon)
{
	if (m_campTimer.HasStarted() || !m_deployingTimer.IsElapsed())
	{
		return ANSWER_NO;
	}

	return ANSWER_UNDEFINED;
}
