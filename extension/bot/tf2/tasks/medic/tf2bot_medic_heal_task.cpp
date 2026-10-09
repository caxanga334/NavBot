#include NAVBOT_PCH_FILE
#include <bot/tf2/tf2bot.h>
#include <bot/tf2/tf2bot_utils.h>
#include <mods/tf2/teamfortress2mod.h>
#include <mods/tf2/tf2lib.h>
#include <entities/tf2/tf_entities.h>
#include <bot/interfaces/path/meshnavigator.h>
#include <bot/tasks_shared/bot_shared_default_combat_tasks.h>
#include <bot/tasks_shared/bot_shared_take_cover_from_spot.h>
#include "tf2bot_medic_retreat_task.h"
#include "tf2bot_medic_revive_task.h"
#include "tf2bot_medic_crossbow_heal_task.h"
#include "tf2bot_medic_heal_task.h"

#ifdef EXT_VPROF_ENABLED
#include <tier0/vprof.h>
#endif // EXT_VPROF_ENABLED

CTF2BotMedicHealTask::CTF2BotMedicHealTask() :
	m_moveGoal(0.0f, 0.0f, 0.0f), m_patientCenter(0.0f, 0.0f, 0.0f), m_clearLOH(false)
{
	m_isMvM = CTeamFortress2Mod::GetTF2Mod()->GetCurrentGameMode() == TeamFortress2::GameModeType::GM_MVM;
}

TaskResult<CTF2Bot> CTF2BotMedicHealTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>*pastTask)
{
	UpdateHealTarget(bot);
	m_secondaryChecks.Start(2.0f);
	return Continue();
}

TaskResult<CTF2Bot> CTF2BotMedicHealTask::OnTaskUpdate(CTF2Bot* bot)
{
	if (m_secondaryChecks.IsElapsed())
	{
		m_secondaryChecks.Start(1.0f);

		const CTF2BotWeapon* weapon = bot->GetInventoryInterface()->GetTheCrusadersCrossbow();

		CBaseEntity* crossbowHeal = CTF2BotMedicCrossbowHealTask::IsPossible(bot, weapon);

		if (crossbowHeal != nullptr)
		{
			return PauseFor(new CTF2BotMedicCrossbowHealTask(crossbowHeal), "Healing teammate with the crossbow!");
		}
	}

	if (m_isMvM && m_reviveScanTimer.IsElapsed())
	{
		m_reviveScanTimer.Start(2.0f);
		CBaseEntity* marker = nullptr;

		if (CTF2BotMedicReviveTask::IsPossible(bot, &marker))
		{
			return PauseFor(new CTF2BotMedicReviveTask(marker), "Reviving dead teammate!");
		}
	}

	EquipMedigun(bot);

	const CTF2BotWeapon* activeWeapon = bot->GetInventoryInterface()->GetActiveTFWeapon();

	if (activeWeapon == nullptr || !activeWeapon->IsMedigun())
	{
		// wait until medigun is equipped
		return Continue();
	}

	UpdateHealTarget(bot);
	CBaseEntity* patient = GetHealTarget();
	const CKnownEntity* threat = bot->GetSensorInterface()->GetPrimaryKnownThreat();

	if (patient == nullptr)
	{
		if (threat != nullptr && threat->IsVisibleNow())
		{
			return PauseFor(new CBotSharedRetreatFromThreatTask<CTF2Bot, CTF2BotPathCost>(bot, true), "Retreating from visible enemy!");
		}

		return Continue();
	}

	m_patientCenter = UtilHelpers::getWorldSpaceCenter(patient);
	m_clearLOH = IsLineOfHealClear(bot, patient);
	HandleMedigun(bot, activeWeapon, patient, threat);
	HandleMovement(bot, activeWeapon, patient);

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotMedicHealTask::OnTaskResume(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	SetHealTarget(nullptr); // force
	UpdateHealTarget(bot);
	m_secondaryChecks.Start(2.0f);

	return Continue();
}

QueryAnswerType CTF2BotMedicHealTask::ShouldSwitchToWeapon(CBaseBot* me, const CBotWeapon* weapon)
{
	if (weapon->ClassnameMatchesPattern("tf_weapon_medigun"))
	{
		return ANSWER_YES;
	}

	return ANSWER_NO;
}

QueryAnswerType CTF2BotMedicHealTask::ShouldRetreat(CBaseBot* me)
{
	if (me->GetHealthPercentage() <= 0.6f)
	{
		return ANSWER_YES;
	}

	return ANSWER_NO;
}

TaskEventResponseResult<CTF2Bot> CTF2BotMedicHealTask::OnSquadEvent(CTF2Bot* bot, SquadEventType evtype)
{
	if (evtype == IEventListener::SquadEventType::SQUAD_EVENT_JOINED)
	{
		CTF2BotSquad* squad = bot->GetSquadInterface();

		if (squad->IsSquadValid())
		{
			const ISquad::Member* leader = squad->GetSquadData()->GetSquadLeader();

			if (leader != nullptr)
			{
				SetHealTarget(leader->GetPlayerEntity());
				m_changePatientTimer.Start(15.0f);
			}
		}
	}

	return TryToMaintain(PRIORITY_HIGH);
}

TaskEventResponseResult<CTF2Bot> CTF2BotMedicHealTask::OnVoiceCommand(CTF2Bot* bot, CBaseEntity* subject, int command)
{
	if (!m_respondToVoiceTimer.IsElapsed())
	{
		return TryToMaintain(PRIORITY_HIGH);
	}

	if (subject != nullptr && subject != bot->GetEntity())
	{
		if (bot->GetSensorInterface()->IsFriendly(subject))
		{
			m_respondToVoiceTimer.StartRandom(5.0f, 10.0f);

			TeamFortress2::VoiceCommandsID vcmd = static_cast<TeamFortress2::VoiceCommandsID>(command);

			switch (vcmd)
			{
			case TeamFortress2::VoiceCommandsID::VC_HELP:
				[[fallthrough]];
			case TeamFortress2::VoiceCommandsID::VC_MEDIC:
			{
				SetHealTarget(subject);
				m_changePatientTimer.StartRandom(4.0f, 6.0f);
				bot->SendVoiceCommand(TeamFortress2::VoiceCommandsID::VC_YES);
				break;
			}
			case TeamFortress2::VoiceCommandsID::VC_GOGOGO:
				[[fallthrough]];
			case TeamFortress2::VoiceCommandsID::VC_DEPLOYUBER:
			{
				if (GetHealTarget() != subject)
				{
					m_respondToVoiceTimer.Start(1.0f);
					bot->SendVoiceCommand(TeamFortress2::VoiceCommandsID::VC_NO);
					return TryToMaintain(PRIORITY_HIGH);
				}

				const CTF2BotWeapon* medigun = bot->GetInventoryInterface()->GetMedigun();

				if (medigun != nullptr && medigun->Medigun_CanDeployUber())
				{
					bot->SendVoiceCommand(TeamFortress2::VoiceCommandsID::VC_YES);
					bot->GetControlInterface()->PressSecondaryAttackButton(0.5f);
				}

				break;
			}
			default:
				break;
			}
		}
	}


	return TryToMaintain(PRIORITY_HIGH);
}

bool CTF2BotMedicHealTask::IsPatientValid(CTF2Bot* me) const
{
	CBaseEntity* patient = GetHealTarget();

	if (patient == nullptr)
	{
		return false;
	}

	if (modhelpers->IsDead(patient))
	{
		return false;
	}

	if (!me->GetSensorInterface()->IsFriendly(patient))
	{
		return false;
	}

	return true;
}

bool CTF2BotMedicHealTask::ShouldChangePatient(CTF2Bot* me)
{
	if (IsPatientValid(me))
	{
		// Don't switch too frequently
		if (!m_changePatientTimer.IsElapsed())
		{
			return false;
		}

		// NULL check done in IsPatientValid
		CBaseEntity* patient = GetHealTarget();

		// Don't switch off players while uber is active
		if (tf2lib::IsPlayerUnderUbercharge(patient))
		{
			m_changePatientTimer.Start(1.0f); // in this case, check again after 1 second
			return false;
		}

		int index = UtilHelpers::IndexOfEntity(patient);

		float hp = tf2lib::GetPlayerHealthPercentage(index);

		// heal until close to full health
		if (hp <= 0.95f)
		{
			return false;
		}

		// keep healing if these conditions are active
		if (tf2lib::IsPlayerInCondition(patient, TeamFortress2::TFCond::TFCond_Bleeding) ||
			tf2lib::IsPlayerInCondition(patient, TeamFortress2::TFCond::TFCond_OnFire) ||
			tf2lib::IsPlayerInCondition(patient, TeamFortress2::TFCond::TFCond_Jarated))
		{
			return false;
		}

		return true;
	}

	// Purge the heal target, might be a dead player
	SetHealTarget(nullptr);
	// invalid patient
	return true;
}

void CTF2BotMedicHealTask::EquipMedigun(CTF2Bot* me) const
{
	CTF2BotInventory* inv = me->GetInventoryInterface();
	const CTF2BotWeapon* activeweapon = inv->GetActiveTFWeapon();
	const CTF2BotWeapon* medigun = inv->GetMedigun();

	if (medigun == nullptr)
	{
		return;
	}

	if (activeweapon != medigun)
	{
		inv->EquipWeapon(medigun);
	}
}

CBaseEntity* CTF2BotMedicHealTask::SelectPatient(CTF2Bot* me) const
{
	std::priority_queue<PatientScore> queue;

	auto func = [&queue, &me](CBaseExtPlayer* player) {
		if (me->GetIndex() == player->GetIndex())
		{
			return;
		}

		CBaseEntity* entity = player->GetEntity();

		if (modhelpers->IsDead(entity))
		{
			return;
		}

		if (!me->GetSensorInterface()->IsFriendly(entity))
		{
			return;
		}

		float score = 5000.0f;

		constexpr float HP_SCORE_MULTIPLIER_WHEN_OVERHEALED = 0.25f;
		const float hp_percent = player->GetHealthPercentage();
		const float hp_mult = hp_percent >= 1.2f ? HP_SCORE_MULTIPLIER_WHEN_OVERHEALED : RemapValClamped(hp_percent, 1.0f, 0.05f, 1.0f, 40.0f);

		float distance = me->GetRangeTo(entity);
		// remap so higher distances get lower scores
		float dist_score = RemapVal(distance, 64.0f, 1024.0f, 4000.0f, 1.0f);

		if (distance <= 400.0f)
		{
			dist_score *= 1.5f; // additional score for those in the medigun range
		}

		score += dist_score;
		score *= hp_mult;

		if (tf2lib::IsPlayerInCondition(entity, TeamFortress2::TFCond::TFCond_Bleeding) ||
			tf2lib::IsPlayerInCondition(entity, TeamFortress2::TFCond::TFCond_OnFire) ||
			tf2lib::IsPlayerInCondition(entity, TeamFortress2::TFCond::TFCond_Jarated))
		{
			score *= 3.0f; // priorize these players
		}

		// randomize the score a bit
		// score *= CBaseBot::s_botrng.GetRandomReal<float>(0.8f, 1.2f);

		queue.emplace(entity, score);
	};

	extmanager->ForEachClient(func);

	if (queue.empty())
	{
		return nullptr;
	}

	auto& best = queue.top();

	if (me->IsDebugging(BOTDEBUG_TASKS))
	{
		me->DebugPrintToConsole(255, 0, 255, "%s MEDIC HEAL: SELECTED PATIENT: %s <SCORE: %g>\n", 
			me->GetDebugIdentifier(), UtilHelpers::textformat::FormatPlayer(UtilHelpers::IndexOfEntity(best.m_patient)), best.m_score);
	}

	if (best.m_score < 0.0f)
	{
		return nullptr;
	}

	return best.m_patient;
}

void CTF2BotMedicHealTask::HandleMedigun(CTF2Bot* me, const CTF2BotWeapon* medigun, CBaseEntity* patient, const CKnownEntity* threat)
{
	CTF2BotPlayerController* input = me->GetControlInterface();
	float distance = me->GetEyeDistanceTo(m_patientCenter);

	if (distance >= medigun->GetTF2Info()->GetPrimaryAttackInfo().GetMaxRange() || !m_clearLOH)
	{
		input->ReleaseAttackButton();
		return;
	}

	input->AimAt(m_patientCenter, IPlayerController::LOOK_SUPPORT, 1.0f, "Looking at patient to heal!");

	if (medigun->Medigun_IsHealing())
	{
		if (medigun->Medigun_GetHealTarget() == patient)
		{
			// Autoheal is enabled, holding the attack button isn't needed
			input->ReleaseAttackButton();

			if (ShouldDeployUbercharge(me, medigun, patient, threat))
			{
				DeployUbercharge(me);
			}

			return;
		}
		
		if (input->IsPressingAttackButton() && distance <= medigun->GetTF2Info()->GetPrimaryAttackInfo().GetMaxRange() * 1.3f)
		{
			input->ReleaseAttackButton();
		}
	}

	if (input->IsAimOnTarget())
	{
		input->PressAttackButton();
	}
}

bool CTF2BotMedicHealTask::ShouldDeployUbercharge(CTF2Bot* me, const CTF2BotWeapon* medigun, CBaseEntity* patient, const CKnownEntity* threat) const
{
	if (!medigun->Medigun_CanDeployUber() || medigun->Medigun_IsUberActive())
	{
		return false;
	}

	if (threat == nullptr)
	{
		return false;
	}

	CTF2BotSensor* sensor = me->GetSensorInterface();

	if ((sensor->GetVisibleEnemiesCount() - (sensor->GetVisibleAlliesCount() - 1)) >= 3)
	{
		return true;
	}

	float hp = tf2lib::GetPlayerHealthPercentage(UtilHelpers::IndexOfEntity(patient));
	return hp <= 0.6f;
}

void CTF2BotMedicHealTask::DeployUbercharge(CTF2Bot* me) const
{
	me->GetControlInterface()->PressSecondaryAttackButton(0.5f);
}

bool CTF2BotMedicHealTask::IsLineOfHealClear(CTF2Bot* me, CBaseEntity* patient) const
{
	auto trace_func_heal = [&me, &patient](IHandleEntity* pHandleEntity, int contentsMask) {
		CBaseEntity* hit = trace::EntityFromEntityHandle(pHandleEntity);

		if (hit == me->GetEntity() || hit == patient)
		{
			// don't hit the medic or the patient, hit everything else including other teammates
			return false;
		}

		return true;
	};

	trace::CTraceFilterSimple filter(nullptr, COLLISION_GROUP_NONE, trace_func_heal);
	trace_t tr;
	trace::line(me->GetEyeOrigin(), m_patientCenter, MASK_SOLID, &filter, tr);

	if (!tr.DidHit())
	{
		return true;
	}

	if (tr.m_pEnt == patient)
	{
		return true;
	}

	return false;
}

void CTF2BotMedicHealTask::HandleMovement(CTF2Bot* me, const CTF2BotWeapon* medigun, CBaseEntity* patient)
{
	const float distanceToPatient = me->GetEyeDistanceTo(m_patientCenter);
	const float medigunMaxRange = medigun->GetTF2Info()->GetPrimaryAttackInfo().GetMaxRange();
	const float halfMaxRange = medigunMaxRange / 2.0f;
	m_moveGoal = UtilHelpers::getEntityOrigin(patient);

	if (!m_clearLOH || distanceToPatient >= medigunMaxRange)
	{
		CTF2BotPathCost cost(me, RouteType::FASTEST_ROUTE);
		m_nav.Update(me, m_moveGoal, cost);
	}
}

void CTF2BotMedicHealTask::OnPatientChanged(CBaseEntity* newPatient)
{
	m_changePatientTimer.Start(2.0f);
}
