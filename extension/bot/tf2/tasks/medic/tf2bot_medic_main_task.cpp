#include <vector>
#include <limits>

#include <extension.h>
#include <util/helpers.h>
#include <bot/tf2/tf2bot.h>
#include <mods/tf2/tf2lib.h>
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
		// TO-DO: Retreat if no patient
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
	// filter for getting potential teammates to heal
	auto functor = [me](int client, edict_t* entity) -> bool {
		auto myteam = me->GetMyTFTeam();
		auto theirteam = tf2lib::GetEntityTFTeam(client);

		if (client == me->GetIndex())
		{
			return false; // can't heal self
		}

		if (myteam != theirteam) // can only heal teammates
		{
			return false;
		}

		if (tf2lib::IsPlayerInvisible(client))
		{
			return false; // can't heal invisible players
		}

		Vector mypos = me->WorldSpaceCenter();
		Vector theirpos = UtilHelpers::getWorldSpaceCenter(entity);
		float range = (theirpos - mypos).Length();
		constexpr auto max_scan_range = 1200.0f;

		if (range > max_scan_range)
		{
			return false;
		}

		// Don't do LOS checks, medic has an autocall feature that can be used to find teammates
		
		return true;
	};

	std::vector<int> players;
	players.reserve(100);
	UtilHelpers::CollectPlayers(players, functor);

	if (players.size() == 0)
	{
		return false; // no players to heal
	}

	edict_t* best = nullptr;
	float bestrange = std::numeric_limits<float>::max();
	Vector mypos = me->WorldSpaceCenter();

	for (auto& client : players)
	{
		auto them = gamehelpers->EdictOfIndex(client);
		Vector theirpos = UtilHelpers::getWorldSpaceCenter(them);
		float distance = (theirpos - mypos).Length();
		float health_percent = tf2lib::GetPlayerHealthPercentage(client);
		
		if (health_percent >= 1.0f)
		{
			distance *= 1.25f; // at max health or overhealed
		}

		if (health_percent <= patient_health_critical_level())
		{
			distance *= 0.5f;
		}

		if (health_percent <= patient_health_low_level())
		{
			distance *= 0.75f;
		}

		if (tf2lib::IsPlayerInCondition(client, TeamFortress2::TFCond_OnFire) ||
			tf2lib::IsPlayerInCondition(client, TeamFortress2::TFCond_Bleeding))
		{
			distance *= 0.8f;
		}

		if (distance < bestrange)
		{
			bestrange = distance;
			best = them;
		}
	}

	if (best)
	{
		gamehelpers->SetHandleEntity(m_patient, best);
		m_haspatienttimer.Start();
		m_repathtimer.Start(0.25f);

		me->DebugPrintToConsole(BOTDEBUG_TASKS, 0, 102, 0, "%s MEDIC: Found patient %s, range factor: %3.4f \n", me->GetDebugIdentifier(),
			UtilHelpers::GetPlayerDebugIdentifier(best), bestrange);

		return true;
	}

	return false;
}
