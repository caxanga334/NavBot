#ifndef NAVBOT_TF2BOT_TASKS_ENGINEER_MAIN_H_
#define NAVBOT_TF2BOT_TASKS_ENGINEER_MAIN_H_

class CTF2Bot;

class CTF2BotEngineerMainTask : public AITask<CTF2Bot>
{
public:

	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;

	const char* GetName() const override { return "EngineerMain"; }
};

#endif // !NAVBOT_TF2BOT_TASKS_ENGINEER_MAIN_H_
