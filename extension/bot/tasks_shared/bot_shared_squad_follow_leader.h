#ifndef __NAVBOT_BOT_SHARED_SQUAD_FOLLOW_LEADER_TASK_H_
#define __NAVBOT_BOT_SHARED_SQUAD_FOLLOW_LEADER_TASK_H_

#include <extension.h>
#include <util/helpers.h>
#include <bot/basebot.h>
#include <bot/basebot_pathcost.h>
#include <bot/interfaces/path/meshnavigator.h>
#include <sdkports/sdk_timers.h>
#include "bot_shared_default_combat_tasks.h"

/**
 * @brief Task for bots following their squad leader.
 * @tparam BT Bot class.
 * @tparam CT Path cost class.
 */
template <typename BT, typename CT>
class CBotSharedSquadFollowLeaderTask : public AITask<BT>
{
public:
	CBotSharedSquadFollowLeaderTask(BT* bot, const float followrange = 200.0f) :
		m_pathcost(bot), m_followrange(followrange)
	{
	}

	TaskResult<BT> OnTaskUpdate(BT* bot) override;

	TaskEventResponseResult<BT> OnSight(BT* bot, CBaseEntity* subject) override;

	TaskEventResponseResult<BT> OnStuck(BT* bot) override;
	TaskEventResponseResult<BT> OnMoveToFailure(BT* bot, CPath* path, IEventListener::MovementFailureType reason) override;

	const char* GetName() const override { return "FollowSquadLeader"; }
private:
	CT m_pathcost;
	float m_followrange;
	CMeshNavigatorAutoRepath m_nav;
	CPathFailCounter m_counter;
};

template<typename BT, typename CT>
inline TaskResult<BT> CBotSharedSquadFollowLeaderTask<BT, CT>::OnTaskUpdate(BT* bot)
{
	const CKnownEntity* threat = bot->GetSensorInterface()->GetPrimaryKnownThreat(true);

	if (threat && threat->GetLastKnownArea() != nullptr && bot->GetMovementInterface()->IsAreaTraversable(threat->GetLastKnownArea()))
	{
		if (bot->GetBehaviorInterface()->ShouldSeekAndDestroy(bot, threat) != ANSWER_NO)
		{
			return AITask<BT>::PauseFor(new CBotSharedDefaultCombatBehaviorTask<BT, CT>(), "Attacking visible enemy!");
		}
	}

	ISquad* squadiface = bot->GetSquadInterface();

	if (!squadiface->IsSquadValid())
	{
		return AITask<BT>::Continue();
	}

	const ISquad::Member* leader = squadiface->GetSquadData()->GetSquadLeader();

	if (!leader || !leader->IsValid())
	{
		return AITask<BT>::Continue();
	}

	const Vector& goal = squadiface->GetSquadData()->GetSquadLeaderPosition();

	if (bot->GetRangeTo(goal) > m_followrange)
	{
		m_nav.Update(bot, goal, m_pathcost);
	}

	return AITask<BT>::Continue();
}

template<typename BT, typename CT>
inline TaskEventResponseResult<BT> CBotSharedSquadFollowLeaderTask<BT, CT>::OnSight(BT* bot, CBaseEntity* subject)
{
	return AITask<BT>::TryContinue();
}

template<typename BT, typename CT>
inline TaskEventResponseResult<BT> CBotSharedSquadFollowLeaderTask<BT, CT>::OnStuck(BT* bot)
{
	if (m_counter.Increase())
	{
		bot->GetSquadInterface()->DestroySquad();
		m_counter.Reset();
	}

	return AITask<BT>::TryToMaintain();
}

template<typename BT, typename CT>
inline TaskEventResponseResult<BT> CBotSharedSquadFollowLeaderTask<BT, CT>::OnMoveToFailure(BT* bot, CPath* path, IEventListener::MovementFailureType reason)
{
	// OnMoveToFailure is fired when the bot cannot find a complete path to the squad leader.
	// If there are too many failures, give up and leave the squad.
	if (m_counter.Increase())
	{
		bot->GetSquadInterface()->DestroySquad();
		m_counter.Reset();
	}

	return AITask<BT>::TryToMaintain();
}

#endif // !__NAVBOT_BOT_SHARED_SQUAD_FOLLOW_LEADER_TASK_H_