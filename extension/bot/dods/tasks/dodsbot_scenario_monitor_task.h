#ifndef __NAVBOT_DODSBOT_SCENARIO_MONITOR_TASK_H_
#define __NAVBOT_DODSBOT_SCENARIO_MONITOR_TASK_H_

class CDoDSBotScenarioMonitorTask : public AITask<CDoDSBot>
{
public:
	AITask<CDoDSBot>* InitialNextTask(CDoDSBot* bot) override;
	TaskResult<CDoDSBot> OnTaskStart(CDoDSBot* bot, AITask<CDoDSBot>* pastTask) override;
	TaskResult<CDoDSBot> OnTaskUpdate(CDoDSBot* bot) override;

	TaskEventResponseResult<CDoDSBot> OnNavAreaChanged(CDoDSBot* bot, CNavArea* oldArea, CNavArea* newArea) override;
	TaskEventResponseResult<CDoDSBot> OnRoundStateChanged(CDoDSBot* bot) override;
	TaskEventResponseResult<CDoDSBot> OnPathStatusChanged(CDoDSBot* bot) override;

	const char* GetName() const override { return "ScenarioMonitor"; }
private:

	void FindControlPointToDefend(CDoDSBot* bot, CBaseEntity** defuse, const CDayOfDefeatSourceMod::DoDControlPoint** defend);
};

#endif // !__NAVBOT_DODSBOT_SCENARIO_MONITOR_TASK_H_
