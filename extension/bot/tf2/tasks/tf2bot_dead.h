#ifndef NAVBOT_TF2BOT_DEAD_TASK_H_
#define NAVBOT_TF2BOT_DEAD_TASK_H_

class CTF2Bot;

class CTF2BotDeadTask : public AITask<CTF2Bot>
{
public:
	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;

	const char* GetName() const override { return "Dead"; }
};

#endif // !NAVBOT_TF2BOT_DEAD_TASK_H_
