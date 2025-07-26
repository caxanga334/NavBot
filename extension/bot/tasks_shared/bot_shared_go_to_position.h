#ifndef __NAVBOT_BOT_SHARED_GO_TO_POSITION_TASK_H_
#define __NAVBOT_BOT_SHARED_GO_TO_POSITION_TASK_H_

#include <string>
#include <extension.h>
#include <util/librandom.h>
#include <bot/basebot.h>
#include <bot/basebot_pathcost.h>
#include <bot/interfaces/path/meshnavigator.h>
#include <sdkports/sdk_timers.h>
#include <navmesh/nav_mesh.h>
#include <navmesh/nav_pathfind.h>
#include <navmesh/nav_waypoint.h>
#include "bot_shared_attack_enemy.h"

/**
 * @brief General purpose task for making the bot go to a given position.
 * 
 * Will exit early if no path can be found. Has a built-in timer to end the task if the bot fails to reach the task goal.
 * @tparam BT Bot class.
 * @tparam CT Path cost class.
 */
template <typename BT, typename CT = CBaseBotPathCost>
class CBotSharedGoToPositionTask : public AITask<BT>
{
public:
	/**
	 * @brief Constructor.
	 * @param bot Bot that will use this task.
	 * @param goal Position the bot will move to.
	 * @param name The task name for debug commands. NULL for default.
	 * @param attackEnemies If true, this task will be paused to attack any enemies found.
	 * @param isPriority If true, this task won't be interrupted by gameplay related events.
	 * @param fastestPath If true, use the fastest path.
	 */
	CBotSharedGoToPositionTask(BT* bot, const Vector& goal, const char* name = nullptr, const bool attackEnemies = true, const bool isPriority = false, const bool fastestPath = false) :
		m_pathcost(bot), m_attackEnemies(attackEnemies), m_priority(isPriority)
	{

		if (name)
		{
			m_taskdebugname.assign(name);
		}
		else
		{
			m_taskdebugname.assign("GoToPosition");
		}
		
		m_goal = goal;

		if (fastestPath)
		{
			m_pathcost.SetRouteType(FASTEST_ROUTE);
		}
		else
		{
			if (!CBaseBot::s_botrng.GetRandomChance(bot->GetDifficultyProfile()->GetAggressiveness()))
			{
				m_pathcost.SetRouteType(SAFEST_ROUTE);
			}
		}
	}

	TaskResult<BT> OnTaskStart(BT* bot, AITask<BT>* pastTask) override;
	TaskResult<BT> OnTaskUpdate(BT* bot) override;
	TaskResult<BT> OnTaskResume(BT* bot, AITask<BT>* pastTask) override;

	TaskEventResponseResult<BT> OnMoveToSuccess(BT* bot, CPath* path) override;
	TaskEventResponseResult<BT> OnFlagTaken(BT* bot, CBaseEntity* player) override;
	TaskEventResponseResult<BT> OnFlagDropped(BT* bot, CBaseEntity* player) override;
	TaskEventResponseResult<BT> OnControlPointCaptured(BT* bot, CBaseEntity* point) override;
	TaskEventResponseResult<BT> OnControlPointLost(BT* bot, CBaseEntity* point) override;
	TaskEventResponseResult<BT> OnControlPointContested(BT* bot, CBaseEntity* point) override;

	const char* GetName() const override { return m_taskdebugname.c_str(); }
private:
	std::string m_taskdebugname;
	CT m_pathcost;
	CountdownTimer m_timeout;
	CMeshNavigator m_nav;
	Vector m_goal;
	bool m_attackEnemies;
	bool m_priority;
};

template<typename BT, typename CT>
inline TaskResult<BT> CBotSharedGoToPositionTask<BT, CT>::OnTaskStart(BT* bot, AITask<BT>* pastTask)
{
	if (!m_nav.ComputePathToPosition(bot, m_goal, m_pathcost, 0.0f, true))
	{
		return AITask<BT>::Done("Failed to compute path to goal position!");
	}

	float speed = bot->GetMovementInterface()->GetWalkSpeed();
	float time = m_nav.GetTravelDistance() / speed;

	if (time < 10.0f)
	{
		time = 20.0f;
	}
	else
	{
		time *= 1.8f; // add a buffer
	}

	m_timeout.Start(time);
	m_nav.StartRepathTimer();

	return AITask<BT>::Continue();
}

template<typename BT, typename CT>
inline TaskResult<BT> CBotSharedGoToPositionTask<BT, CT>::OnTaskUpdate(BT* bot)
{
	const CKnownEntity* threat = bot->GetSensorInterface()->GetPrimaryKnownThreat(true);

	if (threat)
	{
		return AITask<BT>::PauseFor(new CBotSharedAttackEnemyTask<BT, CT>(bot), "Attacking visible enemy!");
	}

	if (m_timeout.IsElapsed())
	{
		return AITask<BT>::Done("Failed to reach goal within a reasonable time!");
	}

	if (m_nav.NeedsRepath())
	{
		m_nav.StartRepathTimer();
		m_nav.ComputePathToPosition(bot, m_goal, m_pathcost, 0.0f, true);
	}

	m_nav.Update(bot);

	return AITask<BT>::Continue();
}

template<typename BT, typename CT>
inline TaskResult<BT> CBotSharedGoToPositionTask<BT, CT>::OnTaskResume(BT* bot, AITask<BT>* pastTask)
{
	if (!m_nav.ComputePathToPosition(bot, m_goal, m_pathcost, 0.0f, true))
	{
		return AITask<BT>::Done("Failed to compute path to goal position!");
	}

	float speed = bot->GetMovementInterface()->GetWalkSpeed();
	float time = m_nav.GetTravelDistance() / speed;

	if (time < 5.0f)
	{
		time = 10.0f;
	}
	else
	{
		time *= 1.5f; // add a buffer
	}

	m_timeout.Start(time);
	m_nav.StartRepathTimer();

	return AITask<BT>::Continue();
}

template<typename BT, typename CT>
inline TaskEventResponseResult<BT> CBotSharedGoToPositionTask<BT, CT>::OnMoveToSuccess(BT* bot, CPath* path)
{
	const CKnownEntity* threat = bot->GetSensorInterface()->GetPrimaryKnownThreat(true);

	if (threat)
	{
		return AITask<BT>::TrySwitchTo(new CBotSharedAttackEnemyTask<BT, CT>(bot), EventResultPriorityType::PRIORITY_HIGH, "Goal reached, attacking visible enemy!");
	}

	return AITask<BT>::TryDone(EventResultPriorityType::PRIORITY_HIGH, "Goal reached!");
}

template<typename BT, typename CT>
inline TaskEventResponseResult<BT> CBotSharedGoToPositionTask<BT, CT>::OnFlagTaken(BT* bot, CBaseEntity* player)
{
	if (m_priority)
	{
		return AITask<BT>::TryContinue();
	}

	return AITask<BT>::TryDone(EventResultPriorityType::PRIORITY_HIGH, "Responding to priority event!");
}

template<typename BT, typename CT>
inline TaskEventResponseResult<BT> CBotSharedGoToPositionTask<BT, CT>::OnFlagDropped(BT* bot, CBaseEntity* player)
{
	if (m_priority)
	{
		return AITask<BT>::TryContinue();
	}

	return AITask<BT>::TryDone(EventResultPriorityType::PRIORITY_HIGH, "Responding to priority event!");
}

template<typename BT, typename CT>
inline TaskEventResponseResult<BT> CBotSharedGoToPositionTask<BT, CT>::OnControlPointCaptured(BT* bot, CBaseEntity* point)
{
	if (m_priority)
	{
		return AITask<BT>::TryContinue();
	}

	return AITask<BT>::TryDone(EventResultPriorityType::PRIORITY_HIGH, "Responding to priority event!");
}

template<typename BT, typename CT>
inline TaskEventResponseResult<BT> CBotSharedGoToPositionTask<BT, CT>::OnControlPointLost(BT* bot, CBaseEntity* point)
{
	if (m_priority)
	{
		return AITask<BT>::TryContinue();
	}

	return AITask<BT>::TryDone(EventResultPriorityType::PRIORITY_HIGH, "Responding to priority event!");
}

template<typename BT, typename CT>
inline TaskEventResponseResult<BT> CBotSharedGoToPositionTask<BT, CT>::OnControlPointContested(BT* bot, CBaseEntity* point)
{
	if (m_priority)
	{
		return AITask<BT>::TryContinue();
	}

	return AITask<BT>::TryDone(EventResultPriorityType::PRIORITY_HIGH, "Responding to priority event!");
}

#endif // !__NAVBOT_BOT_SHARED_GO_TO_POSITION_TASK_H_