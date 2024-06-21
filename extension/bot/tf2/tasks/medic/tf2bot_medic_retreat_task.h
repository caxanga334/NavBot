#ifndef NAVBOT_TF2BOT_MEDIC_RETREAT_TASK_H_
#define NAVBOT_TF2BOT_MEDIC_RETREAT_TASK_H_
#pragma once

#include <bot/interfaces/path/meshnavigator.h>

class CTF2Bot;

class CTF2BotMedicRetreatTask : public AITask<CTF2Bot>
{
public:
	TaskResult<CTF2Bot> OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;
	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;

	QueryAnswerType ShouldAttack(CBaseBot* me, const CKnownEntity* them) override { return ANSWER_YES; }

	const char* GetName() const override { return "MedicRetreat"; }

private:
	Vector m_goal;
	CMeshNavigator m_nav;
	CountdownTimer m_repathtimer;

	static constexpr float home_range() { return 256.0f; }

	Vector GetRetreatPosition(CTF2Bot* me) const;
};


#endif // !NAVBOT_TF2BOT_MEDIC_RETREAT_TASK_H_
