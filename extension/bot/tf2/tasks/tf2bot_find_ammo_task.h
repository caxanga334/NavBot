#ifndef NAVBOT_TF2BOT_FIND_AMMO_TASK_H_
#define NAVBOT_TF2BOT_FIND_AMMO_TASK_H_
#pragma once

#include <basehandle.h>
#include <sdkports/sdk_timers.h>
#include <bot/interfaces/path/meshnavigator.h>

class CTF2BotFindAmmoTask : public AITask<CTF2Bot>
{
public:
	enum class AmmoSource : uint8_t
	{
		NONE = 0, // none found
		AMMOPACK, // item_ammo*
		RESUPPLY, // resupply cabinet
		DISPENSER // dispenser
	};

	CTF2BotFindAmmoTask();

	TaskResult<CTF2Bot> OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;
	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;

	TaskEventResponseResult<CTF2Bot> OnMoveToFailure(CTF2Bot* bot, CPath* path, IEventListener::MovementFailureType reason) override;
	TaskEventResponseResult<CTF2Bot> OnMoveToSuccess(CTF2Bot* bot, CPath* path) override;

	const char* GetName() const override { return "FindAmmo"; }

private:
	CountdownTimer m_repathtimer;
	CountdownTimer m_failsafetimer;
	CMeshNavigator m_nav;
	AmmoSource m_type;
	bool m_reached;
	CBaseHandle m_sourceentity;
	Vector m_sourcepos;

	AmmoSource FindSource(CTF2Bot* me);
	void UpdateSourcePosition();
	bool IsSourceStillValid(CTF2Bot* me);

	// max distance to search for ammo
	static constexpr float max_distance() { return 2048.0f; }
};

#endif // !NAVBOT_TF2BOT_FIND_AMMO_TASK_H_
