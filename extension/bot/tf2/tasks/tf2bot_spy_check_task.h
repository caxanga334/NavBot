#ifndef __NAVBOT_TF2_BOT_SPY_CHECK_TASK_H_
#define __NAVBOT_TF2_BOT_SPY_CHECK_TASK_H_

class CTF2BotSpyCheckTask : public AITask<CTF2Bot>
{
public:
	static bool IsPossible(CTF2Bot* bot, CBaseEntity* spy);

	CTF2BotSpyCheckTask(CBaseEntity* target);

	TaskResult<CTF2Bot> OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;
	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;
	void OnTaskEnd(CTF2Bot* bot, AITask<CTF2Bot>* nextTask) override;

	TaskEventResponseResult<CTF2Bot> OnSight(CTF2Bot* bot, CBaseEntity* subject) override;
	TaskEventResponseResult<CTF2Bot> OnLostSight(CTF2Bot* bot, CBaseEntity* subject) override;

	QueryAnswerType ShouldHurry(CBaseBot* me) override { return ANSWER_YES; }

	const char* GetName() const override { return "SpyCheck"; }
private:
	CChaseNavigator m_nav;
	CHandle<CBaseEntity> m_target;
	sensorutils::PrimaryThreatOverride<CTF2Bot> m_pto;
	CountdownTimer m_timeout;
};


#endif // !__NAVBOT_TF2_BOT_SPY_CHECK_TASK_H_
