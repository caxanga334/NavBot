#ifndef NAVBOT_BOT_SHARED_PREREQ_USE_ENTITY_TASK_H_
#define NAVBOT_BOT_SHARED_PREREQ_USE_ENTITY_TASK_H_

#include <extension.h>
#include <util/helpers.h>
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
class CBotSharedPrereqUseEntityTask : public AITask<BT>
{
public:
	CBotSharedPrereqUseEntityTask(BT* bot, const CNavPrerequisite* prereq) :
		AITask<BT>(), m_pathCost(bot)
	{
		m_prereq = prereq;
		m_failCount = 0;
	}

	TaskResult<BT> OnTaskStart(BT* bot, AITask<BT>* pastTask) override;
	TaskResult<BT> OnTaskUpdate(BT* bot) override;

	TaskEventResponseResult<BT> OnMoveToFailure(BT* bot, CPath* path, IEventListener::MovementFailureType reason) override;
	TaskEventResponseResult<BT> OnMoveToSuccess(BT* bot, CPath* path) override;

	const char* GetName() const override { return "UseEntity"; }

private:
	CT m_pathCost;
	CMeshNavigator m_nav;
	CountdownTimer m_repathTimer;
	CountdownTimer m_lookDelay;
	Vector m_goal;
	int m_failCount;
	const CNavPrerequisite* m_prereq;
};

template<typename BT, typename CT>
inline TaskResult<BT> CBotSharedPrereqUseEntityTask<BT, CT>::OnTaskStart(BT* bot, AITask<BT>* pastTask)
{
	CBaseEntity* useEnt = m_prereq->GetTaskEntity();

	if (!useEnt)
	{
		return AITask<BT>::Done("Use entity is NULL!");
	}

	m_goal = UtilHelpers::getWorldSpaceCenter(useEnt);
	m_lookDelay.Invalidate();

	return AITask<BT>::Continue();
}


template<typename BT, typename CT>
inline TaskResult<BT> CBotSharedPrereqUseEntityTask<BT, CT>::OnTaskUpdate(BT* bot)
{
	CBaseEntity* useEnt = m_prereq->GetTaskEntity();

	if (!useEnt)
	{
		return AITask<BT>::Done("Use entity is NULL!");
	}

	Vector entPos = UtilHelpers::getWorldSpaceCenter(useEnt);

	if (m_repathTimer.IsElapsed())
	{
		m_repathTimer.StartRandom(1.0f, 2.0f);

		if (!m_nav.ComputePathToPosition(bot, m_goal, m_pathCost))
		{
			return AITask<BT>::Done("No path to goal!");
		}
	}

	Vector eyePos = bot->GetEyeOrigin();
	float range = (eyePos - entPos).Length();

	if (range < CBaseExtPlayer::PLAYER_USE_RADIUS)
	{
		bot->GetControlInterface()->AimAt(entPos, IPlayerController::LOOK_VERY_IMPORTANT, 0.2f, "Looking at prerequisite task entity!");

		if (bot->IsLookingTowards(entPos, 0.98f))
		{
			if (!m_lookDelay.HasStarted())
			{
				m_lookDelay.Start(0.250f);
			}
			else if (m_lookDelay.IsElapsed())
			{
				bot->GetControlInterface()->PressUseButton();
				return AITask<BT>::Done("Button pressed!");
			}
		}
	}
	else
	{
		m_nav.Update(bot);
	}

	return AITask<BT>::Continue();
}

template<typename BT, typename CT>
inline TaskEventResponseResult<BT> CBotSharedPrereqUseEntityTask<BT, CT>::OnMoveToFailure(BT* bot, CPath* path, IEventListener::MovementFailureType reason)
{
	if (++m_failCount > 20)
	{
		return AITask<BT>::TryDone(PRIORITY_HIGH, "Too many path failures!");
	}

	return AITask<BT>::TryContinue();
}

template<typename BT, typename CT>
inline TaskEventResponseResult<BT> CBotSharedPrereqUseEntityTask<BT, CT>::OnMoveToSuccess(BT* bot, CPath* path)
{
	return AITask<BT>::TryContinue();
}

#endif // !NAVBOT_BOT_SHARED_PREREQ_USE_ENTITY_TASK_H_