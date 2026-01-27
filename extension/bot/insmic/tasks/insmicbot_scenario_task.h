#ifndef __NAVBOT_INSMICBOT_SCENARIO_TASK_H_
#define __NAVBOT_INSMICBOT_SCENARIO_TASK_H_

class CInsMICBotScenarioTask : public AITask<CInsMICBot>
{
public:
	CInsMICBotScenarioTask();

	AITask<CInsMICBot>* InitialNextTask(CInsMICBot* bot) override;
	TaskResult<CInsMICBot> OnTaskStart(CInsMICBot* bot, AITask<CInsMICBot>* pastTask) override;
	TaskResult<CInsMICBot> OnTaskUpdate(CInsMICBot* bot) override;

	const char* GetName() const override { return "Scenario"; }
};


#endif // !__NAVBOT_INSMICBOT_SCENARIO_TASK_H_
