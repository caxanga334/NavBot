#ifndef __NAVBOT_TF2BOT_MEDIC_CROSSBOW_HEAL_TASK_H_
#define __NAVBOT_TF2BOT_MEDIC_CROSSBOW_HEAL_TASK_H_

class CTF2BotMedicCrossbowHealTask : public AITask<CTF2Bot>
{
public:
	CTF2BotMedicCrossbowHealTask(CBaseEntity* healTarget) :
		m_healTarget(healTarget)
	{

	}

	// returns the entity of the player the bot should heal with the crossbow
	static CBaseEntity* IsPossible(CTF2Bot* me, const CBotWeapon* crossbow);

	TaskResult<CTF2Bot> OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;
	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;

	QueryAnswerType ShouldAttack(CBaseBot* me, const CKnownEntity* them) override { return ANSWER_NO; }
	QueryAnswerType ShouldSwitchToWeapon(CBaseBot* me, const CBotWeapon* weapon) override { return ANSWER_NO; }

	const char* GetName() const override { return "MedicCrossbowHeal"; }
private:
	CHandle<CBaseEntity> m_healTarget;
	CountdownTimer m_aimTimer;
	CountdownTimer m_giveupTimer;
};


#endif // !__NAVBOT_TF2BOT_MEDIC_CROSSBOW_HEAL_TASK_H_
