#ifndef NAVBOT_TF2BOT_TASK_MEDIC_MAIN_H_
#define NAVBOT_TF2BOT_TASK_MEDIC_MAIN_H_
#pragma once

#include <sdkports/sdk_timers.h>
#include <bot/interfaces/path/meshnavigator.h>
#include <sdkports/sdk_ehandle.h>

class CTF2Bot;
struct edict_t;

class CTF2BotMedicMainTask : public AITask<CTF2Bot>
{
public:
	// virtual AITask<CTF2Bot>* InitialNextTask() override;
	TaskResult<CTF2Bot> OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;
	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;
	TaskResult<CTF2Bot> OnTaskResume(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;

	TaskEventResponseResult<CTF2Bot> OnFlagTaken(CTF2Bot* bot, CBaseEntity* player) override;

	// medics never attack
	QueryAnswerType ShouldAttack(CBaseBot* me, const CKnownEntity* them) override { return ANSWER_NO; }

	const char* GetName() const override { return "MedicMain"; }

private:
	CHandle<CBaseEntity> m_patient; // my current heal target
	CHandle<CBaseEntity> m_caller; // last player who yelled MEDIC!
	IntervalTimer m_calltimer; // timer since last MEDIC! call
	CountdownTimer m_scantimer; // timer for scanning for patients
	CountdownTimer m_repathtimer;
	IntervalTimer m_haspatienttimer;
	CMeshNavigator m_nav;

	bool IsCurrentPatientValid();
	bool LookForPatients(CTF2Bot* me);
	void PathToPatient(CTF2Bot* me);
	void DeployUberIfNeeded(CTF2Bot* me);

	void EquipMedigun(CTF2Bot* me) const;
	// true if the medic should listen to a MEDIC yell
	inline bool ShouldListenToCall() const
	{
		return m_calltimer.IsGreaterThen(2.0f);
	}

	float GetUbercharge(CTF2Bot* me);

	CBaseEntity* FindReviveMarker(CTF2Bot* me);

	static constexpr float MEDIC_MAX_DISTANCE = 200.0f;
	static constexpr auto medigun_max_range() { return 420.0f; }
	static constexpr auto heal_dot_tolerance() { return 0.95f; }
};

#endif // !NAVBOT_TF2BOT_TASK_MEDIC_MAIN_H_
