#ifndef __NAVBOT_TF2BOT_MEDIC_HEAL_TASK_H_
#define __NAVBOT_TF2BOT_MEDIC_HEAL_TASK_H_

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
	QueryAnswerType ShouldRetreat(CBaseBot* me) override;

	TaskEventResponseResult<CTF2Bot> OnSquadEvent(CTF2Bot* bot, SquadEventType evtype) override;
	TaskEventResponseResult<CTF2Bot> OnVoiceCommand(CTF2Bot* bot, CBaseEntity* subject, int command) override;

	const char* GetName() const override { return "MedicHeal"; }
private:
	CMeshNavigatorAutoRepath m_nav;
	CHandle<CBaseEntity> m_pocketTarget; // Player who the medic will pocket (main follow target).
	CHandle<CBaseEntity> m_healTarget; // Player who the medic is currently trying to heal.
	CountdownTimer m_changePatientTimer; // timer for expensive medic logic
	CountdownTimer m_secondaryChecks;
	CountdownTimer m_respondToVoiceTimer;
	CountdownTimer m_reviveScanTimer;
	Vector m_moveGoal;
	Vector m_patientCenter;
	bool m_clearLOH;
	bool m_isMvM;
	
	class PatientScore
	{
	public:
		PatientScore(CBaseEntity* patient, float score) :
			m_patient(patient), m_score(score)
		{
		}

		// used by std::less in the priority_queue
		bool operator<(const PatientScore& other) const
		{
			return this->m_score < other.m_score;
		}

		CBaseEntity* m_patient;
		float m_score;
	};

	bool IsPatientValid(CTF2Bot* me) const;
	bool ShouldChangePatient(CTF2Bot* me);
	void EquipMedigun(CTF2Bot* me) const;
	void UpdateHealTarget(CTF2Bot* me)
	{
		if (ShouldChangePatient(me))
		{
			CBaseEntity* patient = SelectPatient(me);

			if (patient != nullptr)
			{
				OnPatientChanged(patient);
				SetHealTarget(patient);
			}
		}
	}

	CBaseEntity* GetHealTarget() const { return m_healTarget.Get(); }
	void SetHealTarget(CBaseEntity* entity) { m_healTarget = entity; }
	CBaseEntity* SelectPatient(CTF2Bot* me) const;
	void HandleMedigun(CTF2Bot* me, const CTF2BotWeapon* medigun, CBaseEntity* patient, const CKnownEntity* threat);
	bool ShouldDeployUbercharge(CTF2Bot* me, const CTF2BotWeapon* medigun, CBaseEntity* patient, const CKnownEntity* threat) const;
	void DeployUbercharge(CTF2Bot* me) const;
	bool IsLineOfHealClear(CTF2Bot* me, CBaseEntity* patient) const;
	void HandleMovement(CTF2Bot* me, const CTF2BotWeapon* medigun, CBaseEntity* patient);
	void OnPatientChanged(CBaseEntity* newPatient);
};

#endif // !__NAVBOT_TF2BOT_MEDIC_HEAL_TASK_H_
