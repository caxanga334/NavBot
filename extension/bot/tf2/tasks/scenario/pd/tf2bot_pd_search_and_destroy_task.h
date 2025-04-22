#ifndef __NAVBOT_TF2BOT_PD_SEARCH_AND_DESTROY_TASK_H_
#define __NAVBOT_TF2BOT_PD_SEARCH_AND_DESTROY_TASK_H_

class CTF2BotPDSearchAndDestroyTask : public AITask<CTF2Bot>
{
public:
	TaskResult<CTF2Bot> OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;
	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;

	TaskEventResponseResult<CTF2Bot> OnMoveToSuccess(CTF2Bot* bot, CPath* path) override;

	const char* GetName() const override { return "PDSearchAndDestroy"; }
private:
	Vector m_goal;
	CountdownTimer m_checkpointstimer;
	CountdownTimer m_checkdelivertimer;
	CountdownTimer m_repathtimer;
	CMeshNavigator m_nav;

	bool SelectRandomGoal(CTF2Bot* bot);
};

#endif // !__NAVBOT_TF2BOT_PD_SEARCH_AND_DESTROY_TASK_H_
