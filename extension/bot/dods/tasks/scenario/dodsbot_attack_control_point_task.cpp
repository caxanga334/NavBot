#include NAVBOT_PCH_FILE
#include <vector>
#include <extension.h>
#include <util/helpers.h>
#include <util/entprops.h>
#include <sdkports/sdk_ehandle.h>
#include <sdkports/sdk_timers.h>
#include <mods/dods/dayofdefeatsourcemod.h>
#include <mods/dods/nav/dods_nav_waypoint.h>
#include <mods/dods/dodslib.h>
#include <bot/dods/dodsbot.h>
#include <bot/interfaces/path/meshnavigator.h>
#include <bot/dods/tasks/rifleman/dodsbot_fire_rifle_grenade_task.h>
#include "dodsbot_attack_control_point_task.h"
#include "dodsbot_fetch_bomb_task.h"
#include "dodsbot_deploy_bomb_task.h"
#include <bot/tasks_shared/bot_shared_defend_spot.h>
#include <bot/tasks_shared/bot_shared_roam.h>
#include <bot/tasks_shared/bot_shared_move_to_brush_entity.h>

CDoDSBotAttackControlPointTask::CDoDSBotAttackControlPointTask(const CDayOfDefeatSourceMod::DoDControlPoint* controlpoint)
{
	m_controlpoint = controlpoint;
}

bool CDoDSBotAttackControlPointTask::IsPossible(CDoDSBot* bot, const CDayOfDefeatSourceMod::DoDControlPoint** controlpoint)
{
	std::vector<const CDayOfDefeatSourceMod::DoDControlPoint*> points;
	points.reserve(MAX_CONTROL_POINTS);
	dayofdefeatsource::DoDTeam team = bot->GetMyDoDTeam();

	CDayOfDefeatSourceMod::GetDODMod()->CollectControlPointsToAttack(team, points);

	if (points.empty())
	{
		return false;
	}

	if (controlpoint)
	{
		// no need to NULL check here, CollectControlPointsToAttack returns early if it's NULL so the points.empty() will catch it
		const CDODObjectiveResource* objres = CDayOfDefeatSourceMod::GetDODMod()->GetDODObjectiveResource();

		for (auto point : points)
		{
			if (team == dayofdefeatsource::DoDTeam::DODTEAM_ALLIES)
			{
				// Someone from my team is trying to cap this point and there isn't enough players to cap it, priorize the first point found
				if (objres->GetAlliesRequiredCappers(point->index) > 1 && objres->GetNumAlliesOnPoint(point->index) > 0 &&
					objres->GetNumAlliesOnPoint(point->index) < objres->GetAlliesRequiredCappers(point->index))
				{
					*controlpoint = point;
					return true;
				}
			}
			else
			{
				if (objres->GetAxisRequiredCappers(point->index) > 1 && objres->GetNumAxisOnPoint(point->index) > 0 &&
					objres->GetNumAxisOnPoint(point->index) < objres->GetAxisRequiredCappers(point->index))
				{
					*controlpoint = point;
					return true;
				}
			}
		}

		if (points.size() == 1U)
		{
			*controlpoint = points[0];
		}
		else
		{
			*controlpoint = points[CBaseBot::s_botrng.GetRandomInt<std::size_t>(0U, points.size() - 1U)];
		}
	}

	return true;
}

AITask<CDoDSBot>* CDoDSBotAttackControlPointTask::InitialNextTask(CDoDSBot* bot)
{
	CBaseEntity* trigger = m_controlpoint->capture_trigger.Get();
	return new CBotSharedMoveToBrushEntityTask<CDoDSBot, CDoDSBotPathCost, false>(bot, trigger); // it's safe if trigger is NULL, the task will just end
}

TaskResult<CDoDSBot> CDoDSBotAttackControlPointTask::OnTaskStart(CDoDSBot* bot, AITask<CDoDSBot>* pastTask)
{
	const CDODObjectiveResource* objres = CDayOfDefeatSourceMod::GetDODMod()->GetDODObjectiveResource();
	CBaseEntity* pTrigger = m_controlpoint->capture_trigger.Get();

	if (!pTrigger)
	{
		if (!objres || objres->GetNumBombsRequired(m_controlpoint->index) == 0)
		{
			return Done("Can't capture target control point!");
		}
	}

	if (objres && objres->GetNumBombsRemaining(m_controlpoint->index) > 0)
	{
		// need bombs

		CBaseEntity* target = nullptr;
		int myteam = static_cast<int>(bot->GetMyDoDTeam());

		for (auto& handle : m_controlpoint->bomb_targets)
		{
			CBaseEntity* ent = handle.Get();

			if (!ent)
			{
				continue;
			}

			if (dodslib::CanPlantBombAtTarget(ent) && dodslib::CanTeamPlantBombAtTarget(ent, myteam))
			{
				target = ent;
				break;
			}
		}

		// no bomb to plant
		if (!target)
		{
			// all bomb targets are unavailable
			for (auto& handle : m_controlpoint->bomb_targets)
			{
				CBaseEntity* ent = handle.Get();

				if (!ent)
				{
					continue;
				}

				int state = 0;
				entprops->GetEntProp(handle.GetEntryIndex(), Prop_Send, "m_iState", state);

				if (state == static_cast<int>(dayofdefeatsource::DoDBombTargetState::BOMB_TARGET_ACTIVE))
				{
					target = ent;
					break;
				}
			}

			// found planted bomb, defending it!
			if (target)
			{
				const Vector& pos = UtilHelpers::getEntityOrigin(target);
				return SwitchTo(new CBotSharedDefendSpotTask<CDoDSBot, CDoDSBotPathCost>(bot, pos, 15.0f, true), "Defending armed bomb!");
			}
			else
			{
				// No bomb target found

				if (pTrigger && dodslib::IsTeamAllowedToCapture(pTrigger, bot->GetMyDoDTeam()))
				{
					// if a capture trigger exists, go cap it
					return Continue();
				}

				// No point to attack, no bomb to plant or defend
				return SwitchTo(new CBotSharedRoamTask<CDoDSBot, CDoDSBotPathCost>(bot, 4096.0f, true), "Nothing to do, roaming!");
			}
		}

		if (bot->GetInventoryInterface()->HasBomb())
		{
			return SwitchTo(new CDoDSBotDeployBombTask(target), "Bombing the control point!");
		}

		CDoDSBotDeployBombTask* task = new CDoDSBotDeployBombTask(target);
		return SwitchTo(new CDoDSBotFetchBombTask(task), "Going to fetch a bomb to plant at the control point!");
	}

	// Double check we can actually capture this control point
	if (pTrigger && !dodslib::IsTeamAllowedToCapture(pTrigger, bot->GetMyDoDTeam()))
	{
		return SwitchTo(new CBotSharedRoamTask<CDoDSBot, CDoDSBotPathCost>(bot, 4096.0f, true), "Nothing to do, roaming!");
	}

	CDoDSWaypoint* waypoint = nullptr;

	if (CDoDSBotFireRifleGrenadeTask::IsPossible(bot, &waypoint, m_controlpoint->index))
	{
		return PauseFor(new CDoDSBotFireRifleGrenadeTask(waypoint), "Opportunistically using rifle grenades.");
	}

	return Continue();
}

TaskResult<CDoDSBot> CDoDSBotAttackControlPointTask::OnTaskUpdate(CDoDSBot* bot)
{
	CBaseEntity* trigger = m_controlpoint->capture_trigger.Get();

	if (trigger == nullptr || m_controlpoint->point.Get() == nullptr || m_controlpoint->index == dayofdefeatsource::INVALID_CONTROL_POINT)
	{
		return Done("Invalid control point!");
	}

	const CDODObjectiveResource* objres = CDayOfDefeatSourceMod::GetDODMod()->GetDODObjectiveResource();

	if (objres)
	{
		if (objres->GetOwnerTeamIndex(m_controlpoint->index) == static_cast<int>(bot->GetMyDoDTeam()) ||
			(objres->GetNumBombsRequired(m_controlpoint->index) > 0 && objres->GetNumBombsRemaining(m_controlpoint->index) == 0))
		{
			// my team owns it or all bombs exploded
			return Done("Control point has been captured!");
		}
	}

	if (GetNextTask() == nullptr)
	{
		return Done("Failed to reach the control point!");
	}

	/*
	if (bot->GetControlPointIndex() == m_controlpoint->index)
	{
		return Continue(); // capturing the point
	}

	if (m_nav.NeedsRepath())
	{
		m_nav.StartRepathTimer();
		Vector goal = UtilHelpers::getWorldSpaceCenter(trigger);
		CDoDSBotPathCost cost(bot);
		m_nav.ComputePathToPosition(bot, goal, cost);
	}

	m_nav.Update(bot);

	*/

	return Continue();
}
