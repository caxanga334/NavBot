#include NAVBOT_PCH_FILE
#include <bot/insmic/insmicbot.h>
#include <bot/tasks_shared/bot_shared_simple_deathmatch_behavior.h>
#include <mods/insmic/insmicmod.h>
#include <bot/interfaces/behavior_utils.h>
#include <bot/tasks_shared/bot_shared_rogue_behavior.h>
#include "scenario/insmicbot_checkpoint_monitor_task.h"
#include "insmicbot_scenario_task.h"

CInsMICBotScenarioTask::CInsMICBotScenarioTask()
{
}

AITask<CInsMICBot>* CInsMICBotScenarioTask::InitialNextTask(CInsMICBot* bot)
{
	switch (CInsMICMod::GetInsurgencyMod()->GetGameType())
	{
	case insmic::InsMICGameTypes_t::GAMETYPE_BATTLE:
		[[fallthrough]];
	case insmic::InsMICGameTypes_t::GAMETYPE_FIREFIGHT:
		[[fallthrough]];
	case insmic::InsMICGameTypes_t::GAMETYPE_PUSH:
		[[fallthrough]];
	case insmic::InsMICGameTypes_t::GAMETYPE_STRIKE:
		return new CInsMICBotCheckPointMonitorTask;
	case insmic::InsMICGameTypes_t::GAMETYPE_TDM:
		[[fallthrough]];
	default:
		break;
	}

	// fallback to deathmatch behavior
	return new CBotSharedSimpleDMBehaviorTask<CInsMICBot, CInsMICBotPathCost>;
}

TaskResult<CInsMICBot> CInsMICBotScenarioTask::OnTaskStart(CInsMICBot* bot, AITask<CInsMICBot>* pastTask)
{
	BOTBEHAVIOR_IMPLEMENT_SIMPLE_ROGUE_CHECK(CInsMICBot, CInsMICBotPathCost);

	return Continue();
}

TaskResult<CInsMICBot> CInsMICBotScenarioTask::OnTaskUpdate(CInsMICBot* bot)
{
	return Continue();
}
