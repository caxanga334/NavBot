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
	CBotSharedSquadFollowLeaderTask(BT* bot, const float followrange = 400.0f) :
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
	const ISquad::SquadMember* leader = bot->GetSquadInterface()->GetSquadLeader();

	if (!leader || !leader->IsValid())
	{
		AITask<BT>::Continue();
	}

	const Vector& goal = UtilHelpers::getEntityOrigin(leader->handle.Get());

	if (bot->GetRangeTo(goal) > m_followrange)
	{
		m_nav.Update(bot, goal, m_pathcost);
	}

	return AITask<BT>::Continue();
}

template<typename BT, typename CT>
inline TaskEventResponseResult<BT> CBotSharedSquadFollowLeaderTask<BT, CT>::OnSight(BT* bot, CBaseEntity* subject)
{
	if (subject && !bot->GetSensorInterface()->IsIgnored(subject) && bot->GetSensorInterface()->IsEnemy(subject))
	{
		bot->GetSquadInterface()->NotifyVisibleEnemy(subject);
	}

	return AITask<BT>::TryContinue();
}

#endif // !__NAVBOT_BOT_SHARED_SQUAD_FOLLOW_LEADER_TASK_H_