#ifndef NAVBOT_TF2BOT_TASK_MEDIC_MAIN_H_
#define NAVBOT_TF2BOT_TASK_MEDIC_MAIN_H_
#pragma once

#include <sdkports/sdk_timers.h>
#include <bot/interfaces/path/meshnavigator.h>
#include <sdkports/sdk_ehandle.h>

class CTF2Bot;
struct edict_t;

class CTF2BotMedicMainTask : public AITask<CTF2Bot>
{
public:
	AITask<CTF2Bot>* InitialNextTask(CTF2Bot* bot) override;
	TaskResult<CTF2Bot> OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;
	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;
	TaskResult<CTF2Bot> OnTaskResume(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;

	TaskEventResponseResult<CTF2Bot> OnFlagTaken(CTF2Bot* bot, CBaseEntity* player) override;

	const char* GetName() const override { return "MedicMain"; }

private:

};

#endif // !NAVBOT_TF2BOT_TASK_MEDIC_MAIN_H_
