#ifndef NAVBOT_TF2BOT_FIND_AMMO_TASK_H_
#define NAVBOT_TF2BOT_FIND_AMMO_TASK_H_
#pragma once

#include <sdkports/sdk_ehandle.h>
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

	static bool IsPossible(CTF2Bot* bot, CBaseEntity** source);

	CTF2BotFindAmmoTask(CBaseEntity* entity);
	CTF2BotFindAmmoTask(CBaseEntity* entity, int maxmetal);

	TaskResult<CTF2Bot> OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;
	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;

	TaskEventResponseResult<CTF2Bot> OnMoveToFailure(CTF2Bot* bot, CPath* path, IEventListener::MovementFailureType reason) override;
	TaskEventResponseResult<CTF2Bot> OnMoveToSuccess(CTF2Bot* bot, CPath* path) override;

	const char* GetName() const override { return "FindAmmo"; }

private:
	CountdownTimer m_failsafetimer;
	CMeshNavigator m_nav;
	AmmoSource m_type;
	CHandle<CBaseEntity> m_sourceentity;
	int m_metalLimit;
	bool m_isDroppedAmmo;

	bool IsSourceStillValid(CTF2Bot* me);
};

#endif // !NAVBOT_TF2BOT_FIND_AMMO_TASK_H_
