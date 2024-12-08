#ifndef NAVBOT_TF2BOT_ATTACK_TASK_H_
#define NAVBOT_TF2BOT_ATTACK_TASK_H_

#include <bot/interfaces/path/chasenavigator.h>

class CTF2Bot;

class CTF2BotAttackTask : public AITask<CTF2Bot>
{
public:
	CTF2BotAttackTask(CBaseEntity* entity, const float escapeTime = 5.0f, const float maxChaseTime = 60.0f);

	TaskResult<CTF2Bot> OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;
	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;
	TaskResult<CTF2Bot> OnTaskResume(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;

	// Always allow weapon switches and attacking
	QueryAnswerType ShouldAttack(CBaseBot* me, const CKnownEntity* them) override { return ANSWER_YES; }
	QueryAnswerType ShouldSwitchToWeapon(CBaseBot* me, const CBotWeapon* weapon) override { return ANSWER_YES; }

	const char* GetName() const override { return "Attack"; }
private:
	CHandle<CBaseEntity> m_target;
	CChaseNavigator m_nav;
	CountdownTimer m_escapeTimer; // timer to give up if the bot doesn't have LOS
	CountdownTimer m_expireTimer; // timer to give up if this ends up taking too much time
	float m_escapeDuration;
	float m_chaseDuration;
};

#endif // !NAVBOT_TF2BOT_ATTACK_TASK_H_
