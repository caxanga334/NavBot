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
	if (!bot->GetSquadInterface()->IsInASquad())
	{
		if (m_exitTask != nullptr)
		{
			AITask<BT>* task = m_exitTask;
			m_exitTask = nullptr;
			return AITask<BT>::SwitchTo(task, "No longer in a squad!");
		}
		else
		{
			AITask<BT>::Done("No longer in a squad!");
		}
	}

	return AITask<BT>::Continue();
}

#endif // !__NAVBOT_BOT_SHARED_SQUAD_MEMBER_MONITOR_TASK_H_
