#ifndef NAVBOT_TF2_BOT_MVM_MONITOR_TASK_H_
#define NAVBOT_TF2_BOT_MVM_MONITOR_TASK_H_

class CTF2Bot;

class CTF2BotMvMMonitorTask : public AITask<CTF2Bot>
{
public:
	CTF2BotMvMMonitorTask()
	{
		m_hasUpgradedInThisWave = false;
	}

	AITask<CTF2Bot>* InitialNextTask(CTF2Bot* bot) override;
	TaskResult<CTF2Bot> OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;
	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;

	TaskEventResponseResult<CTF2Bot> OnRoundStateChanged(CTF2Bot* bot) override;

	const char* GetName() const override { return "MvMMonitor"; }
private:
	bool m_hasUpgradedInThisWave;

	CountdownTimer m_currencyScan;

	// cvar cache
	static inline bool s_collect{ false };

	void ScanForDroppedCurrency(std::vector<CHandle<CBaseEntity>>& currencyPacks);
};

#endif // !NAVBOT_TF2_BOT_MVM_MONITOR_TASK_H_
