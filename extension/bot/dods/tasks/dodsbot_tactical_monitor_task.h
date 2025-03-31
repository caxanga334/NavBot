#ifndef __NAVBOT_DODSBOT_TACTICAL_MONITOR_TASK_H_
#define __NAVBOT_DODSBOT_TACTICAL_MONITOR_TASK_H_

class CDoDSBotTacticalMonitorTask : public AITask<CDoDSBot>
{
public:

	AITask<CDoDSBot>* InitialNextTask(CDoDSBot* bot) override;

	TaskResult<CDoDSBot> OnTaskUpdate(CDoDSBot* bot) override;

	const char* GetName() const override { return "TacticalMonitor"; }
private:
	
};

#endif // !__NAVBOT_DODSBOT_TACTICAL_MONITOR_TASK_H_
