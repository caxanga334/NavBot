#ifndef NAVBOT_TF2BOT_TASK_MEDIC_REVIVE_H_
#define NAVBOT_TF2BOT_TASK_MEDIC_REVIVE_H_
#pragma once

#include <sdkports/sdk_timers.h>
#include <bot/interfaces/path/meshnavigator.h>
#include <sdkports/sdk_ehandle.h>

class CTF2Bot;
struct edict_t;

class CTF2BotMedicReviveTask : public AITask<CTF2Bot>
{
public:
	CTF2BotMedicReviveTask(CBaseEntity* marker);

	TaskResult<CTF2Bot> OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;
	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;

	// medics never attack
	QueryAnswerType ShouldAttack(CBaseBot* me, const CKnownEntity* them) override { return ANSWER_NO; }

	const char* GetName() const override { return "MedicRevive"; }

private:
	CHandle<CBaseEntity> m_marker; // revive marker
	CountdownTimer m_repathTimer;
	CMeshNavigator m_nav;
	Vector m_aimpos;
	Vector m_goal;

	static constexpr auto marker_aimpos_z_offset() { return 32.0f; }
	static constexpr auto marker_heal_range() { return 300.0f; }
};

#endif // !NAVBOT_TF2BOT_TASK_MEDIC_REVIVE_H_
