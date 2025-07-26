#ifndef __NAVBOT_BOT_SHARED_ROAM_TASK_H_
#define __NAVBOT_BOT_SHARED_ROAM_TASK_H_

#include <extension.h>
#include <util/librandom.h>
#include <bot/basebot.h>
#include <bot/basebot_pathcost.h>
#include <bot/interfaces/path/meshnavigator.h>
#include <sdkports/sdk_timers.h>
#include <navmesh/nav_mesh.h>
#include <navmesh/nav_pathfind.h>
#include <navmesh/nav_waypoint.h>
#include "bot_shared_attack_nearest_enemy.h"

template <typename BT, typename CT = CBaseBotPathCost>
class CBotSharedRoamTask : public AITask<BT>
{
public:
	CBotSharedRoamTask(BT* bot, const float travellimit = 4096.0f, const bool attackvisiblenemies = false) :
		m_pathcost(bot), m_goal(0.0f, 0.0f, 0.0f)
	{
		m_goalSet = false;
		m_attackEnemies = attackvisiblenemies;
		m_travelLimit = travellimit;
		m_moveFailures = 0;
	}

	CBotSharedRoamTask(BT* bot, const Vector& goal, const float timeout = -1.0f, const bool attackvisiblenemies = false) :
		m_pathcost(bot), m_goal(goal)
	{
		if (timeout > 0.0f)
		{
			m_timeout.Start(timeout);
		}

		m_goalSet = true;
		m_attackEnemies = attackvisiblenemies;
		m_moveFailures = 0;
	}

	CBotSharedRoamTask(BT* bot, CWaypoint* waypoint, const float timeout = -1.0f, const bool attackvisiblenemies = false) :
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
	}

	TaskResult<BT> OnTaskStart(BT* bot, AITask<BT>* pastTask) override;
	TaskResult<BT> OnTaskUpdate(BT* bot) override;

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

		if (threat)
		{
			return AITask<BT>::PauseFor(new CBotSharedAttackNearestEnemyTask<BT, CT>(bot), "Attacking visible enemy!");
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
inline TaskEventResponseResult<BT> CBotSharedRoamTask<BT, CT>::OnMoveToFailure(BT* bot, CPath* path, IEventListener::MovementFailureType reason)
{
	if (++m_moveFailures > 20)
	{
		return AITask<BT>::TryDone(PRIORITY_HIGH, "Too many path failures! Giving up!");
	}

	return AITask<BT>::TryContinue(PRIORITY_LOW);
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
		INavAreaCollector<CNavArea> collector;
		collector.SetStartArea(bot->GetLastKnownNavArea());
		collector.SetTravelLimit(m_travelLimit);
		collector.SetSearchLinks(true);
		collector.SetSearchLadders(true);
		collector.SetSearchElevators(true);
		collector.Execute();

		if (collector.IsCollectedAreasEmpty())
		{
			return;
		}

		CNavArea* goal = collector.GetRandomCollectedArea();

		m_goal = goal->GetRandomPoint();
		m_goalSet = true;
		m_timeout.Start(CBaseBot::s_botrng.GetRandomReal<float>(180.0f, 300.0f));
	}
}

#endif // !__NAVBOT_BOT_SHARED_ROAM_TASK_H_
