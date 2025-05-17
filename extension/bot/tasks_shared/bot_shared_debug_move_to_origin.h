#ifndef NAVBOT_BOT_SHARED_DEBUG_MOVE_TO_ORIGIN_TASK_H_
#define NAVBOT_BOT_SHARED_DEBUG_MOVE_TO_ORIGIN_TASK_H_

#include <extension.h>
#include <bot/basebot.h>
#include <bot/basebot_pathcost.h>
#include <bot/interfaces/path/meshnavigator.h>
#include <sdkports/sdk_timers.h>

/**
 * @brief Shared bot debug task for moving to a specific coordinates.
 * @tparam BT Bot class.
 * @tparam CT Bot path cost class.
 */
template <typename BT, typename CT = CBaseBotPathCost>
class CBotSharedDebugMoveToOriginTask : public AITask<BT>
{
public:
	CBotSharedDebugMoveToOriginTask(BT* bot, const Vector& origin) :
		AITask<BT>(), m_pathCost(bot)
	{
		m_goal = origin;
		m_failCount = 0;
	}

	TaskResult<BT> OnTaskUpdate(BT* bot) override;

	TaskEventResponseResult<BT> OnMoveToFailure(BT* bot, CPath* path, IEventListener::MovementFailureType reason) override;
	TaskEventResponseResult<BT> OnMoveToSuccess(BT* bot, CPath* path) override;

	const char* GetName() const override { return "DebugMoveToOrigin"; }

private:
	CT m_pathCost;
	CMeshNavigator m_nav;
	CountdownTimer m_repathTimer;
	Vector m_goal;
	int m_failCount;
};

template<typename BT, typename CT>
inline TaskResult<BT> CBotSharedDebugMoveToOriginTask<BT, CT>::OnTaskUpdate(BT* bot)
{
	if (m_repathTimer.IsElapsed())
	{
		m_repathTimer.StartRandom(1.0f, 2.0f);

		if (!m_nav.ComputePathToPosition(bot, m_goal, m_pathCost))
		{
			return AITask<BT>::Done("No path to goal!");
		}
	}

	m_nav.Update(bot);

	return AITask<BT>::Continue();
}

template<typename BT, typename CT>
inline TaskEventResponseResult<BT> CBotSharedDebugMoveToOriginTask<BT, CT>::OnMoveToFailure(BT* bot, CPath* path, IEventListener::MovementFailureType reason)
{
	if (++m_failCount > 20)
	{
		return AITask<BT>::TryDone(PRIORITY_HIGH, "Too many path failures!");
	}

	return AITask<BT>::TryContinue();
}

template<typename BT, typename CT>
inline TaskEventResponseResult<BT> CBotSharedDebugMoveToOriginTask<BT, CT>::OnMoveToSuccess(BT* bot, CPath* path)
{
	bot->GetMovementInterface()->DoCounterStrafe();
	return AITask<BT>::TryDone(PRIORITY_HIGH, "Goal reached");
}

#endif // !NAVBOT_BOT_SHARED_DEBUG_MOVE_TO_ORIGIN_TASK_H_
