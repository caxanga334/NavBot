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

	TaskResult<CTF2Bot> OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;
	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;

	const char* GetName() const override { return "FindHealth"; }

private:
	CountdownTimer m_failsafetimer;
	CMeshNavigatorAutoRepath m_nav;
	HealthSource m_type;
	bool m_reached;
	CBaseHandle m_sourceentity;
	Vector m_sourcepos;

	HealthSource FindSource(CTF2Bot* me);
	void UpdateSourcePosition();
	bool IsSourceStillValid(CTF2Bot* me);

	// max distance to search for health
	static constexpr float max_distance() { return 4096.0f; }
};

#endif // !NAVBOT_TF2BOT_FIND_HEALTH_TASK_H_