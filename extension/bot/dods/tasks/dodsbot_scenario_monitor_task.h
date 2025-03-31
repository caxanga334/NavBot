#ifndef __NAVBOT_DODSBOT_SCENARIO_MONITOR_TASK_H_
#define __NAVBOT_DODSBOT_SCENARIO_MONITOR_TASK_H_

class CDoDSBotScenarioMonitorTask : public AITask<CDoDSBot>
{
public:
	TaskResult<CDoDSBot> OnTaskUpdate(CDoDSBot* bot) override;

	const char* GetName() const override { return "ScenarioMonitor"; }
private:

};

#endif // !__NAVBOT_DODSBOT_SCENARIO_MONITOR_TASK_H_
