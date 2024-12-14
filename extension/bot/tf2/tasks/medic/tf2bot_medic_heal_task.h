#ifndef NAVBOT_TF2BOT_MEDIC_HEAL_TASK_H_
#define NAVBOT_TF2BOT_MEDIC_HEAL_TASK_H_

#include <sdkports/sdk_timers.h>
#include <bot/interfaces/path/meshnavigator.h>
#include <sdkports/sdk_ehandle.h>

class CTF2BotMedicHealTask : public AITask<CTF2Bot>
{
public:
	CTF2BotMedicHealTask();

	TaskResult<CTF2Bot> OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;
	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;
	TaskResult<CTF2Bot> OnTaskResume(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;

	QueryAnswerType ShouldSwitchToWeapon(CBaseBot* me, const CBotWeapon* weapon) override;

	const char* GetName() const override { return "MedicHeal"; }
private:
	CMeshNavigatorAutoRepath m_nav;
	CHandle<CBaseEntity> m_followTarget; // Player I want to follow
	CHandle<CBaseEntity> m_healTarget; // Player I am healing
	CountdownTimer m_patientScanTimer; // Time to scan for people to heal
	Vector m_moveGoal;

	void UpdateFollowTarget(CTF2Bot* bot);
	void UpdateHealTarget(CTF2Bot* bot);
	void UpdateMovePosition(CTF2Bot* bot, const CKnownEntity* threat);

	void EquipMedigun(CTF2Bot* me);
	float GetUbercharge(CTF2Bot* me);
};

#endif // !NAVBOT_TF2BOT_MEDIC_HEAL_TASK_H_
