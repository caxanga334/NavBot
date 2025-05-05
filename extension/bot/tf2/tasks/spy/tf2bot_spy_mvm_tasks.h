#ifndef __NAVBOT_TF2BOT_SPY_MVM_TASKS_H_
#define __NAVBOT_TF2BOT_SPY_MVM_TASKS_H_

class CTF2BotSpySapMvMRobotTask : public AITask<CTF2Bot>
{
public:
	static bool IsPossible(CTF2Bot* bot);

	CTF2BotSpySapMvMRobotTask(CBaseEntity* target);

	TaskResult<CTF2Bot> OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;
	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;

	QueryAnswerType ShouldAttack(CBaseBot* me, const CKnownEntity* them) override { return ANSWER_NO; }
	QueryAnswerType ShouldSwitchToWeapon(CBaseBot* me, const CBotWeapon* weapon) override { return ANSWER_NO; }

	const char* GetName() const override { return "SpySapMvMRobot"; }

private:
	CHandle<CBaseEntity> m_target;
	CChaseNavigator m_nav;
	float* m_flEffectBarRegenTime;
};

#endif // !__NAVBOT_TF2BOT_SPY_MVM_TASKS_H_
