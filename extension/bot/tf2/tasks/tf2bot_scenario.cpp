#include <extension.h>
#include <mods/basemod.h>
#include <mods/tf2/teamfortress2mod.h>
#include "bot/tf2/tf2bot.h"
#include <bot/tf2/tasks/scenario/tf2bot_map_ctf.h>
#include <bot/tf2/tasks/medic/tf2bot_medic_main_task.h>
#include <bot/tf2/tasks/engineer/tf2bot_engineer_main.h>
#include "scenario/mvm/tf2bot_mvm_idle.h"
#include "tf2bot_scenario.h"

AITask<CTF2Bot>* CTF2BotScenarioTask::InitialNextTask(CTF2Bot* bot)
{
	auto tf2mod = CTeamFortress2Mod::GetTF2Mod();
	auto gm = tf2mod->GetCurrentGameMode();
	auto myclass = bot->GetMyClassType();

	if (gm == TeamFortress2::GameModeType::GM_MVM)
	{
		return new CTF2BotMvMIdleTask; // In MvM, all bots start with this
	}

	switch (myclass)
	{
	case TeamFortress2::TFClass_Sniper:
		break; // TODO
	case TeamFortress2::TFClass_Medic:
		return new CTF2BotMedicMainTask;
	case TeamFortress2::TFClass_Spy:
		break; // TODO
	case TeamFortress2::TFClass_Engineer:
		return new CTF2BotEngineerMainTask;
	default:
		break;
	}

	switch (gm)
	{
	case TeamFortress2::GameModeType::GM_CTF:
		return new CTF2BotCTFMonitorTask;
	default:
		break;
	}

	return nullptr;
}

TaskResult<CTF2Bot> CTF2BotScenarioTask::OnTaskUpdate(CTF2Bot* bot)
{
	return Continue();
}
