#ifndef NAVBOT_TF2_BOT_MVM_MONITOR_TASK_H_
#define NAVBOT_TF2_BOT_MVM_MONITOR_TASK_H_

class CTF2Bot;

class CTF2BotMvMMonitorTask : public AITask<CTF2Bot>
{
public:
	CTF2BotMvMMonitorTask()
	{
		m_hasUpgradedInThisWave = false;
		m_myclass = static_cast<TeamFortress2::TFClassType>(0);
	}

	AITask<CTF2Bot>* InitialNextTask(CTF2Bot* bot) override;
	TaskResult<CTF2Bot> OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;
	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;

	TaskEventResponseResult<CTF2Bot> OnRoundStateChanged(CTF2Bot* bot) override;

	const char* GetName() const override { return "MvMMonitor"; }
private:
	bool m_hasUpgradedInThisWave;
	TeamFortress2::TFClassType m_myclass;
	CountdownTimer m_currencyScan;
	CountdownTimer m_engineerSBTimer; // sentry buster timer
};

#endif // !NAVBOT_TF2_BOT_MVM_MONITOR_TASK_H_
