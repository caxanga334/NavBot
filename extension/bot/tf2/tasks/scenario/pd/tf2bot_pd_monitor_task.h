#ifndef __NAVBOT_TF2BOT_PD_MONITOR_TASK_H_
#define __NAVBOT_TF2BOT_PD_MONITOR_TASK_H_

class CTF2BotPDMonitorTask : public AITask<CTF2Bot>
{
public:
	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;

	const char* GetName() const override { return "PlayerDestruction"; }
};

#endif // !__NAVBOT_TF2BOT_PD_MONITOR_TASK_H_
