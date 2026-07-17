#ifndef __NAVBOT_BOT_SHARED_ROAM_TASK_H_
#define __NAVBOT_BOT_SHARED_ROAM_TASK_H_

#include <extension.h>
#include <util/librandom.h>
#include <bot/basebot.h>
#include <bot/basebot_pathcost.h>
#include <bot/bot_shared_utils.h>
#include <bot/interfaces/path/meshnavigator.h>
#include <sdkports/sdk_timers.h>
#include <navmesh/nav_mesh.h>
#include <navmesh/nav_pathfind.h>
#include <navmesh/nav_waypoint.h>
#include "bot_shared_default_combat_tasks.h"

template <typename BT, typename CT>
class CBotSharedRoamTask : public AITask<BT>
{
public:
	/**
	 * @brief This version of the constructor will search for a random nav area or random waypoint.
	 * 
	 * Waypoints are priorized.
	 * @param bot Bot that will be using this task.
	 * @param travellimit Maximum search range for a random nav area.
	 * @param attackvisiblenemies If true, bots will stop and attack visible enemies.
	 * @param waypointSearchRange Maximum search range for a random "roam" flagged waypoint. Negative values for no limit.
	 * @param interruptTask If true, this task will end when it gets interrupted.
	 */
	CBotSharedRoamTask(BT* bot, const float travellimit = 4096.0f, const bool attackvisiblenemies = false, const float waypointSearchRange = -1.0f, const bool interruptTask = false) :
		m_pathcost(bot), m_goal(0.0f, 0.0f, 0.0f)
	{
		m_goalSet = false;
		m_attackEnemies = attackvisiblenemies;
		m_travelLimit = travellimit;
		m_moveFailures = 0;
		m_interrupt = interruptTask;

		CWaypoint* randomWpt = botsharedutils::waypoints::GetRandomRoamWaypoint(bot, waypointSearchRange);

		if (randomWpt)
		{
			m_goal = randomWpt->GetRandomPoint();
			m_goalSet = true;
			randomWpt->Use(bot, 30.0f);
		}
	}
	/**
	 * @brief Makes the bot roams to a specific position.
	 * @param bot Bot that will be using this task.
	 * @param goal The position the bot should roam to.
	 * @param timeout If greater than zero, sets a maximum time allowed to reach the goal position.
	 * @param attackvisiblenemies If true, switch to combat behavior when an enemy is spotted.
	 * @param interruptTask If true, the tasks ends if it's interrupted (IE: paused by another task).
	 */
	CBotSharedRoamTask(BT* bot, const Vector& goal, const float timeout = -1.0f, const bool attackvisiblenemies = false, const bool interruptTask = false) :
		m_pathcost(bot), m_goal(goal)
	{
		if (timeout > 0.0f)
		{
			m_timeout.Start(timeout);
		}

		m_goalSet = true;
		m_attackEnemies = attackvisiblenemies;
		m_moveFailures = 0;
		m_interrupt = interruptTask;
		m_travelLimit = 4096.0f;
	}
	/**
	 * @brief Makes the bot roams to a specific waypoint.
	 * @param bot Bot that will be using this task.
	 * @param waypoint Waypoint to roam to. The waypoint will be marked as used by this bot.
	 * @param timeout If greater than zero, sets a maximum time allowed to reach the goal position.
	 * @param attackvisiblenemies If true, switch to combat behavior when an enemy is spotted.
	 * @param interruptTask If true, the tasks ends if it's interrupted (IE: paused by another task).
	 */
	CBotSharedRoamTask(BT* bot, CWaypoint* waypoint, const float timeout = -1.0f, const bool attackvisiblenemies = false, const bool interruptTask = false) :
		m_pathcost(bot)
	{
		if (timeout > 0.0f)
		{
			m_timeout.Start(timeout);
		}

		m_goal = waypoint->GetRandomPoint();
		waypoint->Use(bot, 30.0f);
		m_goalSet = true;
		m_attackEnemies = attackvisiblenemies;
		m_moveFailures = 0;
		m_interrupt = interruptTask;
	}

	TaskResult<BT> OnTaskStart(BT* bot, AITask<BT>* pastTask) override;
	TaskResult<BT> OnTaskUpdate(BT* bot) override;
	bool OnTaskPause(BT* bot, AITask<BT>* nextTask) override;

	TaskEventResponseResult<BT> OnStuck(BT* bot) override;
	TaskEventResponseResult<BT> OnMoveToFailure(BT* bot, CPath* path, IEventListener::MovementFailureType reason) override;
	TaskEventResponseResult<BT> OnMoveToSuccess(BT* bot, CPath* path) override;

	const char* GetName() const override { return "Roam"; }
private:
	CT m_pathcost;
	CountdownTimer m_timeout;
	CMeshNavigator m_nav;
	Vector m_goal;
	bool m_goalSet;
	bool m_attackEnemies;
	bool m_interrupt;
	float m_travelLimit;
	int m_moveFailures;

	void FindRandomDestination(BT* bot);
};

template<typename BT, typename CT>
inline TaskResult<BT> CBotSharedRoamTask<BT, CT>::OnTaskStart(BT* bot, AITask<BT>* pastTask)
{
	if (randomgen->GetRandomInt<int>(1, 4) == 1)
	{
		m_pathcost.SetRouteType(SAFEST_ROUTE);
	}

	FindRandomDestination(bot);

	if (!m_nav.ComputePathToPosition(bot, m_goal, m_pathcost))
	{
		return AITask<BT>::Done("Failed to build path to destination!");
	}

	m_nav.StartRepathTimer();
	return AITask<BT>::Continue();
}

template<typename BT, typename CT>
inline TaskResult<BT> CBotSharedRoamTask<BT, CT>::OnTaskUpdate(BT* bot)
{
	if (m_timeout.HasStarted() && m_timeout.IsElapsed())
	{
		return AITask<BT>::Done("Task timed out!");
	}

	if (m_attackEnemies)
	{
		const CKnownEntity* threat = bot->GetSensorInterface()->GetPrimaryKnownThreat(true);

		if (threat && threat->GetLastKnownArea() != nullptr && bot->GetMovementInterface()->IsAreaTraversable(threat->GetLastKnownArea()))
		{
			if (bot->GetBehaviorInterface()->ShouldSeekAndDestroy(bot, threat) != ANSWER_NO)
			{
				return AITask<BT>::PauseFor(new CBotSharedDefaultCombatBehaviorTask<BT, CT>(), "Attacking visible enemy!");
			}
		}
	}

	if (m_nav.NeedsRepath())
	{
		m_nav.StartRepathTimer();

		if (!m_nav.ComputePathToPosition(bot, m_goal, m_pathcost))
		{
			if (++m_moveFailures > 20)
			{
				return AITask<BT>::Done("Failed to build path to destination!");
			}
		}
	}

	m_nav.Update(bot);

	return AITask<BT>::Continue();
}

template<typename BT, typename CT>
inline bool CBotSharedRoamTask<BT, CT>::OnTaskPause(BT* bot, AITask<BT>* nextTask)
{
	if (m_interrupt)
	{
		return false;
	}

	return true;
}

template<typename BT, typename CT>
inline TaskEventResponseResult<BT> CBotSharedRoamTask<BT, CT>::OnStuck(BT* bot)
{
	m_nav.Invalidate();
	m_nav.ForceRepath();

	if (++m_moveFailures > 10)
	{
		return AITask<BT>::TryDone(PRIORITY_HIGH, "Too many path failures! Giving up!");
	}

	return AITask<BT>::TryToMaintain(PRIORITY_LOW);
}

template<typename BT, typename CT>
inline TaskEventResponseResult<BT> CBotSharedRoamTask<BT, CT>::OnMoveToFailure(BT* bot, CPath* path, IEventListener::MovementFailureType reason)
{
	if (++m_moveFailures > 10)
	{
		return AITask<BT>::TryDone(PRIORITY_HIGH, "Too many path failures! Giving up!");
	}

	return AITask<BT>::TryToMaintain(PRIORITY_LOW);
}

template<typename BT, typename CT>
inline TaskEventResponseResult<BT> CBotSharedRoamTask<BT, CT>::OnMoveToSuccess(BT* bot, CPath* path)
{
	return AITask<BT>::TryDone(PRIORITY_HIGH, "Goal reached!");
}

template<typename BT, typename CT>
inline void CBotSharedRoamTask<BT, CT>::FindRandomDestination(BT* bot)
{
	if (bot->GetLastKnownNavArea() == nullptr)
	{
		if (bot->IsDebugging(BOTDEBUG_TASKS))
		{
			bot->DebugPrintToConsole(255, 0, 0, "%s CBotSharedRoamTask::FindRandomDestination failed! GetLastKnownNavArea == NULL! \n", bot->GetDebugIdentifier());
		}

		return;
	}

	if (!m_goalSet)
	{
		botsharedutils::RandomDestinationCollector collector(bot, m_travelLimit);
		collector.SetSearchLinks(true);
		collector.SetSearchLadders(true);
		collector.SetSearchElevators(true);
		collector.Execute();

		if (collector.IsCollectedAreasEmpty())
		{
			return;
		}

		CNavArea* goal = nullptr;

		// random chance based on aggressiveness, capped at 50%, to go to the farthest area found.
		if (CBaseBot::s_botrng.GetRandomChance(std::min(bot->GetDifficultyProfile()->GetAggressiveness(), 50)))
		{
			goal = collector.GetFarthestCollectedArea();
		}
		else
		{
			goal = collector.GetRandomCollectedArea();
		}

		m_goal = goal->GetRandomPoint();
		m_goalSet = true;
		m_timeout.Start(CBaseBot::s_botrng.GetRandomReal<float>(180.0f, 300.0f));
	}
}

#endif // !__NAVBOT_BOT_SHARED_ROAM_TASK_H_
