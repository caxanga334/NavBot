#ifndef __NAVBOT_DODSBOT_TACTICAL_MONITOR_TASK_H_
#define __NAVBOT_DODSBOT_TACTICAL_MONITOR_TASK_H_

class CDoDSBotTacticalMonitorTask : public AITask<CDoDSBot>
{
public:

	AITask<CDoDSBot>* InitialNextTask(CDoDSBot* bot) override;

	TaskResult<CDoDSBot> OnTaskUpdate(CDoDSBot* bot) override;

	TaskEventResponseResult<CDoDSBot> OnNavAreaChanged(CDoDSBot* bot, CNavArea* oldArea, CNavArea* newArea) override;
	TaskEventResponseResult<CDoDSBot> OnInjured(CDoDSBot* bot, const CTakeDamageInfo& info) override;

	const char* GetName() const override { return "TacticalMonitor"; }
private:
	
};

#endif // !__NAVBOT_DODSBOT_TACTICAL_MONITOR_TASK_H_
