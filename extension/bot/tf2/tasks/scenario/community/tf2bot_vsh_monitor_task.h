#ifndef __NAVBOT_TF2BOT_VSH_MONITOR_TASK_H_
#define __NAVBOT_TF2BOT_VSH_MONITOR_TASK_H_

class CTF2BotVSHMonitorTask : public AITask<CTF2Bot>
{
public:
	AITask<CTF2Bot>* InitialNextTask(CTF2Bot* bot) override;

	TaskResult<CTF2Bot> OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;
	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;

	QueryAnswerType ShouldHurry(CBaseBot* me) override
	{
		if (m_isHale)
		{
			return ANSWER_YES;
		}

		return ANSWER_UNDEFINED;
	}

	QueryAnswerType ShouldRetreat(CBaseBot* me) override
	{
		if (m_isHale)
		{
			return ANSWER_NO;
		}

		return ANSWER_UNDEFINED;
	}

	const char* GetName() const override { return "VSHMonitor"; }

private:
	bool m_isHale;
	bool m_isStomping;
	CountdownTimer m_haleAbilityCheckTimer;
	CountdownTimer m_stompCooldownTimer;
	CountdownTimer m_chargeCooldownTimer;
	CountdownTimer m_chargeTimer;

	bool IsForwardJumpPossible(CTF2Bot* bot);
};


#endif // !__NAVBOT_TF2BOT_VSH_MONITOR_TASK_H_
