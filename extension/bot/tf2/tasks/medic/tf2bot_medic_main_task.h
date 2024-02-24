#ifndef NAVBOT_TF2BOT_TASK_MEDIC_MAIN_H_
#define NAVBOT_TF2BOT_TASK_MEDIC_MAIN_H_
#pragma once

#include <sdkports/sdk_timers.h>
#include <bot/interfaces/path/meshnavigator.h>
#include <basehandle.h>

class CTF2Bot;
struct edict_t;

class CTF2BotMedicMainTask : public AITask<CTF2Bot>
{
public:
	// virtual AITask<CTF2Bot>* InitialNextTask() override;
	virtual TaskResult<CTF2Bot> OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;
	virtual TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;
	virtual TaskResult<CTF2Bot> OnTaskResume(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;

	// medics never attack
	virtual QueryAnswerType ShouldAttack(CBaseBot* me, CKnownEntity* them) override { return ANSWER_NO; }

	virtual const char* GetName() const override { return "MedicMain"; }

private:
	CBaseHandle m_patient; // my current heal target
	CBaseHandle m_caller; // last player who yelled MEDIC!
	IntervalTimer m_calltimer; // timer since last MEDIC! call
	CountdownTimer m_scantimer; // timer for scanning for patients
	CountdownTimer m_repathtimer;
	IntervalTimer m_haspatienttimer;
	CMeshNavigator m_nav;

	edict_t* GetPatient();
	bool ScanForPatients(CTF2Bot* me);
	inline bool ShouldListenToCall() const
	{
		return m_calltimer.IsGreaterThen(2.0f);
	}

	static constexpr float patient_health_critical_level() { return 0.3f; }
	static constexpr float patient_health_low_level() { return 0.6f; }
	static constexpr float medigun_max_heal_range() { return 400.0f; }
	static constexpr float medic_range_to_stay_near_patient() { return 250.0f; }
};

#endif // !NAVBOT_TF2BOT_TASK_MEDIC_MAIN_H_
