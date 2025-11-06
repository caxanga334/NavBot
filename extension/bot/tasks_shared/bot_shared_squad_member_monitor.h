#ifndef __NAVBOT_BOT_SHARED_SQUAD_MEMBER_MONITOR_TASK_H_
#define __NAVBOT_BOT_SHARED_SQUAD_MEMBER_MONITOR_TASK_H_

#include <extension.h>
#include <bot/basebot.h>
#include <bot/basebot_pathcost.h>
#include "bot_shared_squad_follow_leader.h"

/**
 * @brief Default main behavior for squad member bots.
 * @tparam BT 
 */
template <typename BT, typename CT = CBaseBotPathCost>
class CBotSharedSquadMemberMonitorTask : public AITask<BT>
{
public:
	CBotSharedSquadMemberMonitorTask(AITask<BT>* exitTask)
	{
		m_exitTask = exitTask;
	}

	~CBotSharedSquadMemberMonitorTask() override;

	AITask<BT>* InitialNextTask(BT* bot) override;
	TaskResult<BT> OnTaskUpdate(BT* bot) override;

	// No. I'm working with a squad
	QueryAnswerType ShouldAssistTeammate(CBaseBot* me, CBaseEntity* teammate) override { return ANSWER_NO; }
	QueryAnswerType ShouldSeekAndDestroy(CBaseBot* me, const CKnownEntity* them) override;

	const char* GetName() const override { return "SquadMemberMonitor"; }
private:
	AITask<BT>* m_exitTask;
};

template<typename BT, typename CT>
inline CBotSharedSquadMemberMonitorTask<BT, CT>::~CBotSharedSquadMemberMonitorTask()
{
	if (m_exitTask != nullptr)
	{
		delete m_exitTask;
	}
}

template<typename BT, typename CT>
inline AITask<BT>* CBotSharedSquadMemberMonitorTask<BT, CT>::InitialNextTask(BT* bot)
{
	return new CBotSharedSquadFollowLeaderTask<BT, CT>(bot);
}

template<typename BT, typename CT>
inline TaskResult<BT> CBotSharedSquadMemberMonitorTask<BT, CT>::OnTaskUpdate(BT* bot)
{
	if (!bot->GetSquadInterface()->IsSquadValid())
	{
		if (m_exitTask != nullptr)
		{
			AITask<BT>* task = m_exitTask;
			m_exitTask = nullptr;
			return AITask<BT>::SwitchTo(task, "No longer in a squad!");
		}
		else
		{
			return AITask<BT>::Done("No longer in a squad!");
		}
	}

	return AITask<BT>::Continue();
}

template<typename BT, typename CT>
inline QueryAnswerType CBotSharedSquadMemberMonitorTask<BT, CT>::ShouldSeekAndDestroy(CBaseBot* me, const CKnownEntity* them)
{
	const CBotWeapon* activeWeapon = me->GetInventoryInterface()->GetActiveBotWeapon();
	ISquad* squad = me->GetSquadInterface();

	if (!activeWeapon || !squad->IsSquadValid())
	{
		return ANSWER_UNDEFINED;
	}

	const float range = (me->GetAbsOrigin() - them->GetLastKnownPosition()).Length();
	const WeaponAttackFunctionInfo& info = activeWeapon->GetWeaponInfo()->GetAttackInfo(WeaponInfo::AttackFunctionType::PRIMARY_ATTACK);
	
	if (info.IsMelee())
	{
		if (info.HasMaxRange() && info.GetMaxRange() * 3.0f <= range)
		{
			return ANSWER_YES;
		}
		else
		{
			return ANSWER_NO;
		}
	}
	else
	{
		if (range > activeWeapon->GetWeaponInfo()->GetAttackRange())
		{
			return ANSWER_NO;
		}

		const float leaderrange = (me->GetAbsOrigin() - squad->GetSquad()->GetSquadLeaderPosition()).Length();

		if (leaderrange >= range)
		{
			return ANSWER_NO;
		}
	}

	return ANSWER_UNDEFINED;
}

#endif // !__NAVBOT_BOT_SHARED_SQUAD_MEMBER_MONITOR_TASK_H_
