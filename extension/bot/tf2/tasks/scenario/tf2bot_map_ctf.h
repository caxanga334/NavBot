#ifndef NAVBOT_TF2BOT_MAP_CTF_TASK_H_
#define NAVBOT_TF2BOT_MAP_CTF_TASK_H_
#pragma once

class CTF2Bot;

class CTF2BotCTFMonitorTask : public AITask<CTF2Bot>
{
public:
	virtual TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;

	virtual const char* GetName() const override { return "CTFMonitor"; }
};

#endif // !NAVBOT_TF2BOT_MAP_CTF_TASK_H_
