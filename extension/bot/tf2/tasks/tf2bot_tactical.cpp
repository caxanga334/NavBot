#include <limits>
#include <cstring>

#include <extension.h>
#include <util/helpers.h>
#include <util/entprops.h>
#include <util/librandom.h>
#include <mods/tf2/teamfortress2mod.h>
#include <entities/tf2/tf_entities.h>
#include <sdkports/sdk_takedamageinfo.h>
#include <mods/tf2/tf2lib.h>
#include "bot/tf2/tf2bot.h"
#include "tf2bot_tactical.h"
#include "tf2bot_find_ammo_task.h"
#include "tf2bot_find_health_task.h"
#include <bot/tf2/tasks/scenario/tf2bot_map_ctf.h>
#include <bot/tf2/tasks/medic/tf2bot_medic_main_task.h>
#include <bot/tf2/tasks/engineer/tf2bot_engineer_main.h>
#include <bot/tf2/tasks/sniper/tf2bot_task_sniper_main.h>
#include <bot/tf2/tasks/spy/tf2bot_spy_tasks.h>
#include <bot/tf2/tasks/scenario/deathmatch/tf2bot_deathmatch.h>
#include "scenario/controlpoints/tf2bot_controlpoints_monitor.h"
#include "scenario/payload/tf2bot_task_defend_payload.h"
#include "scenario/payload/tf2bot_task_push_payload.h"
#include "scenario/mvm/tf2bot_mvm_monitor.h"
#include "scenario/specialdelivery/tf2bot_special_delivery_monitor_task.h"
#include "tf2bot_use_teleporter.h"
#include <bot/tasks_shared/bot_shared_escort_entity.h>

#undef max
#undef min
#undef clamp // undef mathlib macros

#if SOURCE_ENGINE == SE_TF2 // only declare the ConVar on TF2 engine
static ConVar sm_navbot_tf_ai_low_health_percent("sm_navbot_tf_ai_low_health_percent", "0.5", FCVAR_GAMEDLL, "If the bot health is below this percentage, the bot should retreat for health", true, 0.0f, true, 1.0f);
#endif // SOURCE_ENGINE == SE_TF2

AITask<CTF2Bot>* CTF2BotTacticalTask::InitialNextTask(CTF2Bot* bot)
{
	return CTF2BotTacticalTask::SelectScenarioTask(bot);
}

TaskResult<CTF2Bot> CTF2BotTacticalTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	m_ammochecktimer.Start(5.0f);
	m_healthchecktimer.Start(5.0f);
	m_teleportertimer.Start(3.0f);

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotTacticalTask::OnTaskUpdate(CTF2Bot* bot)
{
#if SOURCE_ENGINE == SE_TF2
	// low ammo and health check
	if (bot->GetBehaviorInterface()->ShouldRetreat(bot) != ANSWER_NO && bot->GetBehaviorInterface()->ShouldHurry(bot) != ANSWER_YES)
	{
		if (m_healthchecktimer.IsElapsed())
		{
			m_healthchecktimer.StartRandom(1.0f, 5.0f);

			if (bot->GetHealthPercentage() <= sm_navbot_tf_ai_low_health_percent.GetFloat())
			{
				return PauseFor(new CTF2BotFindHealthTask, "I'm low on health!");
			}
		}

		if (m_ammochecktimer.IsElapsed())
		{
			m_ammochecktimer.StartRandom(1.0f, 5.0f);

			if (bot->IsAmmoLow())
			{
				return PauseFor(new CTF2BotFindAmmoTask, "I'm low on ammo!");
			}
		}
	}
#endif // SOURCE_ENGINE == SE_TF2

	if (m_teleportertimer.IsElapsed())
	{
		CBaseEntity* teleporter = nullptr;

		if (CTF2BotUseTeleporterTask::IsPossible(bot, &teleporter))
		{
			m_teleportertimer.StartRandom(75.0f, 135.0f); // don't search again for a while
			return PauseFor(new CTF2BotUseTeleporterTask(teleporter), "Using teleporter!");
		}
		else
		{
			m_teleportertimer.StartRandom(1.0f, 6.0f);
		}
	}

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotTacticalTask::OnTaskResume(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	m_ammochecktimer.Start(5.0f);
	m_healthchecktimer.Start(0.5f);

	return Continue();
}

QueryAnswerType CTF2BotTacticalTask::ShouldRetreat(CBaseBot* base)
{
	CTF2Bot* me = static_cast<CTF2Bot*>(base);

	if (tf2lib::IsPlayerInvulnerable(me->GetIndex()))
	{
		return ANSWER_NO;
	}

#if SOURCE_ENGINE == SE_TF2
	if (me->GetHealthPercentage() <= sm_navbot_tf_ai_low_health_percent.GetFloat())
	{
		return ANSWER_YES;
	}
#endif // SOURCE_ENGINE == SE_TF2

	if (me->IsAmmoLow())
	{
		return ANSWER_YES;
	}

	return ANSWER_UNDEFINED;
}

AITask<CTF2Bot>* CTF2BotTacticalTask::SelectScenarioTask(CTF2Bot* me, bool skipClassBehavior)
{
	auto tf2mod = CTeamFortress2Mod::GetTF2Mod();
	auto gm = tf2mod->GetCurrentGameMode();
	bool defend = (randomgen->GetRandomInt<int>(1, 100) < tf2mod->GetModSettings()->GetDefendRate());


	if (gm == TeamFortress2::GameModeType::GM_MVM)
	{
		return new CTF2BotMvMMonitorTask; // In MvM, all bots start with this
	}

	if (!skipClassBehavior)
	{
		AITask<CTF2Bot>* task = CTF2BotTacticalTask::SelectClassTask(me);

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
	default:
		break;
	}

	return new CTF2BotDeathmatchScenarioTask;
}

AITask<CTF2Bot>* CTF2BotTacticalTask::SelectClassTask(CTF2Bot* me)
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
		return new CTF2BotSpyInfiltrateTask;
	case TeamFortress2::TFClass_Engineer:
		return new CTF2BotEngineerMainTask;
	default:
		break;
	}

	return nullptr;
}

TaskEventResponseResult<CTF2Bot> CTF2BotTacticalTask::OnInjured(CTF2Bot* bot, const CTakeDamageInfo& info)
{
	if (bot->GetDifficultyProfile()->GetGameAwareness() >= 15)
	{
		CBaseEntity* inflictor = info.GetInflictor();

		if (inflictor != nullptr)
		{
			int index = gamehelpers->EntityToBCompatRef(inflictor);

			if (index > 0 && index <= MAX_EDICTS)
			{
				Vector pos = UtilHelpers::getWorldSpaceCenter(inflictor);
				bot->GetControlInterface()->AimAt(pos, IPlayerController::LOOK_DANGER, 2.0f, "Aiming at damage source!");
			}
		}
	}

	return TryContinue(PRIORITY_DONT_CARE);
}

TaskEventResponseResult<CTF2Bot> CTF2BotTacticalTask::OnVoiceCommand(CTF2Bot* bot, CBaseEntity* subject, int command)
{
	int skill = bot->GetDifficultyProfile()->GetGameAwareness();

	if (skill <= 5)
	{
		return TryContinue(PRIORITY_DONT_CARE); // this one doesn't know about VCs yet
	}

	TeamFortress2::TFTeam theirteam = static_cast<TeamFortress2::TFTeam>(entityprops::GetEntityTeamNum(subject));
	TeamFortress2::VoiceCommandsID vcmd = static_cast<TeamFortress2::VoiceCommandsID>(command);

	if (skill >= 75 && theirteam != bot->GetMyTFTeam())
	{
		if (bot->GetRangeTo(subject) < bot->GetSensorInterface()->GetMaxHearingRange())
		{
			CKnownEntity* known = bot->GetSensorInterface()->AddKnownEntity(subject);
			Vector pos = UtilHelpers::getWorldSpaceCenter(subject);
			bot->GetControlInterface()->AimAt(pos, IPlayerController::LOOK_ALERT, 2.0f, "Looking at hostile voice command source position!");

			if (known)
			{
				// this will update the entity position and mark it as heard
				known->UpdateHeard();
			}
		}
	}
	else if (!IsPaused() && theirteam == bot->GetMyTFTeam() && skill > 15 && bot->GetBehaviorInterface()->ShouldAssistTeammate(bot, subject) != ANSWER_NO &&
		CBaseBot::s_botrng.GetRandomInt<int>(0, 100) >= bot->GetDifficultyProfile()->GetTeamwork())
	{
		switch (bot->GetMyClassType())
		{
		case TeamFortress2::TFClassType::TFClass_Scout:
			[[fallthrough]];
		case TeamFortress2::TFClassType::TFClass_Soldier:
			[[fallthrough]];
		case TeamFortress2::TFClassType::TFClass_Pyro:
			[[fallthrough]];
		case TeamFortress2::TFClassType::TFClass_DemoMan:
			[[fallthrough]];
		case TeamFortress2::TFClassType::TFClass_Heavy:
		{
			if (bot->GetRangeTo(UtilHelpers::getEntityOrigin(subject)) < 1200.0f && vcmd == TeamFortress2::VoiceCommandsID::VC_HELP)
			{
				bot->SendVoiceCommand(TeamFortress2::VoiceCommandsID::VC_YES);
				return TryPauseFor(new CBotSharedEscortEntityTask<CTF2Bot, CTF2BotPathCost>(bot, subject, 30.0f), PRIORITY_MEDIUM, "Responding to teammate call for help!");
			}

			break;
		}
		default:
			break;
		}
	}

	return TryContinue(PRIORITY_DONT_CARE);
}

