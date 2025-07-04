#include NAVBOT_PCH_FILE
#include <extension.h>
#include <util/helpers.h>
#include <entities/tf2/tf_entities.h>
#include <util/entprops.h>
#include <mods/tf2/tf2lib.h>
#include <mods/tf2/teamfortress2mod.h>
#include <bot/tf2/tf2bot.h>
#include <mods/tf2/nav/tfnavmesh.h>
#include <mods/tf2/nav/tfnav_waypoint.h>
#include "tf2bot_engineer_move_object.h"

CTF2BotEngineerMoveObjectTask::CTF2BotEngineerMoveObjectTask(CBaseEntity* object, const Vector& goal, const bool allowDestroying)
{
	m_goal = goal;
	m_waypoint = nullptr;
	m_angle.Init(0.0f, randomgen->GetRandomReal<float>(0.0f, 359.0f), 0.0f);
	m_building = object;
	m_hasBuilding = false;
	m_canDestroy = allowDestroying;
}

CTF2BotEngineerMoveObjectTask::CTF2BotEngineerMoveObjectTask(CBaseEntity* object, CTFWaypoint* goal, const bool allowDestroying)
{
	m_waypoint = goal;
	m_goal = m_waypoint->GetRandomPoint();
	m_building = object;

	auto angle = m_waypoint->GetRandomAngle();

	m_angle = angle.value_or(QAngle{ 0.0f, 0.0f, 0.0f });

	m_hasBuilding = false;
	m_canDestroy = allowDestroying;
}

TaskResult<CTF2Bot> CTF2BotEngineerMoveObjectTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	CBaseEntity* object = m_building.Get();

	if (object == nullptr)
	{
		return Done("Object is NULL!");
	}

	if (m_canDestroy)
	{
		CTF2BotPathCost cost(bot, FASTEST_ROUTE);
		CMeshNavigator nav;
		const Vector& origin = UtilHelpers::getEntityOrigin(object);
		nav.ComputePathToPosition(bot, origin, cost, 0.0f, true);

		if (nav.GetTravelDistance() > CTeamFortress2Mod::GetTF2Mod()->GetTF2ModSettings()->GetEngineerMoveDestroyBuildingRange())
		{
			tfentities::HBaseObject baseobject(object);
			bot->DestroyBuilding(baseobject.GetType(), baseobject.GetMode());
		}
	}

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotEngineerMoveObjectTask::OnTaskUpdate(CTF2Bot* bot)
{
	CBaseEntity* object = m_building.Get();

	if (object == nullptr)
	{
		return Done("Object is NULL!");
	}

	if (!m_hasBuilding)
	{
		Vector goal = UtilHelpers::getWorldSpaceCenter(object);
		CTF2BotPathCost cost(bot, SAFEST_ROUTE);
		m_nav.Update(bot, goal, cost);

		if (bot->GetRangeTo(object) < 96.0f)
		{
			bot->GetControlInterface()->AimAt(goal, IPlayerController::LOOK_PRIORITY, 1.0f, "Looking at object to pick it!");

			if (bot->GetControlInterface()->IsAimOnTarget())
			{
				bot->GetControlInterface()->PressSecondaryAttackButton();
			}
		}

		CBaseEntity* weapon = bot->GetActiveWeapon();

		if (weapon)
		{
			if (UtilHelpers::FClassnameIs(weapon, "tf_weapon_builder"))
			{
				m_hasBuilding = true;
			}
		}

		return Continue();
	}

	if (bot->GetRangeTo(m_goal) <= 36.0f)
	{
		if (!bot->IsCarryingObject())
		{
			return Done("Dropped building!");
		}

		bot->GetControlInterface()->AimAt(m_angle, IPlayerController::LOOK_PRIORITY, 0.5f, "Looking at direction to place building!");

		if (bot->GetControlInterface()->IsAimOnTarget())
		{
			bot->GetControlInterface()->PressAttackButton();
		}

		if (!m_changeAngleTimer.HasStarted())
		{
			m_changeAngleTimer.Start(1.0f);
		}

		if (m_changeAngleTimer.IsElapsed())
		{
			m_changeAngleTimer.Start(1.0f);
			m_angle.Init(0.0f, randomgen->GetRandomReal<float>(0.0f, 359.0f), 0.0f);
		}
	}
	else
	{
		CTF2BotPathCost cost(bot, SAFEST_ROUTE);
		m_nav.Update(bot, m_goal, cost);
	}

	return Continue();
}

void CTF2BotEngineerMoveObjectTask::OnTaskEnd(CTF2Bot* bot, AITask<CTF2Bot>* nextTask)
{
}
