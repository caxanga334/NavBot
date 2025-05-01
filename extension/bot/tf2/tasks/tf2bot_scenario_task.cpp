#include <cstring>

#include <extension.h>
#include <util/helpers.h>
#include <util/entprops.h>
#include <util/librandom.h>
#include <mods/tf2/teamfortress2mod.h>
#include <entities/tf2/tf_entities.h>
#include <sdkports/sdk_takedamageinfo.h>
#include <mods/tf2/tf2lib.h>
#include <bot/tf2/tf2bot.h>
#include <bot/bot_shared_convars.h>
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
#include <bot/tasks_shared/bot_shared_escort_entity.h>
#include <bot/tf2/tasks/scenario/tf2bot_destroy_halloween_boss_task.h>
#include "tf2bot_scenario_task.h"

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
	m_respondToTeamMatesTimer.Invalidate();

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotScenarioTask::OnTaskUpdate(CTF2Bot* bot)
{
	if (GetNextTask() == nullptr)
	{
		return SwitchTo(new CTF2BotScenarioTask, "Restarting scenario monitor!");
	}

	return Continue();
}

AITask<CTF2Bot>* CTF2BotScenarioTask::SelectScenarioTask(CTF2Bot* me, const bool skipClassBehavior)
{
	auto tf2mod = CTeamFortress2Mod::GetTF2Mod();
	auto gm = tf2mod->GetCurrentGameMode();
	bool defend = (randomgen->GetRandomInt<int>(1, 100) < tf2mod->GetModSettings()->GetDefendRate());

	// Current gamemode is deathmatch, no class specific behavior is used
	if (tf2mod->UseDeathmatchBehaviorOnly())
	{
		return new CTF2BotDeathmatchScenarioTask;
	}

	if (gm == TeamFortress2::GameModeType::GM_MVM)
	{
		return new CTF2BotMvMMonitorTask; // In MvM, all bots start with this
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
	default:
		break;
	}

	return new CTF2BotDeathmatchScenarioTask;
}

AITask<CTF2Bot>* CTF2BotScenarioTask::SelectClassTask(CTF2Bot* me)
{
	auto myclass = me->GetMyClassType();

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

	constexpr float FOLLOW_TEAMMATE_MAX_RANGE = 1200.0f; // maximum range between the bot and the requesting teammate to follow them.
	TeamFortress2::TFTeam theirteam = static_cast<TeamFortress2::TFTeam>(entityprops::GetEntityTeamNum(subject));
	TeamFortress2::VoiceCommandsID vcmd = static_cast<TeamFortress2::VoiceCommandsID>(command);

	if (!IsPaused() && theirteam == bot->GetMyTFTeam() && bot->GetBehaviorInterface()->ShouldHurry(bot) != ANSWER_YES
		&& bot->GetBehaviorInterface()->ShouldAssistTeammate(bot, subject) != ANSWER_NO &&
		CBaseBot::s_botrng.GetRandomInt<int>(1, 100) >= bot->GetDifficultyProfile()->GetTeamwork())
	{
		if (bot->GetRangeTo(UtilHelpers::getEntityOrigin(subject)) < FOLLOW_TEAMMATE_MAX_RANGE && vcmd == TeamFortress2::VoiceCommandsID::VC_HELP)
		{
			bot->SendVoiceCommand(TeamFortress2::VoiceCommandsID::VC_YES);
			m_respondToTeamMatesTimer.StartRandom(90.0f, 180.0f);
			return TryPauseFor(new CBotSharedEscortEntityTask<CTF2Bot, CTF2BotPathCost>(bot, subject, 30.0f), PRIORITY_MEDIUM, "Responding to teammate call for help!");
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
