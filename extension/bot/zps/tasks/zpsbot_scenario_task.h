#ifndef __NAVBOT_ZPSBOT_SCENARIO_TASK_H_
#define __NAVBOT_ZPSBOT_SCENARIO_TASK_H_

class CZPSBotScenarioTask : public AITask<CZPSBot>
{
public:
	CZPSBotScenarioTask();

	static AITask<CZPSBot>* SelectScenarioTask(CZPSBot* bot);

	AITask<CZPSBot>* InitialNextTask(CZPSBot* bot) override;
	TaskResult<CZPSBot> OnTaskStart(CZPSBot* bot, AITask<CZPSBot>* pastTask) override;
	TaskResult<CZPSBot> OnTaskUpdate(CZPSBot* bot) override;
	TaskResult<CZPSBot> OnTaskResume(CZPSBot* bot, AITask<CZPSBot>* pastTask) override;

	QueryAnswerType ShouldHurry(CBaseBot* me) override;
	QueryAnswerType ShouldRetreat(CBaseBot* me) override;

	const char* GetName() const override { return "Scenario"; }

private:
	bool m_roundisactive;
	CountdownTimer m_ammoSearchTimer;
	CountdownTimer m_weaponSearchTimer;
	CountdownTimer m_healthSearchTimer;
	CountdownTimer m_armorSearchTimer;
};

#endif // !__NAVBOT_ZPSBOT_SCENARIO_TASK_H_
