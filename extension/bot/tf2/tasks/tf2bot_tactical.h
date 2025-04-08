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

	/**
	 * @brief Selects a task for the current scenario (game mode)
	 * 
	 * Made static so other tasks can use this to select a game mode behavior
	 * @param me Bot
	 * @param skipClassBehavior if true, will not check for class specific behavior (medic, engineer, sniper, spy). Set to true to skip.
	 * @return Task to run or NULL if not supported.
	 */
	static AITask<CTF2Bot>* SelectScenarioTask(CTF2Bot* me, bool skipClassBehavior = false);

	// Selects a class specific behavior
	static AITask<CTF2Bot>* SelectClassTask(CTF2Bot* me);

	TaskEventResponseResult<CTF2Bot> OnInjured(CTF2Bot* bot, const CTakeDamageInfo& info) override;
	TaskEventResponseResult<CTF2Bot> OnVoiceCommand(CTF2Bot* bot, CBaseEntity* subject, int command) override;

private:
	CountdownTimer m_ammochecktimer;
	CountdownTimer m_healthchecktimer;
	CountdownTimer m_teleportertimer;
};


#endif // !NAVBOT_TF2BOT_TACTICAL_TASK_H_
