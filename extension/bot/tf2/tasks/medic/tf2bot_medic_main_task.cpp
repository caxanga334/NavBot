#include <vector>
#include <limits>

#include <extension.h>
#include <util/helpers.h>
#include <bot/tf2/tf2bot.h>
#include <mods/tf2/teamfortress2mod.h>
#include <mods/tf2/tf2lib.h>
#include <entities/tf2/tf_entities.h>
#include "tf2bot_medic_retreat_task.h"
#include "tf2bot_medic_revive_task.h"
#include "tf2bot_medic_main_task.h"

#undef max
#undef min
#undef clamp

static ConVar c_navbot_tf_medic_scan_time("sm_navbot_tf_medic_scan_time", "1.0", FCVAR_GAMEDLL, "Medics will scan for new heal targets every this seconds.");
static ConVar c_navbot_tf_medic_uber_enemies_count("sm_navbot_tf_medic_uber_enemies_count", "3", FCVAR_GAMEDLL, "Medics will deploy uber if this many enemies are visible on screen.");

TaskResult<CTF2Bot> CTF2BotMedicMainTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	m_scantimer.Start(c_navbot_tf_medic_scan_time.GetFloat());
	m_calltimer.Reset();
	m_haspatienttimer.Reset();
	LookForPatients(bot);
	return Continue();
}

TaskResult<CTF2Bot> CTF2BotMedicMainTask::OnTaskUpdate(CTF2Bot* bot)
{
	if (m_scantimer.IsElapsed())
	{
		LookForPatients(bot);
	}

	if (CTeamFortress2Mod::GetTF2Mod()->GetCurrentGameMode() == TeamFortress2::GameModeType::GM_MVM)
	{
		if (!tf2lib::IsPlayerInvulnerable(bot->GetIndex()) || !IsCurrentPatientValid())
		{
			CBaseEntity* marker = FindReviveMarker(bot);

			if (marker != nullptr)
			{
				return PauseFor(new CTF2BotMedicReviveTask(marker), "Reviving dead teammate.");
			}
		}
	}

	if (IsCurrentPatientValid())
	{
		m_haspatienttimer.Start();
		EquipMedigun(bot);
		PathToPatient(bot);

		if (bot->GetRangeTo(m_patient.ToEdict()) <= medigun_max_range() && bot->GetSensorInterface()->IsLineOfSightClear(m_patient.Get()))
		{
			bot->GetControlInterface()->AimAt(m_patient.ToEdict(), IPlayerController::LOOK_COMBAT, 0.5f, "Looking at my patient to heal them.");

			if (bot->IsLookingTowards(m_patient.Get(), heal_dot_tolerance()))
			{
				bot->GetControlInterface()->PressAttackButton(0.2f);

				if (GetUbercharge(bot) >= 0.99f)
				{
					if (bot->GetHealthPercentage() < 0.6f || tf2lib::GetPlayerHealthPercentage(m_patient.GetEntryIndex()) < 0.6f)
					{
						bot->GetControlInterface()->PressSecondaryAttackButton(0.1f);
					}
					else
					{
						// deploy based on visible enemies
						DeployUberIfNeeded(bot);
					}
				}
			}
		}
	}
	else
	{
		if (!LookForPatients(bot) && m_haspatienttimer.IsGreaterThen(3.0f))
		{
			return PauseFor(new CTF2BotMedicRetreatTask, "No patient, retreating to find one.");
		}
	}

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotMedicMainTask::OnTaskResume(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	return Continue();
}

bool CTF2BotMedicMainTask::IsCurrentPatientValid()
{
	if (m_patient.Get() == nullptr)
	{
		return false;
	}

	if (!UtilHelpers::IsPlayerAlive(m_patient.GetEntryIndex()))
	{
		m_patient.Term();
		return false;
	}

	return true;
}

bool CTF2BotMedicMainTask::LookForPatients(CTF2Bot* me)
{
	float bestDist = std::numeric_limits<float>::max();
	CBaseEntity* best = nullptr;

	UtilHelpers::ForEachPlayer([&me, &best, &bestDist](int client, edict_t* entity, SourceMod::IGamePlayer* player) {
		if (!player->IsInGame())
		{
			return;
		}

		if (client == me->GetIndex())
		{
			return;
		}

		auto myteam = me->GetMyTFTeam();

		if (myteam != tf2lib::GetEntityTFTeam(client))
		{
			if (tf2lib::GetPlayerClassType(client) == TeamFortress2::TFClass_Spy)
			{
				auto& spy = me->GetSpyMonitorInterface()->GetKnownSpy(entity);

				if (spy.GetDetectionLevel() != CTF2BotSpyMonitor::DETECTION_FOOLED)
				{
					return; // Don't heal hostile spies
				}
			}
			else
			{
				return; // Don't heal enemies that are not spies
			}
		}

		if (!me->GetSensorInterface()->IsLineOfSightClear(entity))
		{
			return;
		}

		float distance = me->GetRangeTo(entity);

		if (tf2lib::GetPlayerHealthPercentage(client) >= 0.9f)
		{
			distance *= 2.0f;
		}

		if (distance < bestDist)
		{
			best = gamehelpers->ReferenceToEntity(client);
			bestDist = distance;
		}
	});

	if (best != nullptr)
	{
		m_patient.Set(best);
		m_scantimer.Start(c_navbot_tf_medic_scan_time.GetFloat());
		m_haspatienttimer.Start();
		return true;
	}

	return false;
}

void CTF2BotMedicMainTask::PathToPatient(CTF2Bot* me)
{
	entities::HBaseEntity patient(m_patient.Get());
	Vector goal = patient.GetAbsOrigin();

	if (me->GetRangeTo(goal) <= MEDIC_MAX_DISTANCE && me->GetSensorInterface()->IsLineOfSightClear(m_patient.Get()))
	{
		return;
	}

	if (!m_nav.IsValid() || m_repathtimer.IsElapsed())
	{
		CTF2BotPathCost cost(me);

		m_repathtimer.Start(0.5f);
		if (!m_nav.ComputePathToPosition(me, goal, cost))
		{
			m_nav.Invalidate();
			return;
		}
	}

	m_nav.Update(me);
}

void CTF2BotMedicMainTask::DeployUberIfNeeded(CTF2Bot* me)
{
	bool sentry = false;
	int visibleEnemies = 0;
	auto enemyteam = tf2lib::GetEnemyTFTeam(me->GetMyTFTeam());

	me->GetSensorInterface()->ForEveryKnownEntity([&me, &sentry, &visibleEnemies, &enemyteam](const CKnownEntity* known) {
		if (known->IsObsolete())
		{
			return;
		}

		if (enemyteam == tf2lib::GetEntityTFTeam(known->GetIndex()))
		{
			if (known->IsVisibleNow())
			{
				visibleEnemies++;

				if (!sentry && UtilHelpers::FClassnameIs(known->GetEntity(), "obj_sentrygun"))
				{
					sentry = true;
				}
			}
		}

	});

	if (visibleEnemies >= c_navbot_tf_medic_uber_enemies_count.GetInt() || sentry)
	{
		me->GetControlInterface()->PressSecondaryAttackButton(0.1f);
	}
}

void CTF2BotMedicMainTask::EquipMedigun(CTF2Bot* me) const
{
	auto myweapon = me->GetInventoryInterface()->GetActiveBotWeapon();

	if (myweapon && myweapon->GetModWeaponID<TeamFortress2::TFWeaponID>() == TeamFortress2::TFWeaponID::TF_WEAPON_MEDIGUN)
	{
		return;
	}

	me->SelectWeaponByCommand("tf_weapon_medigun");
}

float CTF2BotMedicMainTask::GetUbercharge(CTF2Bot* me)
{
	auto medigun = me->GetInventoryInterface()->GetActiveBotWeapon();

	if (medigun && medigun->GetModWeaponID<TeamFortress2::TFWeaponID>() == TeamFortress2::TFWeaponID::TF_WEAPON_MEDIGUN)
	{
		return tf2lib::GetMedigunUberchargePercentage(medigun->GetIndex());
	}

	return 0.0f;
}

CBaseEntity* CTF2BotMedicMainTask::FindReviveMarker(CTF2Bot* me)
{
	CBaseEntity* marker = nullptr;
	float t = std::numeric_limits<float>::max();
	Vector origin = me->GetAbsOrigin();

	UtilHelpers::ForEachEntityOfClassname("entity_revive_marker", [&marker, &t, &origin](int index, edict_t* edict, CBaseEntity* entity) -> bool {
		tfentities::HTFBaseEntity be(entity);

		float d = (be.GetAbsOrigin() - origin).Length();

		if (d < t && d <= 2048.0f)
		{
			marker = entity;
			t = d;
		}

		return true;
	});

	return marker;
}
