#ifndef __NAVBOT_TF2BOT_MEDIC_MVM_BUILD_UBER_TASK_H_
#define __NAVBOT_TF2BOT_MEDIC_MVM_BUILD_UBER_TASK_H_

class CTF2BotMedicMvMBuildUberTask : public AITask<CTF2Bot>
{
public:

	TaskResult<CTF2Bot> OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;
	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;

	QueryAnswerType IsReady(CBaseBot* me) override { return ANSWER_NO; }
	QueryAnswerType ShouldSwitchToWeapon(CBaseBot* me, const CBotWeapon* weapon) override { return ANSWER_NO; }

	const char* GetName() const override { return "MedicMain"; }

private:
	float* m_flChargeLevel;
	bool* m_bHealing;
	CHandle<CBaseEntity>* m_hHealingTarget; // medigun's current heal target
	CHandle<CBaseEntity> m_healTarget; // bot heal target
	CMeshNavigatorAutoRepath m_nav;

	bool FindPlayerToHeal(CTF2Bot* me);
};


#endif // !__NAVBOT_TF2BOT_MEDIC_MVM_BUILD_UBER_TASK_H_
