#ifndef __NAVBOT_TF2BOT_RD_MONITOR_TASK_H_
#define __NAVBOT_TF2BOT_RD_MONITOR_TASK_H_

class CTF2BotRobotDestructionMonitorTask : public AITask<CTF2Bot>
{
public:

	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;

	const char* GetName() const override { return "RobotDestructionMonitor"; }

private:

};

#endif // !__NAVBOT_TF2BOT_RD_MONITOR_TASK_H_
