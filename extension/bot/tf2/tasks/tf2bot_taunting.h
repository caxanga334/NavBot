#ifndef NAVBOT_TF2BOT_TAUNTING_TASK_H_
#define NAVBOT_TF2BOT_TAUNTING_TASK_H_

#include <sdkports/sdk_timers.h>

class CTF2BotTauntingTask : public AITask<CTF2Bot>
{
public:
	TaskResult<CTF2Bot> OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;
	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;

	const char* GetName() const override { return "Taunting"; }

private:
	CountdownTimer m_endTauntTimer;
};


#endif // !NAVBOT_TF2BOT_TAUNTING_TASK_H_
