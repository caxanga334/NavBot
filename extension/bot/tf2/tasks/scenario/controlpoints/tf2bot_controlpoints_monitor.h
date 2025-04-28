#ifndef NAVBOT_TF2BOT_CONTROL_POINT_MONITOR_TASK_H_
#define NAVBOT_TF2BOT_CONTROL_POINT_MONITOR_TASK_H_

class CTF2BotControlPointMonitorTask : public AITask<CTF2Bot>
{
public:
	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;

	

	const char* GetName() const override { return "ControlPointMonitor"; }

};


#endif // !NAVBOT_TF2BOT_CONTROL_POINT_MONITOR_TASK_H_
