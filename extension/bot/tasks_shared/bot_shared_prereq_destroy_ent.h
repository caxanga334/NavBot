#ifndef NAVBOT_BOT_SHARED_PREREQ_DESTROY_ENT_TASK_H_
#define NAVBOT_BOT_SHARED_PREREQ_DESTROY_ENT_TASK_H_

#include <extension.h>
#include <util/helpers.h>
#include <bot/basebot.h>
#include <bot/basebot_pathcost.h>
#include <bot/interfaces/path/meshnavigator.h>
#include <sdkports/sdk_timers.h>
#include <sdkports/sdk_traces.h>
#include <navmesh/nav_prereq.h>

/**
 * @brief Shared bot debug task for moving to a specific coordinates.
 * @tparam BT Bot class.
 * @tparam CT Bot path cost class.
 */
template <typename BT, typename CT = CBaseBotPathCost>
class CBotSharedPrereqDestroyEntityTask : public AITask<BT>
{
public:
	CBotSharedPrereqDestroyEntityTask(BT* bot, const CNavPrerequisite* prereq) :
		AITask<BT>(), m_pathCost(bot)
	{
		m_prereq = prereq;
		m_failCount = 0;
		m_inRange = false;
	}

	TaskResult<BT> OnTaskStart(BT* bot, AITask<BT>* pastTask) override;
	TaskResult<BT> OnTaskUpdate(BT* bot) override;

	TaskEventResponseResult<BT> OnMoveToFailure(BT* bot, CPath* path, IEventListener::MovementFailureType reason) override;
	TaskEventResponseResult<BT> OnMoveToSuccess(BT* bot, CPath* path) override;

	QueryAnswerType ShouldAttack(CBaseBot* me, const CKnownEntity* them) override;

	const char* GetName() const override { return "UseEntity"; }

private:
	CT m_pathCost;
	CMeshNavigator m_nav;
	CountdownTimer m_repathTimer;
	Vector m_goal;
	int m_failCount;
	const CNavPrerequisite* m_prereq;
	bool m_inRange;
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
	CKnownEntity* known = bot->GetSensorInterface()->AddKnownEntity(targetEnt);

	if (known)
	{
		known->UpdatePosition();
	}

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

	if (!UtilHelpers::IsEntityAlive(targetEnt))
	{
		return AITask<BT>::Done("Target entity is dead!");
	}

	if (m_repathTimer.IsElapsed())
	{
		m_repathTimer.StartRandom(1.0f, 2.0f);

		if (!m_nav.ComputePathToPosition(bot, m_goal, m_pathCost))
		{
			return AITask<BT>::Done("No path to goal!");
		}
	}

	CKnownEntity* known = bot->GetSensorInterface()->AddKnownEntity(targetEnt);

	if (known)
	{
		known->UpdatePosition();
	}
	else
	{
		return AITask<BT>::Done("NULL known entity of target!");
	}

	bot->GetInventoryInterface()->SelectBestWeaponForThreat(known);
	Vector eyePos = bot->GetEyeOrigin();
	Vector entPos = UtilHelpers::getWorldSpaceCenter(targetEnt);
	const float range = (eyePos - entPos).Length();
	m_inRange = (m_prereq->GetFloatData() < 0.5f || range <= m_prereq->GetFloatData());

	auto passFunc = [targetEnt](IHandleEntity* pHandleEntity, int contentsMask) -> bool {
		CBaseEntity* pEntity = trace::EntityFromEntityHandle(pHandleEntity);

		if (pEntity == targetEnt)
		{
			return false;
		}

		return true;
	};

	trace::CTraceFilterSimple filter(bot->GetEntity(), COLLISION_GROUP_NONE);
	trace_t tr;
	trace::line(eyePos, entPos, MASK_SHOT, &filter, tr);

	if (m_inRange && tr.fraction == 1.0f)
	{
		bot->FireWeaponAtEnemy(known, true);
	}
	else
	{
		m_nav.Update(bot);
	}

	return AITask<BT>::Continue();
}

template<typename BT, typename CT>
inline TaskEventResponseResult<BT> CBotSharedPrereqDestroyEntityTask<BT, CT>::OnMoveToFailure(BT* bot, CPath* path, IEventListener::MovementFailureType reason)
{
	if (++m_failCount > 20)
	{
		return AITask<BT>::TryDone(PRIORITY_HIGH, "Too many path failures!");
	}

	return AITask<BT>::TryContinue();
}

template<typename BT, typename CT>
inline TaskEventResponseResult<BT> CBotSharedPrereqDestroyEntityTask<BT, CT>::OnMoveToSuccess(BT* bot, CPath* path)
{
	return AITask<BT>::TryContinue();
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