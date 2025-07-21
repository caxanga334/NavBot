#ifndef __NAVBOT_TF2BOT_SCENARIO_MONITOR_TASK_H_
#define __NAVBOT_TF2BOT_SCENARIO_MONITOR_TASK_H_

class CTF2BotScenarioTask : public AITask<CTF2Bot>
{
public:
	AITask<CTF2Bot>* InitialNextTask(CTF2Bot* bot) override;
	TaskResult<CTF2Bot> OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;
	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;

	static AITask<CTF2Bot>* SelectScenarioTask(CTF2Bot* me, const bool skipClassBehavior = false);
	static AITask<CTF2Bot>* SelectClassTask(CTF2Bot* me);

	TaskEventResponseResult<CTF2Bot> OnVoiceCommand(CTF2Bot* bot, CBaseEntity* subject, int command) override;
	TaskEventResponseResult<CTF2Bot> OnTruceChanged(CTF2Bot* bot, const bool enabled) override;
	TaskEventResponseResult<CTF2Bot> OnObjectSapped(CTF2Bot* bot, CBaseEntity* owner, CBaseEntity* saboteur) override;

	const char* GetName() const override { return "Scenario"; }
private:
	CountdownTimer m_respondToTeamMatesTimer;

};


#endif // !__NAVBOT_TF2BOT_SCENARIO_MONITOR_TASK_H_
