#ifndef NAVBOT_BOT_SHARED_PREREQ_MOVE_TO_POS_TASK_H_
#define NAVBOT_BOT_SHARED_PREREQ_MOVE_TO_POS_TASK_H_

#include <extension.h>
#include <bot/basebot.h>
#include <bot/basebot_pathcost.h>
#include <bot/interfaces/path/meshnavigator.h>
#include <sdkports/sdk_timers.h>
#include <navmesh/nav_prereq.h>

/**
 * @brief Shared bot debug task for moving to a specific coordinates.
 * @tparam BT Bot class.
 * @tparam CT Bot path cost class.
 */
template <typename BT, typename CT = CBaseBotPathCost>
class CBotSharedPrereqMoveToPositionTask : public AITask<BT>
{
public:
	CBotSharedPrereqMoveToPositionTask(BT* bot, const CNavPrerequisite* prereq) :
		AITask<BT>(), m_pathCost(bot)
	{
		m_goal = prereq->GetGoalPosition();
		m_failCount = 0;
	}

	TaskResult<BT> OnTaskUpdate(BT* bot) override;

	TaskEventResponseResult<BT> OnStuck(BT* bot) override;
	TaskEventResponseResult<BT> OnMoveToFailure(BT* bot, CPath* path, IEventListener::MovementFailureType reason) override;
	TaskEventResponseResult<BT> OnMoveToSuccess(BT* bot, CPath* path) override;

	const char* GetName() const override { return "MoveToGoalPosition"; }

private:
	CT m_pathCost;
	CMeshNavigator m_nav;
	Vector m_goal;
	int m_failCount;
};

template<typename BT, typename CT>
inline TaskResult<BT> CBotSharedPrereqMoveToPositionTask<BT, CT>::OnTaskUpdate(BT* bot)
{
	if (m_nav.NeedsRepath())
	{
		m_nav.StartRepathTimer();

		if (!m_nav.ComputePathToPosition(bot, m_goal, m_pathCost))
		{
			return AITask<BT>::Done("No path to goal!");
		}
	}

	m_nav.Update(bot);

	return AITask<BT>::Continue();
}

template<typename BT, typename CT>
inline TaskEventResponseResult<BT> CBotSharedPrereqMoveToPositionTask<BT, CT>::OnStuck(BT* bot)
{
	if (++m_failCount > 20)
	{
		return AITask<BT>::TryDone(PRIORITY_HIGH, "Too many path failures!");
	}

	return AITask<BT>::TryContinue();
}

template<typename BT, typename CT>
inline TaskEventResponseResult<BT> CBotSharedPrereqMoveToPositionTask<BT, CT>::OnMoveToFailure(BT* bot, CPath* path, IEventListener::MovementFailureType reason)
{
	if (++m_failCount > 20)
	{
		return AITask<BT>::TryDone(PRIORITY_HIGH, "Too many path failures!");
	}

	return AITask<BT>::TryContinue();
}

template<typename BT, typename CT>
inline TaskEventResponseResult<BT> CBotSharedPrereqMoveToPositionTask<BT, CT>::OnMoveToSuccess(BT* bot, CPath* path)
{
	return AITask<BT>::TryDone(PRIORITY_HIGH, "Goal reached");
}

#endif // !NAVBOT_BOT_SHARED_PREREQ_MOVE_TO_POS_TASK_H_
