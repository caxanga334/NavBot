#ifndef NAVBOT_BOT_SHARED_PREREQ_DESTROY_ENT_TASK_H_
#define NAVBOT_BOT_SHARED_PREREQ_DESTROY_ENT_TASK_H_

#include <bot/interfaces/path/meshnavigator.h>
#include <bot/interfaces/sensor_utils.h>
#include <mods/modhelpers.h>
#include <navmesh/nav_prereq.h>

/**
 * @brief Shared bot debug task for moving to a specific coordinates.
 * @tparam BT Bot class.
 * @tparam CT Bot path cost class.
 */
template <typename BT, typename CT>
class CBotSharedPrereqDestroyEntityTask : public AITask<BT>
{
public:
	CBotSharedPrereqDestroyEntityTask(BT* bot, const CNavPrerequisite* prereq) :
		AITask<BT>(), m_pathCost(bot)
	{
		m_prereq = prereq;
		m_useMelee = prereq->GetFloatData() >= 0.5f;
	}

	TaskResult<BT> OnTaskStart(BT* bot, AITask<BT>* pastTask) override;
	TaskResult<BT> OnTaskUpdate(BT* bot) override;

	void OnTaskEnd(BT* bot, AITask<BT>* nextTask) override
	{
		m_pto.Clear();
	}

	TaskEventResponseResult<BT> OnStuck(BT* bot) override
	{
		if (m_counter.Increase())
		{
			return AITask<BT>::TryDone(PRIORITY_HIGH, "Too many path failures!");
		}

		return AITask<BT>::TryToMaintain(PRIORITY_LOW);
	}

	TaskEventResponseResult<BT> OnMoveToFailure(BT* bot, CPath* path, IEventListener::MovementFailureType reason) override;
	TaskEventResponseResult<BT> OnMoveToSuccess(BT* bot, CPath* path) override;

	QueryAnswerType ShouldAttack(CBaseBot* me, const CKnownEntity* them) override;

	const char* GetName() const override { return "DestroyEntity"; }

private:
	CT m_pathCost;
	CMeshNavigator m_nav;
	CPathFailCounter m_counter;
	Vector m_goal;
	const CNavPrerequisite* m_prereq;
	sensorutils::PrimaryThreatOverride<BT> m_pto;
	bool m_useMelee;
};

template<typename BT, typename CT>
inline TaskResult<BT> CBotSharedPrereqDestroyEntityTask<BT, CT>::OnTaskStart(BT* bot, AITask<BT>* pastTask)
{
	CBaseEntity* targetEnt = m_prereq->GetTaskEntity();

	if (!targetEnt)
	{
		return AITask<BT>::Done("Target entity is NULL!");
	}

	m_goal = UtilHelpers::getWorldSpaceCenter(targetEnt);

	return AITask<BT>::Continue();
}


template<typename BT, typename CT>
inline TaskResult<BT> CBotSharedPrereqDestroyEntityTask<BT, CT>::OnTaskUpdate(BT* bot)
{
	CBaseEntity* targetEnt = m_prereq->GetTaskEntity();

	if (!targetEnt)
	{
		return AITask<BT>::Done("Target entity is NULL!");
	}

	if (modhelpers->IsDead(targetEnt))
	{
		return AITask<BT>::Done("Target entity is dead!");
	}

	if (bot->GetMovementInterface()->IsBreakingObstacle())
	{
		return AITask<BT>::Continue();
	}

	// update in case it's a moving entity
	m_goal = UtilHelpers::getWorldSpaceCenter(targetEnt);

	if (m_nav.NeedsRepath())
	{
		m_nav.StartRepathTimer();

		if (!m_nav.ComputePathToPosition(bot, m_goal, m_pathCost))
		{
			if (m_counter.Increase())
			{
				return AITask<BT>::Done("No path to goal!");
			}
		}
	}

	if (!m_useMelee && !m_pto.IsSet())
	{
		m_pto.Set(bot, targetEnt);
	}

	const float range = bot->GetRangeTo(m_goal);

	if (m_useMelee || !bot->GetSensorInterface()->IsAbleToSee(targetEnt) || range >= 256.0f)
	{
		m_nav.Update(bot);
	}

	if (m_useMelee && range <= 128.0f)
	{
		bot->GetMovementInterface()->BreakObstacle(targetEnt);
	}
	
	return AITask<BT>::Continue();
}

template<typename BT, typename CT>
inline TaskEventResponseResult<BT> CBotSharedPrereqDestroyEntityTask<BT, CT>::OnMoveToFailure(BT* bot, CPath* path, IEventListener::MovementFailureType reason)
{
	if (m_counter.Increase())
	{
		return AITask<BT>::TryDone(PRIORITY_HIGH, "Too many path failures!");
	}

	return AITask<BT>::TryToMaintain(PRIORITY_LOW);
}

template<typename BT, typename CT>
inline TaskEventResponseResult<BT> CBotSharedPrereqDestroyEntityTask<BT, CT>::OnMoveToSuccess(BT* bot, CPath* path)
{
	return AITask<BT>::TryToMaintain(PRIORITY_LOW);
}

template<typename BT, typename CT>
inline QueryAnswerType CBotSharedPrereqDestroyEntityTask<BT, CT>::ShouldAttack(CBaseBot* me, const CKnownEntity* them)
{
	CBaseEntity* goalEnt = m_prereq->GetTaskEntity();

	if (!goalEnt)
	{
		return ANSWER_UNDEFINED;
	}

	const CKnownEntity* target = me->GetSensorInterface()->GetKnown(goalEnt);

	if (target)
	{
		if (them == target)
		{
			return ANSWER_YES; // only attack the target entity
		}
		else
		{
			return ANSWER_NO;
		}
	}

	return ANSWER_UNDEFINED;
}

#endif // !NAVBOT_BOT_SHARED_PREREQ_DESTROY_ENT_TASK_H_