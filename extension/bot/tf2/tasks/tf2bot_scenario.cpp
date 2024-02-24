#include <extension.h>
#include <mods/basemod.h>
#include <mods/tf2/teamfortress2mod.h>
#include "bot/tf2/tf2bot.h"
#include <bot/tf2/tasks/scenario/tf2bot_map_ctf.h>
#include <bot/tf2/tasks/medic/tf2bot_medic_main_task.h>
#include "tf2bot_scenario.h"

TaskResult<CTF2Bot> CTF2BotScenarioTask::OnTaskUpdate(CTF2Bot* bot)
{
	auto tf2mod = CTeamFortress2Mod::GetTF2Mod();
	auto gm = tf2mod->GetCurrentGameMode();
	auto myclass = bot->GetMyClassType();

	if (myclass == TeamFortress2::TFClass_Medic)
	{
		return SwitchTo(new CTF2BotMedicMainTask, "Starting Medic Behavior");
	}
	
	switch (gm)
	{
	case TeamFortress2::GameModeType::GM_CTF:
	{
		auto newtask = new CTF2BotCTFMonitorTask;
		return SwitchTo(newtask, "Starting CTF Behavior");
	}
	default:
		break;
	}

	return Continue();
}
