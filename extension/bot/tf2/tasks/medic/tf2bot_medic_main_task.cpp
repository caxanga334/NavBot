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
	auto patient = GetPatient();
	EquipMedigun(bot);

	if (m_scantimer.IsElapsed() || patient == nullptr)
	{
		ScanForPatients(bot);
		m_scantimer.Start(1.0f);
	}

	bool allieds = false;
	bool enemySentryGun = false;
	int visibleEnemyCount = 0;

	bot->GetSensorInterface()->ForEveryKnownEntity([&bot, &allieds, &visibleEnemyCount, &enemySentryGun](const CKnownEntity* known) {
		if (!known->IsObsolete() && known->IsPlayer())
		{
			if (!allieds && tf2lib::GetEntityTFTeam(known->GetIndex()) == bot->GetMyTFTeam())
			{
				allieds = true;
			}

			if (known->IsVisibleNow() && tf2lib::GetEntityTFTeam(known->GetIndex()) == tf2lib::GetEnemyTFTeam(bot->GetMyTFTeam()))
			{
				++visibleEnemyCount;
				
				if (!enemySentryGun)
				{
					auto classname = gamehelpers->GetEntityClassname(known->GetEntity());

					if (strncasecmp(classname, "obj_sentrygun", 13) == 0)
					{
						enemySentryGun = true;
					}
				}
			}
		}
	});

	if (!patient)
	{
		if (!allieds) // no nearby allied player
		{
			return PauseFor(new CTF2BotMedicRetreatTask, "No patient to heal and no nearby teammates, retreating to safety!");
		}

		return Continue();
	}

	m_haspatienttimer.Start();
	bot->GetControlInterface()->AimAt(patient, IPlayerController::LOOK_COMBAT, 0.2f, "MEDIC: Looking at patient!");

	const float range = bot->GetRangeTo(patient);
	Vector aimat = UtilHelpers::getWorldSpaceCenter(patient);
	auto medigun = bot->GetActiveBotWeapon();
	const float max_heal_range = medigun != nullptr ? medigun->GetWeaponInfo().GetAttackInfo(WeaponInfo::PRIMARY_ATTACK).GetMaxRange() : 400.0f;

	// When near the patient, begin healing them
	if (range <= max_heal_range)
	{
		bot->GetControlInterface()->PressAttackButton(0.5f);
	}
	else
	{
		bot->GetControlInterface()->ReleaseAttackButton();
	}

	// Auto deploy uber if at half health or an enemy sentry gun is visible or more than 3 enemies are visible right now
	if (GetUbercharge(bot) >= 0.99f && (bot->GetHealthPercentage() < 0.5f || enemySentryGun || visibleEnemyCount > 3 || 
		tf2lib::GetPlayerHealthPercentage(m_patient.GetEntryIndex()) < 0.5f ))
	{
		bot->GetControlInterface()->PressSecondaryAttackButton(0.3f);
		m_scantimer.Start(10.0f); // Don't switch patients
	}

	if (range > MEDIC_MAX_DISTANCE)
	{
		if (m_repathtimer.IsElapsed() || !m_nav.IsValid())
		{
			const Vector& goal = patient->GetCollideable()->GetCollisionOrigin();
			CTF2BotPathCost cost(bot);
			m_nav.ComputePathToPosition(bot, goal, cost);
			m_repathtimer.Start(0.25f);
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

void CTF2BotMedicMainTask::ScanForPatients(CTF2Bot* me)
{
	auto best = me->MedicFindBestPatient();

	if (best)
	{
		UtilHelpers::SetHandleEntity(m_patient, best);
	}
}

void CTF2BotMedicMainTask::EquipMedigun(CTF2Bot* me) const
{
	auto myweapon = me->GetActiveBotWeapon();

	if (myweapon && myweapon->GetModWeaponID<TeamFortress2::TFWeaponID>() == TeamFortress2::TFWeaponID::TF_WEAPON_MEDIGUN)
	{
		return;
	}

	me->SelectWeaponByCommand("tf_weapon_medigun");
}


float CTF2BotMedicMainTask::GetUbercharge(CTF2Bot* me)
{
	auto medigun = me->GetActiveBotWeapon();

	if (medigun)
	{
		return tf2lib::GetMedigunUberchargePercentage(medigun->GetIndex());
	}

	return 0.0f;
}
