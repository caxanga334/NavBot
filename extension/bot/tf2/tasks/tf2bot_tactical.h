#ifndef NAVBOT_TF2BOT_TACTICAL_TASK_H_
#define NAVBOT_TF2BOT_TACTICAL_TASK_H_
#pragma once

#include <basehandle.h>
#include <sdkports/sdk_timers.h>

class CTF2Bot;

class CTF2BotTacticalTask : public AITask<CTF2Bot>
{
public:
	AITask<CTF2Bot>* InitialNextTask(CTF2Bot* bot) override;
	TaskResult<CTF2Bot> OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;
	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;
	TaskResult<CTF2Bot> OnTaskResume(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;

	QueryAnswerType ShouldRetreat(CBaseBot* me) override;

	const char* GetName() const override { return "Tactical"; }

private:
	CountdownTimer m_ammochecktimer;
	CountdownTimer m_healthchecktimer;

	AITask<CTF2Bot>* SelectScenarioTask(CTF2Bot* me);
};


#endif // !NAVBOT_TF2BOT_TACTICAL_TASK_H_
