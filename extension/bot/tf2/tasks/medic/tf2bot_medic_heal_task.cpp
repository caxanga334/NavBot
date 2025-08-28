#include NAVBOT_PCH_FILE
#include <vector>
#include <limits>

#include <extension.h>
#include <util/helpers.h>
#include <util/entprops.h>
#include <sdkports/sdk_traces.h>
#include <bot/tf2/tf2bot.h>
#include <bot/tf2/tf2bot_utils.h>
#include <mods/tf2/teamfortress2mod.h>
#include <mods/tf2/tf2lib.h>
#include <entities/tf2/tf_entities.h>
#include <bot/tasks_shared/bot_shared_attack_enemy.h>
#include "tf2bot_medic_retreat_task.h"
#include "tf2bot_medic_revive_task.h"
#include "tf2bot_medic_crossbow_heal_task.h"
#include "tf2bot_medic_heal_task.h"

#undef max
#undef min
#undef clamp

static bool IsLineOfHealClearToPatient(CTF2Bot* me, CBaseEntity* patient)
{
	trace::CTraceFilterSimple filter{ me->GetEntity(), COLLISION_GROUP_NONE };
	trace_t tr;
	trace::line(me->GetEyeOrigin(), UtilHelpers::getWorldSpaceCenter(patient), MASK_SOLID, &filter, tr);

	if (!tr.DidHit() || tr.m_pEnt == patient)
	{
		return true;
	}

	return false;
}

CTF2BotMedicHealTask::CTF2BotMedicHealTask() :
	m_moveGoal(0.0f, 0.0f, 0.0f)
{
	m_isMvM = false;
}

TaskResult<CTF2Bot> CTF2BotMedicHealTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	UpdateFollowTarget(bot);
	m_patientScanTimer.Invalidate();

	m_isMvM = CTeamFortress2Mod::GetTF2Mod()->GetCurrentGameMode() == TeamFortress2::GameModeType::GM_MVM;
	m_reviveMarkerScanTimer.Start(10.0f);
	m_crossbowHealTimer.Start(3.0f);

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotMedicHealTask::OnTaskUpdate(CTF2Bot* bot)
{
	UpdateFollowTarget(bot);
	UpdateHealTarget(bot);

	if (m_crossbowHealTimer.IsElapsed())
	{
		const CBotWeapon* crossbow = bot->GetInventoryInterface()->FindWeaponByClassname("tf_weapon_crossbow");

		m_crossbowHealTimer.Start(3.0f);

		if (crossbow && crossbow->GetBaseCombatWeapon().GetClip1() >= 1)
		{
			CBaseEntity* heal = CTF2BotMedicCrossbowHealTask::IsPossible(bot, crossbow);

			if (heal)
			{
				return PauseFor(new CTF2BotMedicCrossbowHealTask(heal), "Healing teammate with the crossbow!");
			}
		}
		else
		{
			if (!crossbow)
			{
				// doesn't own a crossbow
				m_crossbowHealTimer.Start(120.0f);
			}
		}
	}

	if (m_isMvM && m_reviveMarkerScanTimer.IsElapsed())
	{
		m_reviveMarkerScanTimer.StartRandom(2.0f, 5.0f);

		CBaseEntity* marker = nullptr;
		Vector center = bot->GetAbsOrigin();

		if (ScanForReviveMarkers(center, &marker))
		{
			return PauseFor(new CTF2BotMedicReviveTask(marker), "Reviving dead teammate!");
		}
	}

	CBaseEntity* patient = m_healTarget.Get();
	const CKnownEntity* threat = bot->GetSensorInterface()->GetPrimaryKnownThreat(true);

	if (patient == nullptr)
	{
		if (threat)
		{
			return PauseFor(new CBotSharedAttackEnemyTask<CTF2Bot, CTF2BotPathCost>(bot), "Nobody to heal. Attacking visible threat!");
		}

		return Continue();
	}

	// patient is dead and an enemy is visible, retreat!
	if (!UtilHelpers::IsEntityAlive(patient) && threat && threat->IsVisibleNow())
	{
		return PauseFor(new CTF2BotMedicRetreatTask(), "Patient died, retreating from enemy!");
	}

	UpdateMovePosition(bot, threat);
	EquipMedigun(bot);
	CTF2BotPlayerController* input = bot->GetControlInterface();

	input->SetDesiredAimSpot(IDecisionQuery::DesiredAimSpot::AIMSPOT_CENTER);
	input->AimAt(patient, IPlayerController::LOOK_SUPPORT, 0.2f, "Looking at patient to heal them!");

	if (!m_letGoTimer.IsElapsed())
	{
		input->ReleaseAllAttackButtons();
		return Continue();
	}

	CBaseEntity* medigun = bot->GetWeaponOfSlot(static_cast<int>(TeamFortress2::TFWeaponSlot::TFWeaponSlot_Secondary));

	if (medigun)
	{
		float patientRange = bot->GetRangeTo(UtilHelpers::getWorldSpaceCenter(patient));
		CBaseEntity* healtarget = nullptr;
		entprops->GetEntPropEnt(medigun, Prop_Send, "m_hHealingTarget", nullptr, &healtarget);

		if (patientRange < MEDIGUN_LETGO_RANGE)
		{
			// healing my patient
			if (healtarget == patient)
			{
				// auto heal is enabled, just release the attack button
				input->ReleaseAttackButton();
			}
			else if (input->IsPressingAttackButton())
			{
				// let go to allow switching heal targets
				input->ReleaseAttackButton();
			}
			else if (input->IsAimOnTarget() && IsLineOfHealClearToPatient(bot, patient))
			{
				input->PressAttackButton();
			}
		}
		else // patient is outside range
		{
			// release attack button, auto-heal is enabled and the medigun should keep healing
			input->ReleaseAttackButton();
		}
	}

	float uber = GetUbercharge(bot);
	float rage = 0.0f;

	if (m_isMvM)
	{
		entprops->GetEntPropFloat(bot->GetIndex(), Prop_Send, "m_flRageMeter", rage);
	}

	if (uber > 0.9999f || rage > 0.9999f)
	{
		int visiblethreats = 0;
		auto func = [&visiblethreats](const CKnownEntity* known) {
			if (known->IsVisibleNow())
			{
				visiblethreats++;
			}
		};

		bot->GetSensorInterface()->ForEveryKnownEnemy(func);

		int min_threats = 3; // always deploy if there are 3 enemies visible

		if (bot->GetHealthPercentage() < 0.7f || tf2lib::GetPlayerHealthPercentage(m_healTarget.GetEntryIndex()) < 0.7f)
		{
			min_threats = 1; // if me or my patient is getting low on health, deploy even with 1 enemy
		}

		if (visiblethreats >= min_threats)
		{
			if (uber > 0.9999f)
			{
				bot->GetControlInterface()->PressSecondaryAttackButton();
				m_crossbowHealTimer.Start(20.0f);
			}
			else
			{
				bot->GetControlInterface()->PressSpecialAttackButton();
			}
		}
	}

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotMedicHealTask::OnTaskResume(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	return Continue();
}

QueryAnswerType CTF2BotMedicHealTask::ShouldSwitchToWeapon(CBaseBot* me, const CBotWeapon* weapon)
{
	// Only allow the bot to switch to the medigun

	if (weapon->GetWeaponInfo()->GetSlot() == static_cast<int>(TeamFortress2::TFWeaponSlot::TFWeaponSlot_Secondary))
	{
		return ANSWER_YES;
	}

	return ANSWER_NO;
}

QueryAnswerType CTF2BotMedicHealTask::ShouldRetreat(CBaseBot* me)
{
	if (me->GetSensorInterface()->GetVisibleAlliesCount() == 0)
	{
		return ANSWER_YES; // medics with no visible allies should retreat
	}

	if (me->GetDifficultyProfile()->GetTeamwork() > 50)
	{
		if (me->GetHealthState() != CBaseBot::HealthState::HEALTH_CRITICAL)
		{
			return ANSWER_NO; // high team work medics only retreat if on critical health
		}
	}
	else
	{
		// Only retreat if low on health
		if (me->GetHealthState() == CBaseBot::HealthState::HEALTH_OK)
		{
			return ANSWER_NO;
		}
	}

	return ANSWER_UNDEFINED;
}

TaskEventResponseResult<CTF2Bot> CTF2BotMedicHealTask::OnVoiceCommand(CTF2Bot* bot, CBaseEntity* subject, int command)
{
	if (subject == nullptr || tf2lib::GetEntityTFTeam(subject) != bot->GetMyTFTeam())
	{
		return TryContinue();
	}

	TeamFortress2::VoiceCommandsID vcmd = static_cast<TeamFortress2::VoiceCommandsID>(command);

	if (bot->GetBehaviorInterface()->ShouldAssistTeammate(bot, subject) == ANSWER_NO)
	{
		return TryContinue();
	}

	if (vcmd == TeamFortress2::VoiceCommandsID::VC_DEPLOYUBER)
	{
		if (m_healTarget.Get() == subject && GetUbercharge(bot) > 0.9999f)
		{
			auto threat = bot->GetSensorInterface()->GetPrimaryKnownThreat(true);

			if (threat) // only accept the deploy uber command if I can see at least one enemy
			{
				bot->GetControlInterface()->PressSecondaryAttackButton();
			}
		}
	}
	else if (vcmd == TeamFortress2::VoiceCommandsID::VC_HELP || vcmd == TeamFortress2::VoiceCommandsID::VC_MEDIC)
	{
		if (m_healTarget.Get() != subject && bot->GetRangeTo(subject) <= MEDIC_RESPOND_TO_CALL_RANGE)
		{
			if (m_respondToCallsTimer.IsElapsed())
			{
				m_respondToCallsTimer.StartRandom(2.0f, 4.0f);
				m_patientScanTimer.Reset();
				m_healTarget = subject;
			}
		}
	}

	return TryContinue();
}

void CTF2BotMedicHealTask::UpdateFollowTarget(CTF2Bot* bot)
{
	CBaseEntity* follow = m_followTarget.Get();

	if (follow != nullptr)
	{
		if (UtilHelpers::IsEntityAlive(follow))
		{
			return; // Follow target until death
		}
	}

	CBaseEntity* nextFollowTarget = nullptr;
	float best = std::numeric_limits<float>::max();
	auto functor = [&bot, &best, &nextFollowTarget](int client, edict_t* entity, SourceMod::IGamePlayer* player) {

		if (client != bot->GetIndex() && player->IsInGame() && tf2lib::GetEntityTFTeam(client) == bot->GetMyTFTeam() && UtilHelpers::IsPlayerAlive(client))
		{
			auto theirclass = tf2lib::GetPlayerClassType(client);
			float distance = bot->GetRangeToSqr(entity);

			// never follow these classes
			switch (theirclass)
			{
			case TeamFortress2::TFClass_Scout:
				[[fallthrough]];
			case TeamFortress2::TFClass_Sniper:
				[[fallthrough]];
			case TeamFortress2::TFClass_Medic:
				[[fallthrough]];
			case TeamFortress2::TFClass_Spy:
				[[fallthrough]];
			case TeamFortress2::TFClass_Engineer:
				return;
			case TeamFortress2::TFClass_Soldier:
				[[fallthrough]];
			case TeamFortress2::TFClass_Heavy:
				distance *= 0.75f; // prefer following soldiers and heavies
				break;
			default:
				break;
			}

			if (distance < best)
			{
				best = distance;
				nextFollowTarget = entity->GetIServerEntity()->GetBaseEntity();
			}
		}
	};

	UtilHelpers::ForEachPlayer(functor);

	if (nextFollowTarget != nullptr)
	{
		m_followTarget = nextFollowTarget;
	}
}

void CTF2BotMedicHealTask::UpdateHealTarget(CTF2Bot* bot)
{
	CBaseEntity* heal = m_healTarget.Get();

	if (heal && UtilHelpers::IsEntityAlive(heal))
	{
		if (!IsPatientStable(bot, heal))
		{
			// not stable, keep healing
			return;
		}
	}
	else if (heal == nullptr)
	{
		m_healTarget = m_followTarget.Get();
		return;
	}

	if (!m_patientScanTimer.IsElapsed())
	{
		return;
	}

	CBaseEntity* nextHealTarget = tf2botutils::MedicSelectBestPatientToHeal(bot, CTeamFortress2Mod::GetTF2Mod()->GetTF2ModSettings()->GetMedicPatientScanRange());

	if (nextHealTarget == nullptr)
	{
		m_healTarget = m_followTarget.Get();
	}
	else
	{
		m_healTarget = nextHealTarget;
	}

	m_patientScanTimer.Start(2.0f);
	m_letGoTimer.Start(0.5f); // let go of the attack button to switch patients
}

void CTF2BotMedicHealTask::UpdateMovePosition(CTF2Bot* bot, const CKnownEntity* threat)
{
	CBaseEntity* pPatient = m_healTarget.Get();
	tfentities::HTFBaseEntity patient(pPatient);
	const CTF2BotWeapon* myweapon = bot->GetInventoryInterface()->GetActiveTFWeapon();
	float moveRange = 250.0f; // default medigun range is around 450
	
	if (myweapon)
	{
		moveRange = (myweapon->GetTF2Info()->GetAttackInfo(WeaponInfo::PRIMARY_ATTACK).GetMaxRange() * 0.6f);
	}
	else
	{
		bot->GetInventoryInterface()->RequestRefresh();
	}
	
	float rangeToGoal = moveRange * moveRange;

	Vector start = bot->GetAbsOrigin();
	Vector end = patient.GetAbsOrigin();

	trace::CTraceFilterSimple filter(bot->GetEntity(), COLLISION_GROUP_NONE);
	trace_t tr;
	trace::line(bot->GetEyeOrigin(), patient.WorldSpaceCenter(), MASK_SHOT, &filter, tr);
	bool obstruction = false;

	if (tr.fraction < 1.0f && tr.m_pEnt != pPatient)
	{
		obstruction = true;
		rangeToGoal = 64.0f * 64.0f;
	}

	// no enemies or obstruction, move to the patient
	if (threat == nullptr || obstruction)
	{
		m_moveGoal = end;
	}
	else // visible enemies
	{
		// direction from the patient to the threat
		Vector dir = (end - threat->GetLastKnownPosition());
		dir.NormalizeInPlace();
		m_moveGoal = end + (dir * moveRange); // position myself behind my patient in relation to the threat
		CNavArea* area = TheNavMesh->GetNearestNavArea(m_moveGoal, moveRange * 2.0f);
		
		if (area != nullptr)
		{
			area->GetClosestPointOnArea(m_moveGoal, &m_moveGoal);
		}

		rangeToGoal = 30.0f * 30.0f;
	}

	float distance = bot->GetRangeToSqr(m_moveGoal);

	if (!bot->GetSensorInterface()->IsAbleToSee(patient.WorldSpaceCenter()) || distance > rangeToGoal)
	{
		CTF2BotPathCost cost(bot);
		m_nav.Update(bot, m_moveGoal, cost);
	}
}

bool CTF2BotMedicHealTask::ScanForReviveMarkers(const Vector& center, CBaseEntity** marker)
{
	float best = std::numeric_limits<float>::max();
	auto functor = [&marker, &center, &best](int index, edict_t* edict, CBaseEntity* entity) {
		if (entity)
		{
			const Vector& origin = UtilHelpers::getEntityOrigin(entity);
			float range = (origin - center).LengthSqr();

			if (range < best)
			{
				*marker = entity;
				best = range;
			}
		}

		return true;
	};

	UtilHelpers::ForEachEntityOfClassname("entity_revive_marker", functor);

	return *marker != nullptr;
}

bool CTF2BotMedicHealTask::IsPatientStable(CTF2Bot* bot, CBaseEntity* patient)
{
	if (tf2lib::IsPlayerInCondition(bot->GetEntity(), TeamFortress2::TFCond_Ubercharged) || tf2lib::IsPlayerInCondition(bot->GetEntity(), TeamFortress2::TFCond_Kritzkrieged) ||
		tf2lib::IsPlayerInCondition(bot->GetEntity(), TeamFortress2::TFCond_MegaHeal))
	{
		// uber active, don't change patients
		return false;
	}

	if (tf2lib::IsPlayerInvisible(patient))
	{
		return true;
	}

	// Visible enemies, keep healing
	if (bot->GetSensorInterface()->GetVisibleEnemiesCount() > 0)
	{
		return false;
	}

	int entindex = UtilHelpers::IndexOfEntity(patient);

	if (tf2lib::GetPlayerHealthPercentage(entindex) < 0.99f)
	{
		if (tf2lib::GetPlayerHealthPercentage(entindex) < 0.6f)
		{
			m_crossbowHealTimer.Start(2.0f); // don't use the crossbow if my current heal target is low on health
		}

		return false;
	}

	if (tf2lib::IsPlayerInCondition(patient, TeamFortress2::TFCond_OnFire) || tf2lib::IsPlayerInCondition(patient, TeamFortress2::TFCond_Bleeding))
	{
		return false;
	}

	return true;
}


void CTF2BotMedicHealTask::EquipMedigun(CTF2Bot* me)
{
	auto myweapon = me->GetInventoryInterface()->GetActiveBotWeapon();

	if (myweapon && myweapon->GetWeaponInfo()->GetSlot() == static_cast<int>(TeamFortress2::TFWeaponSlot::TFWeaponSlot_Secondary))
	{
		return;
	}

	CBaseEntity* medigun = me->GetWeaponOfSlot(static_cast<int>(TeamFortress2::TFWeaponSlot::TFWeaponSlot_Secondary));

	if (medigun != nullptr)
	{
		me->SelectWeapon(medigun);
	}
}

float CTF2BotMedicHealTask::GetUbercharge(CTF2Bot* me)
{
	auto medigun = me->GetInventoryInterface()->GetActiveBotWeapon();

	if (medigun && medigun->GetWeaponInfo()->GetSlot() == static_cast<int>(TeamFortress2::TFWeaponSlot::TFWeaponSlot_Secondary))
	{
		return tf2lib::GetMedigunUberchargePercentage(medigun->GetIndex());
	}

	return 0.0f;
}
