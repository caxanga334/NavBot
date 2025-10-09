#ifndef __NAVBOT_BOT_SHARED_SQUAD_FOLLOW_LEADER_TASK_H_
#define __NAVBOT_BOT_SHARED_SQUAD_FOLLOW_LEADER_TASK_H_

#include <extension.h>
#include <util/helpers.h>
#include <bot/basebot.h>
#include <bot/basebot_pathcost.h>
#include <bot/interfaces/path/meshnavigator.h>
#include <sdkports/sdk_timers.h>

/**
 * @brief Task for bots following their squad leader.
 * @tparam BT Bot class.
 * @tparam CT Path cost class.
 */
template <typename BT, typename CT = CBaseBotPathCost>
class CBotSharedSquadFollowLeaderTask : public AITask<BT>
{
public:
	CBotSharedSquadFollowLeaderTask(BT* bot, const float followrange = 200.0f) :
		m_pathcost(bot), m_followrange(followrange)
	{
	}

	TaskResult<BT> OnTaskUpdate(BT* bot) override;

	TaskEventResponseResult<BT> OnSight(BT* bot, CBaseEntity* subject) override;

	const char* GetName() const override { return "FollowSquadLeader"; }
private:
	CT m_pathcost;
	float m_followrange;
	CMeshNavigatorAutoRepath m_nav;
};

template<typename BT, typename CT>
inline TaskResult<BT> CBotSharedSquadFollowLeaderTask<BT, CT>::OnTaskUpdate(BT* bot)
{
	ISquad* squadiface = bot->GetSquadInterface();

	if (!squadiface->IsSquadValid())
	{
		return AITask<BT>::Continue();
	}

	const ISquad::SquadMember* leader = squadiface->GetSquad()->GetSquadLeader();

	if (!leader || !leader->IsValid())
	{
		return AITask<BT>::Continue();
	}

	Vector goal = leader->GetPosition();

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

#endif // !__NAVBOT_BOT_SHARED_SQUAD_FOLLOW_LEADER_TASK_H_