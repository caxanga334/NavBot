#include <vector>
#include <limits>

#include <extension.h>
#include <util/helpers.h>
#include <bot/tf2/tf2bot.h>
#include <mods/tf2/tf2lib.h>
#include "tf2bot_medic_retreat_task.h"
#include "tf2bot_medic_main_task.h"

#undef max
#undef min
#undef clamp

TaskResult<CTF2Bot> CTF2BotMedicMainTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	m_scantimer.Start(1.0f);
	m_repathtimer.Start(0.25f);
	ScanForPatients(bot);

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotMedicMainTask::OnTaskUpdate(CTF2Bot* bot)
{
	if (m_scantimer.IsElapsed())
	{
		ScanForPatients(bot);
		m_scantimer.Start(1.0f);
	}

	auto patient = GetPatient();

	if (!patient)
	{
		bool allieds = false;

		bot->GetSensorInterface()->ForEveryKnownEntity([bot, &allieds](const CKnownEntity* known) {
			if (!allieds && !known->IsObsolete() && known->IsPlayer() && tf2lib::GetEntityTFTeam(known->GetIndex()) == bot->GetMyTFTeam())
			{
				allieds = true;
			}
		});

		if (!allieds) // no nearby allied player
		{
			return PauseFor(new CTF2BotMedicRetreatTask, "No patient to heal and no nearby teammates, retreating to safety!");
		}

		return Continue();
	}

	m_haspatienttimer.Start();
	bot->GetControlInterface()->AimAt(patient, IPlayerController::LOOK_COMBAT, 0.2f, "MEDIC: Looking at patient!");
	bot->SelectWeaponByCommand("tf_weapon_medigun");

	CBaseExtPlayer them(patient);
	const float range = bot->GetRangeTo(patient);
	Vector aimat = them.WorldSpaceCenter();

	// When near the patient, begin healing them
	if (range <= medigun_max_heal_range())
	{
		bot->GetControlInterface()->PressAttackButton(0.5f);
	}
	else
	{
		bot->GetControlInterface()->ReleaseAttackButton();
	}

	if (range > medic_range_to_stay_near_patient())
	{
		if (m_repathtimer.IsElapsed() || !m_nav.IsValid())
		{
			Vector goal = them.GetAbsOrigin();
			CTF2BotPathCost cost(bot);
			m_nav.ComputePathToPosition(bot, goal, cost);
		}

		m_nav.Update(bot); // move towards the patient
	}

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotMedicMainTask::OnTaskResume(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	m_scantimer.Start(0.1f);
	m_repathtimer.Start(0.25f);
	ScanForPatients(bot);
	m_nav.Invalidate();

	return Continue();
}

edict_t* CTF2BotMedicMainTask::GetPatient()
{
	if (!m_patient.IsValid())
	{
		return nullptr;
	}

	auto patient = gamehelpers->GetHandleEntity(m_patient);

	if (!patient)
	{
		m_patient.Term();
		return nullptr;
	}

	// patient has died
	if (!UtilHelpers::IsEntityAlive(m_patient.GetEntryIndex()))
	{
		m_patient.Term();
		return nullptr;
	}

	return patient;
}

bool CTF2BotMedicMainTask::ScanForPatients(CTF2Bot* me)
{
	auto patient = me->MedicFindBestPatient();

	if (patient)
	{
		gamehelpers->SetHandleEntity(m_patient, patient);
		m_haspatienttimer.Start();
		m_repathtimer.Start(0.25f);
		return true;
	}

	return false;
}
