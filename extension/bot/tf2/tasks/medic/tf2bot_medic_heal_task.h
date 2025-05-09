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

	// don't attack enemies if I healing my team
	QueryAnswerType ShouldAttack(CBaseBot* me, const CKnownEntity* them) override { return ANSWER_NO; }
	QueryAnswerType ShouldSwitchToWeapon(CBaseBot* me, const CBotWeapon* weapon) override;

	TaskEventResponseResult<CTF2Bot> OnVoiceCommand(CTF2Bot* bot, CBaseEntity* subject, int command) override;

	const char* GetName() const override { return "MedicHeal"; }
private:
	CMeshNavigatorAutoRepath m_nav;
	CHandle<CBaseEntity> m_followTarget; // Player I want to follow
	CHandle<CBaseEntity> m_healTarget; // Player I am healing
	CountdownTimer m_patientScanTimer; // Time to scan for people to heal
	CountdownTimer m_respondToCallsTimer;
	CountdownTimer m_reviveMarkerScanTimer;
	Vector m_moveGoal;
	bool m_isMvM;

	void UpdateFollowTarget(CTF2Bot* bot);
	void UpdateHealTarget(CTF2Bot* bot);
	void UpdateMovePosition(CTF2Bot* bot, const CKnownEntity* threat);
	bool ScanForReviveMarkers(const Vector& center, CBaseEntity** marker);
	bool IsPatientStable(CTF2Bot* bot, CBaseEntity* patient);
	void EquipMedigun(CTF2Bot* me);
	float GetUbercharge(CTF2Bot* me);

	static constexpr float MEDIGUN_LETGO_RANGE = 400.0f;
	static constexpr float MEDIC_RESPOND_TO_CALL_RANGE = 600.0f;
};

#endif // !NAVBOT_TF2BOT_MEDIC_HEAL_TASK_H_
