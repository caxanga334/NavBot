#ifndef __NAVBOT_CSS_BOT_SCENARIO_TASK_H_
#define __NAVBOT_CSS_BOT_SCENARIO_TASK_H_

class CCSSBotScenarioTask : public AITask<CCSSBot>
{
public:
	CCSSBotScenarioTask();

	AITask<CCSSBot>* InitialNextTask(CCSSBot* bot) override;

	TaskResult<CCSSBot> OnTaskUpdate(CCSSBot* bot) override;

	const char* GetName() const override { return "Scenario"; }
private:

	AITask<CCSSBot>* SelectScenarioTask(CCSSBot* bot) const;
};

#endif // !__NAVBOT_CSS_BOT_SCENARIO_TASK_H_
