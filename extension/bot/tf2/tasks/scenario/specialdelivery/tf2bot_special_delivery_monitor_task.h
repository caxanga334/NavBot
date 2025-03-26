#ifndef __NAVBOT_TF2BOT_SPECIAL_DELIVERY_MONITOR_TASK_H_
#define __NAVBOT_TF2BOT_SPECIAL_DELIVERY_MONITOR_TASK_H_

class CTF2BotSDMonitorTask : public AITask<CTF2Bot>
{
public:
	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;

	const char* GetName() const override { return "SpecialDeliveryMonitor"; }
};

#endif // !__NAVBOT_TF2BOT_SPECIAL_DELIVERY_MONITOR_TASK_H_
