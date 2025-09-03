#ifndef NAVBOT_TF2BOT_FIND_HEALTH_TASK_H_
#define NAVBOT_TF2BOT_FIND_HEALTH_TASK_H_
#pragma once

#include <basehandle.h>
#include <sdkports/sdk_timers.h>
#include <bot/interfaces/path/meshnavigator.h>

class CTF2BotFindHealthTask : public AITask<CTF2Bot>
{
public:
	enum class HealthSource : uint8_t
	{
		NONE = 0, // none found
		HEALTHPACK, // item_health*
		RESUPPLY, // resupply cabinet
		DISPENSER // dispenser
	};

	CTF2BotFindHealthTask(CBaseEntity* source);

	static bool IsPossible(CTF2Bot* bot, CBaseEntity** source);

	TaskResult<CTF2Bot> OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;
	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;

	TaskEventResponseResult<CTF2Bot> OnMoveToSuccess(CTF2Bot* bot, CPath* path) override;

	const char* GetName() const override { return "FindHealth"; }

private:
	CountdownTimer m_failsafetimer;
	CMeshNavigatorAutoRepath m_nav;
	HealthSource m_type;
	CHandle<CBaseEntity> m_sourceentity;
	CountdownTimer m_healthUpdateTimer;
	bool m_dontMove;
	int m_heathlValueLastUpdate;

	bool IsSourceStillValid(CTF2Bot* me);
};

#endif // !NAVBOT_TF2BOT_FIND_HEALTH_TASK_H_