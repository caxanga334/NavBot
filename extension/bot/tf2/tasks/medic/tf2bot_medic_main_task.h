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
	TaskResult<CTF2Bot> OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;
	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;
	TaskResult<CTF2Bot> OnTaskResume(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;

	// medics never attack
	QueryAnswerType ShouldAttack(CBaseBot* me, const CKnownEntity* them) override { return ANSWER_NO; }

	const char* GetName() const override { return "MedicMain"; }

private:
	CBaseHandle m_patient; // my current heal target
	CBaseHandle m_caller; // last player who yelled MEDIC!
	IntervalTimer m_calltimer; // timer since last MEDIC! call
	CountdownTimer m_scantimer; // timer for scanning for patients
	CountdownTimer m_repathtimer;
	IntervalTimer m_haspatienttimer;
	CMeshNavigator m_nav;

	edict_t* GetPatient();
	void ScanForPatients(CTF2Bot* me);
	void EquipMedigun(CTF2Bot* me) const;
	inline bool ShouldListenToCall() const
	{
		return m_calltimer.IsGreaterThen(2.0f);
	}

	float GetUbercharge(CTF2Bot* me);

	static constexpr float MEDIC_MAX_DISTANCE = 300.0f;
};

#endif // !NAVBOT_TF2BOT_TASK_MEDIC_MAIN_H_
