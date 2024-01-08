#ifndef NAVBOT_TF2BOT_TACTICAL_TASK_H_
#define NAVBOT_TF2BOT_TACTICAL_TASK_H_
#pragma once

class CTF2Bot;

class CTF2BotTacticalTask : public AITask<CTF2Bot>
{
public:
	virtual AITask<CTF2Bot>* InitialNextTask() override;
	virtual TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;

	virtual const char* GetName() const override { return "Tactical"; }
};

#endif // !NAVBOT_TF2BOT_TACTICAL_TASK_H_
