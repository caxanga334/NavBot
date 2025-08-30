#ifndef __NAVBOT_BOT_SHARED_SEARCH_AREA_TASK_H_
#define __NAVBOT_BOT_SHARED_SEARCH_AREA_TASK_H_

#include <queue>
#include <vector>
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
#include "bot_shared_attack_nearest_enemy.h"

/**
 * @brief General purpose task to make bots search the surrounding areas for enemies.
 * 
 * Will attack enmies if found.
 * @tparam BT Bot Class
 * @tparam CT Path Cost functor class
 */
template <typename BT, typename CT = CBaseBotPathCost>
class CBotSharedSearchAreaTask : public AITask<BT>
{
public:
	/**
	 * @brief Constructor.
	 * @param bot Bot that will be using this task.
	 * @param startPoint Start point for the patrol (generally an enemy last known position).
	 * @param minDistanceFromStart Don't patrol areas this close to the start point.
	 * @param maxPatrolDistance Max distance from the start to patrol.
	 */
	CBotSharedSearchAreaTask(BT* bot, const Vector& startPoint, const float minDistanceFromStart = 750.0f, const float maxPatrolDistance = 3000.0f, const std::size_t maxPatrolAreas = 16U) :
		m_pathcost(bot)
	{
		botsharedutils::CollectPatrolAreas collector{ bot, startPoint, minDistanceFromStart, maxPatrolDistance };

		collector.Execute();

		m_patrolPoints.push(startPoint);

		std::size_t i = 0U;

		for (auto area : collector.GetCollectedAreas())
		{
			m_patrolPoints.push(area->GetRandomPoint());

			if (++i > maxPatrolAreas)
			{
				break;
			}
		}

		m_fails = 0;
	}

	/**
	 * @brief Constructor.
	 * @param bot Bot that will be using this task.
	 * @param patrolPoints A vector of positions the bot should patrol.
	 */
	CBotSharedSearchAreaTask(BT* bot, const std::vector<Vector>& patrolPoints) :
		m_pathcost(bot)
	{
		for (auto& vec : patrolPoints)
		{
			m_patrolPoints.push(vec);
		}

		m_fails = 0;
	}

	TaskResult<BT> OnTaskStart(BT* bot, AITask<BT>* pastTask) override;
	TaskResult<BT> OnTaskUpdate(BT* bot) override;

	TaskEventResponseResult<BT> OnStuck(BT* bot) override;
	TaskEventResponseResult<BT> OnMoveToSuccess(BT* bot, CPath* path) override;
	TaskEventResponseResult<BT> OnMoveToFailure(BT* bot, CPath* path, IEventListener::MovementFailureType reason) override;

	const char* GetName() const override { return "SearchArea"; }
private:
	CT m_pathcost;
	CMeshNavigator m_nav;
	std::queue<Vector> m_patrolPoints;
	int m_fails;
};

template<typename BT, typename CT>
inline TaskResult<BT> CBotSharedSearchAreaTask<BT, CT>::OnTaskStart(BT* bot, AITask<BT>* pastTask)
{
	return AITask<BT>::Continue();
}

template<typename BT, typename CT>
inline TaskResult<BT> CBotSharedSearchAreaTask<BT, CT>::OnTaskUpdate(BT* bot)
{
	const CKnownEntity* threat = bot->GetSensorInterface()->GetPrimaryKnownThreat(ISensor::ONLY_VISIBLE_THREATS);

	if (threat)
	{
		return AITask<BT>::SwitchTo(new CBotSharedAttackNearestEnemyTask<BT, CT>(bot), "Found enemy, attacking!");
	}

	if (m_patrolPoints.empty())
	{
		return AITask<BT>::Done("No more points to patrol!");
	}

	if (m_nav.NeedsRepath())
	{
		const Vector& goal = m_patrolPoints.front();
		
		if (!m_nav.ComputePathToPosition(bot, goal, m_pathcost))
		{
			m_patrolPoints.pop();
			m_nav.Invalidate();
			m_nav.ForceRepath();
		}
		else
		{
			m_nav.StartRepathTimer();
		}
	}

	m_nav.Update(bot);

	return AITask<BT>::Continue();
}

template<typename BT, typename CT>
inline TaskEventResponseResult<BT> CBotSharedSearchAreaTask<BT, CT>::OnStuck(BT* bot)
{
	if (++m_fails > 5)
	{
		m_patrolPoints.pop();
		m_nav.Invalidate();
		m_nav.ForceRepath();
		m_fails = 0;
	}

	return AITask<BT>::TryContinue();
}

template<typename BT, typename CT>
inline TaskEventResponseResult<BT> CBotSharedSearchAreaTask<BT, CT>::OnMoveToSuccess(BT* bot, CPath* path)
{
	m_patrolPoints.pop();
	m_nav.Invalidate();
	m_nav.ForceRepath();
	m_fails = 0;
	return AITask<BT>::TryContinue();
}

template<typename BT, typename CT>
inline TaskEventResponseResult<BT> CBotSharedSearchAreaTask<BT, CT>::OnMoveToFailure(BT* bot, CPath* path, IEventListener::MovementFailureType reason)
{
	if (++m_fails > 5)
	{
		m_patrolPoints.pop();
		m_nav.Invalidate();
		m_nav.ForceRepath();
		m_fails = 0;
	}

	return AITask<BT>::TryContinue();
}

#endif // !__NAVBOT_BOT_SHARED_SEARCH_AREA_TASK_H_