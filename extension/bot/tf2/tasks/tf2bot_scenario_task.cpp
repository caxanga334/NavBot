#include NAVBOT_PCH_FILE
#include <cstring>

#include <extension.h>
#include <util/helpers.h>
#include <util/entprops.h>
#include <util/librandom.h>
#include <mods/tf2/teamfortress2mod.h>
#include <mods/tf2/nav/tfnavmesh.h>
#include <entities/tf2/tf_entities.h>
#include <sdkports/sdk_takedamageinfo.h>
#include <mods/tf2/tf2lib.h>
#include <bot/tf2/tf2bot.h>
#include <bot/interfaces/path/chasenavigator.h>
#include <bot/tf2/tasks/general/tf2bot_remove_sapper_from_object_task.h>
#include <bot/tf2/tasks/scenario/tf2bot_map_ctf.h>
#include <bot/tf2/tasks/medic/tf2bot_medic_main_task.h>
#include <bot/tf2/tasks/engineer/tf2bot_engineer_main.h>
#include <bot/tf2/tasks/sniper/tf2bot_task_sniper_main.h>
#include <bot/tf2/tasks/spy/tf2bot_spy_main_task.h>
#include <bot/tf2/tasks/scenario/deathmatch/tf2bot_deathmatch.h>
#include "scenario/controlpoints/tf2bot_controlpoints_monitor.h"
#include "scenario/payload/tf2bot_task_defend_payload.h"
#include "scenario/payload/tf2bot_task_push_payload.h"
#include "scenario/mvm/tf2bot_mvm_monitor.h"
#include "scenario/specialdelivery/tf2bot_special_delivery_monitor_task.h"
#include "scenario/pd/tf2bot_pd_monitor_task.h"
#include "scenario/community/tf2bot_vsh_monitor_task.h"
#include "scenario/community/tf2bot_of_deathmatch_tasks.h"
#include "scenario/community/tf2bot_zombie_infection_tasks.h"
#include <bot/tasks_shared/bot_shared_escort_entity.h>
#include <bot/tasks_shared/bot_shared_go_to_position.h>
#include <bot/tasks_shared/bot_shared_rogue_behavior.h>
#include <bot/tasks_shared/bot_shared_squad_member_monitor.h>
#include <bot/tasks_shared/bot_shared_collect_items.h>
#include <bot/tf2/tasks/scenario/tf2bot_destroy_halloween_boss_task.h>
#include "scenario/passtime/tf2bot_passtime_monitor_task.h"
#include "scenario/rd/tf2bot_rd_monitor_task.h"
#include "tf2bot_scenario_task.h"

static bool HalloweenSpellPickupValidator(CTF2Bot* bot, CBaseEntity* spellPickup)
{
	const CBotWeapon* spellbook = bot->GetInventoryInterface()->FindWeaponByClassname("tf_weapon_spellbook");

	if (spellbook)
	{
		int charges = 0;
		entprops->GetEntProp(spellbook->GetEntity(), Prop_Send, "m_iSpellCharges", charges);

		if (charges != 0)
		{
			return false; // got a spell
		}
	}

	if (entityprops::IsEffectActiveOnEntity(spellPickup, EF_NODRAW))
	{
		return false;
	}

	return true;
}

AITask<CTF2Bot>* CTF2BotScenarioTask::InitialNextTask(CTF2Bot* bot)
{
	CTeamFortress2Mod* tf2mod = CTeamFortress2Mod::GetTF2Mod();

	// All classes except engineer and medics will hunt the boss.
	if (tf2mod->IsTruceActive() && bot->GetMyClassType() != TeamFortress2::TFClassType::TFClass_Medic && 
		bot->GetMyClassType() != TeamFortress2::TFClassType::TFClass_Engineer)
	{
		return new CTF2BotDestroyHalloweenBossTask;
	}

	return CTF2BotScenarioTask::SelectScenarioTask(bot);
}

TaskResult<CTF2Bot> CTF2BotScenarioTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	CTeamFortress2Mod* tf2mod = CTeamFortress2Mod::GetTF2Mod();
	m_respondToTeamMatesTimer.Invalidate();
	TeamFortress2::GameModeType gm = tf2mod->GetCurrentGameMode();

	// Don't allow going rogue on some game modes
	if (gm != TeamFortress2::GameModeType::GM_MVM && gm != TeamFortress2::GameModeType::GM_VSH && 
		gm != TeamFortress2::GameModeType::GM_ZI && gm != TeamFortress2::GameModeType::GM_SF && 
		gm != TeamFortress2::GameModeType::GM_VSFFA)
	{
		int roguechance = tf2mod->GetModSettings()->GetRogueBehaviorChance();

		if (roguechance > 0 && CBaseBot::s_botrng.GetRandomChance(roguechance))
		{
			return PauseFor(new CBotSharedRogueBehaviorTask<CTF2Bot, CTF2BotPathCost>(), "Starting rogue behavior!");
		}
	}

	bool spellsenabled = false;

	if (entprops->GameRules_GetPropBool("m_bIsUsingSpells", spellsenabled) && spellsenabled)
	{
		m_collectSpellTimer.Start(10.0f);
	}
	else
	{
		// start the timer with a very large time to basically disable it
		m_collectSpellTimer.Start(1e6f);
	}

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotScenarioTask::OnTaskUpdate(CTF2Bot* bot)
{
	if (GetNextTask() == nullptr)
	{
		return SwitchTo(new CTF2BotScenarioTask, "Restarting scenario monitor!");
	}

	if (m_collectSpellTimer.IsElapsed() && bot->GetBehaviorInterface()->ShouldHurry(bot) != ANSWER_YES)
	{
		float delay = 10.0f;

		const CBotWeapon* spellbook = bot->GetInventoryInterface()->FindWeaponByClassname("tf_weapon_spellbook");

		if (spellbook)
		{
			int charges = 0;
			entprops->GetEntProp(spellbook->GetEntity(), Prop_Send, "m_iSpellCharges", charges);

			if (charges == 0)
			{
				NBotSharedCollectItemTask::ItemCollectFilter<CTF2Bot, CTFNavArea> filter{ bot };
				CBaseEntity* spell = nullptr;

				if (CBotSharedCollectItemsTask<CTF2Bot, CTF2BotPathCost>::IsPossible(bot, &filter, "tf_spell_pickup", &spell))
				{
					auto task = new CBotSharedCollectItemsTask<CTF2Bot, CTF2BotPathCost>(bot, spell, NBotSharedCollectItemTask::COLLECT_WALK_OVER);
					auto func = std::bind(HalloweenSpellPickupValidator, std::placeholders::_1, std::placeholders::_2);
					task->SetValidationFunction(func);
					return PauseFor(task, "Collecting halloween spell!");
				}
			}
			else
			{
				// already has a spell, big delay
				delay = 30.0f;
			}
		}
		else
		{
			// spellbook not equipped, probably won't be, bigger delay
			delay = 300.0f;
		}

		m_collectSpellTimer.Start(delay);
	}

	return Continue();
}

AITask<CTF2Bot>* CTF2BotScenarioTask::SelectScenarioTask(CTF2Bot* me, const bool skipClassBehavior)
{
	auto tf2mod = CTeamFortress2Mod::GetTF2Mod();
	auto gm = tf2mod->GetCurrentGameMode();
	bool defend = tf2mod->GetModSettings()->RollDefendChance();

	// Current gamemode is deathmatch, no class specific behavior is used
	if (tf2mod->UseDeathmatchBehaviorOnly())
	{
		return new CTF2BotDeathmatchScenarioTask;
	}

	if (gm == TeamFortress2::GameModeType::GM_MVM)
	{
		return new CTF2BotMvMMonitorTask; // In MvM, all bots start with this
	}

	// Don't use class behavior on this game mode
	if (gm == TeamFortress2::GameModeType::GM_VSFFA)
	{
		return new CTF2BotOFDMMonitorTask;
	}

	if (gm == TeamFortress2::GameModeType::GM_ZI)
	{
		return new CTF2BotZIMonitorTask;
	}

	if (!skipClassBehavior)
	{
		AITask<CTF2Bot>* task = CTF2BotScenarioTask::SelectClassTask(me);

		if (task != nullptr)
		{
			return task;
		}
	}

	switch (gm)
	{
	case TeamFortress2::GameModeType::GM_CTF:
		return new CTF2BotCTFMonitorTask;
	case TeamFortress2::GameModeType::GM_PL:
	{
		if (tf2mod->GetTeamRole(me->GetMyTFTeam()) == TeamFortress2::TEAM_ROLE_DEFENDERS)
		{
			return new CTF2BotDefendPayloadTask;
		}
		else
		{
			return new CTF2BotPushPayloadTask;
		}

		break;
	}
	case TeamFortress2::GameModeType::GM_PL_RACE:
	{
		if (defend)
		{
			return new CTF2BotDefendPayloadTask;
		}
		else
		{
			return new CTF2BotPushPayloadTask;
		}

		break;
	}
	case TeamFortress2::GameModeType::GM_CP:
		[[fallthrough]];
	case TeamFortress2::GameModeType::GM_ADCP:
		[[fallthrough]];
	case TeamFortress2::GameModeType::GM_ARENA: // arena may need it's own task
		[[fallthrough]];
	case TeamFortress2::GameModeType::GM_TC: // same for TC
		[[fallthrough]];
	case TeamFortress2::GameModeType::GM_KOTH:
		return new CTF2BotControlPointMonitorTask;
	case TeamFortress2::GameModeType::GM_SD:
		return new CTF2BotSDMonitorTask;
	case TeamFortress2::GameModeType::GM_PD:
		return new CTF2BotPDMonitorTask;
	case TeamFortress2::GameModeType::GM_PASSTIME:
		return new CTF2BotPassTimeMonitorTask;
	case TeamFortress2::GameModeType::GM_RD:
		return new CTF2BotRobotDestructionMonitorTask;
	case TeamFortress2::GameModeType::GM_VSH:
		return new CTF2BotVSHMonitorTask;
	default:
		break;
	}

	return new CTF2BotDeathmatchScenarioTask;
}

AITask<CTF2Bot>* CTF2BotScenarioTask::SelectClassTask(CTF2Bot* me)
{
	auto myclass = me->GetMyClassType();

	if (CTeamFortress2Mod::GetTF2Mod()->IsPlayingMedievalMode())
	{
		// Sniper and engineers uses default game mode behavior in medieval mode
		switch (myclass)
		{
		case TeamFortress2::TFClass_Medic:
			return new CTF2BotMedicMainTask;
		case TeamFortress2::TFClass_Spy:
			return new CTF2BotSpyMainTask;
		default:
			return nullptr;
		}
	}

	switch (myclass)
	{
	case TeamFortress2::TFClass_Sniper:
	{
		CBaseEntity* weapon = me->GetWeaponOfSlot(static_cast<int>(TeamFortress2::TFWeaponSlot::TFWeaponSlot_Primary));

		if (weapon)
		{
			const char* classname = gamehelpers->GetEntityClassname(weapon);

			if (std::strcmp(classname, "tf_weapon_compound_bow") == 0)
			{
				return nullptr; // snipers with the huntsman will run default behavior
			}
		}

		return new CTF2BotSniperMainTask;
	}
	case TeamFortress2::TFClass_Medic:
		return new CTF2BotMedicMainTask;
	case TeamFortress2::TFClass_Spy:
		return new CTF2BotSpyMainTask;
	case TeamFortress2::TFClass_Engineer:
		return new CTF2BotEngineerMainTask;
	default:
		break;
	}

	return nullptr;
}

TaskEventResponseResult<CTF2Bot> CTF2BotScenarioTask::OnVoiceCommand(CTF2Bot* bot, CBaseEntity* subject, int command)
{
	int skill = bot->GetDifficultyProfile()->GetGameAwareness();

	if (skill <= 15 || !m_respondToTeamMatesTimer.IsElapsed())
	{
		return TryContinue(PRIORITY_DONT_CARE);
	}

	if (CTeamFortress2Mod::GetTF2Mod()->GetCurrentGameMode() == TeamFortress2::GameModeType::GM_MVM && entprops->GameRules_GetRoundState() != RoundState_RoundRunning)
	{
		// don't follow teammates between waves in MvM
		return TryContinue(PRIORITY_DONT_CARE);
	}

	// these classes never follow teammates
	switch (bot->GetMyClassType())
	{
	case TeamFortress2::TFClassType::TFClass_Engineer:
		[[fallthrough]];
	case TeamFortress2::TFClassType::TFClass_Sniper:
		[[fallthrough]];
	case TeamFortress2::TFClassType::TFClass_Medic:
		[[fallthrough]];
	case TeamFortress2::TFClassType::TFClass_Spy:
	{
		return TryContinue(PRIORITY_DONT_CARE);
	}
	default:
		break;
	}

	constexpr float LISTEN_TO_TEAMMATE_MAX_RANGE = 1200.0f; // Limit range to listen to teammate voice commands
	TeamFortress2::TFTeam theirteam = static_cast<TeamFortress2::TFTeam>(entityprops::GetEntityTeamNum(subject));
	TeamFortress2::VoiceCommandsID vcmd = static_cast<TeamFortress2::VoiceCommandsID>(command);

	if (!IsPaused() && theirteam == bot->GetMyTFTeam() && bot->GetBehaviorInterface()->ShouldHurry(bot) != ANSWER_YES
		&& bot->GetBehaviorInterface()->ShouldAssistTeammate(bot, subject) != ANSWER_NO &&
		CBaseBot::s_botrng.GetRandomChance(bot->GetDifficultyProfile()->GetTeamwork()))
	{
		if (bot->GetRangeTo(UtilHelpers::getEntityOrigin(subject)) <= LISTEN_TO_TEAMMATE_MAX_RANGE)
		{
			if (vcmd == TeamFortress2::VoiceCommandsID::VC_HELP)
			{
				bot->SendVoiceCommand(TeamFortress2::VoiceCommandsID::VC_YES);
				m_respondToTeamMatesTimer.StartRandom(90.0f, 180.0f);
				return TryPauseFor(new CBotSharedEscortEntityTask<CTF2Bot, CTF2BotPathCost>(bot, subject, 30.0f), PRIORITY_MEDIUM, "Responding to teammate call for help!");
			}
			else if (vcmd == TeamFortress2::VoiceCommandsID::VC_GOLEFT || vcmd == TeamFortress2::VoiceCommandsID::VC_GORIGHT) /* flank left/right */
			{
				CBaseExtPlayer player(UtilHelpers::BaseEntityToEdict(subject));

				Vector forward, right;
				player.EyeVectors(&forward, &right, nullptr);
				const Vector& origin = UtilHelpers::getEntityOrigin(subject);

				Vector endpos = origin;
				endpos += forward * 750.0f;
				
				if (vcmd == TeamFortress2::VoiceCommandsID::VC_GOLEFT)
				{
					endpos -= right * 500.0f;
				}
				else
				{
					endpos += right * 500.0f;
				}

				// Check if a nav area is present
				CNavArea* area = TheNavMesh->GetNearestNavArea(endpos, CPath::PATH_GOAL_MAX_DISTANCE_TO_AREA * 3.0f, false, true);

				if (area)
				{
					if (bot->IsDebugging(BOTDEBUG_TASKS) && bot->IsDebugging(BOTDEBUG_MISC))
					{
						NDebugOverlay::Line(player.GetEyeOrigin(), endpos, 0, 180, 0, true, 30.0f);
						META_CONPRINTF("%s RESPONDED TO GOLEFT/RIGHT VOICE COMMAND! GOAL AREA #%i <%g, %g, %g> \n",
							bot->GetDebugIdentifier(), area->GetID(), endpos.x, endpos.y, endpos.z);
					}

					endpos = area->GetRandomPoint();
					m_respondToTeamMatesTimer.StartRandom(90.0f, 120.0f);
					bot->SendVoiceCommand(TeamFortress2::VoiceCommandsID::VC_YES);
					return TryPauseFor(new CBotSharedGoToPositionTask<CTF2Bot, CTF2BotPathCost>(bot, endpos, "MoveToCommand", true, false, true),
						EventResultPriorityType::PRIORITY_MEDIUM, "Respoding to teammate flank command!");
				}
			}
		}
	}

	return TryContinue(PRIORITY_DONT_CARE);
}

TaskEventResponseResult<CTF2Bot> CTF2BotScenarioTask::OnTruceChanged(CTF2Bot* bot, const bool enabled)
{
	if (enabled)
	{
		return TrySwitchTo(new CTF2BotScenarioTask, PRIORITY_HIGH, "Truce enabled, restarting scenario behavior!");
	}

	return TryContinue();
}

TaskEventResponseResult<CTF2Bot> CTF2BotScenarioTask::OnObjectSapped(CTF2Bot* bot, CBaseEntity* owner, CBaseEntity* saboteur)
{
	// Engineers already handle this in their behavior
	if (bot->GetMyClassType() == TeamFortress2::TFClassType::TFClass_Engineer)
	{
		return TryContinue();
	}

	if (bot->GetDifficultyProfile()->GetGameAwareness() < 10 || bot->GetDifficultyProfile()->GetTeamwork() < 20)
	{
		return TryContinue();
	}

	if (CTF2BotRemoveObjectSapperTask::IsPossible(bot))
	{
		CBaseEntity* object = CTF2BotRemoveObjectSapperTask::FindNearestSappedObject(bot);

		if (object)
		{
			m_respondToTeamMatesTimer.Start(30.0f); // ignore voice commands for some time
			return TryPauseFor(new CTF2BotRemoveObjectSapperTask(object), EventResultPriorityType::PRIORITY_MEDIUM, "Removing sapper from teammate's building.");
		}
	}
	else
	{
		CBaseEntity* object = CTF2BotRemoveObjectSapperTask::FindNearestSappedObject(bot);

		if (object)
		{
			const Vector& origin = UtilHelpers::getEntityOrigin(object);

			if ((origin - bot->GetAbsOrigin()).Length() <= bot->GetSensorInterface()->GetMaxHearingRange())
			{
				/* bot can't remove sappers (lacks weapon for it), just go to the sapped building position and help the engineer fight the spy */
				m_respondToTeamMatesTimer.Start(30.0f);
				return TryPauseFor(new CBotSharedGoToPositionTask<CTF2Bot, CTF2BotPathCost>(bot, origin, "InvestigatingSappedObject", true, true, true), EventResultPriorityType::PRIORITY_MEDIUM, "Investigating sapped teammate's building.");
			}
		}
	}

	return TryContinue();
}

TaskEventResponseResult<CTF2Bot> CTF2BotScenarioTask::OnSquadEvent(CTF2Bot* bot, SquadEventType evtype)
{
	if (evtype == SquadEventType::SQUAD_EVENT_JOINED)
	{
		AITask<CTF2Bot>* task = static_cast<AITask<CTF2Bot>*>(new CTF2BotScenarioTask);
		return TrySwitchTo(new CBotSharedSquadMemberMonitorTask<CTF2Bot, CTF2BotPathCost>(task), PRIORITY_CRITICAL, "I have joined a squad, starting squad member behavior!");
	}

	return TryContinue(PRIORITY_LOW);
}
