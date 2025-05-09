#ifndef __NAVBOT_TF2BOT_SNIPER_PUSH_TASK_H_
#define __NAVBOT_TF2BOT_SNIPER_PUSH_TASK_H_

/**
 * @brief Task for snipers to aggressively push towards the objective.
 */
class CTF2BotSniperPushTask : public AITask<CTF2Bot>
{
public:
	CTF2BotSniperPushTask();
	TaskResult<CTF2Bot> OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;
	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;
	void OnTaskEnd(CTF2Bot* bot, AITask<CTF2Bot>* nextTask) override;

	TaskEventResponseResult<CTF2Bot> OnMoveToSuccess(CTF2Bot* bot, CPath* path) override;

	// Custom attack behavior
	QueryAnswerType ShouldAttack(CBaseBot* me, const CKnownEntity* them) override { return ANSWER_NO; }
	// I'm busy
	QueryAnswerType ShouldAssistTeammate(CBaseBot* me, CBaseEntity* teammate) override { return ANSWER_NO; }

	const char* GetName() const override { return "SniperPush"; }

private:
	CountdownTimer m_endpushtimer;
	CountdownTimer m_repathtimer;
	CountdownTimer m_firetimer;
	CMeshNavigator m_nav;
	Vector m_goal;

	void SelectPushGoal(CTF2Bot* bot);
};


#endif // !__NAVBOT_TF2BOT_SNIPER_PUSH_TASK_H_
