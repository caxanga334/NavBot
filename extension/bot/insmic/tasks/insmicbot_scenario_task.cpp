#include NAVBOT_PCH_FILE
#include <bot/insmic/insmicbot.h>
#include <bot/tasks_shared/bot_shared_simple_deathmatch_behavior.h>
#include <mods/insmic/insmicmod.h>
#include "insmicbot_scenario_task.h"

CInsMICBotScenarioTask::CInsMICBotScenarioTask()
{
}

AITask<CInsMICBot>* CInsMICBotScenarioTask::InitialNextTask(CInsMICBot* bot)
{
	switch (CInsMICMod::GetInsurgencyMod()->GetGameType())
	{
	case insmic::InsMICGameTypes_t::GAMETYPE_TDM:
		[[fallthrough]];
	default:
		break;
	}

	// fallback to deathmatch behavior
	return new CBotSharedSimpleDMBehaviorTask<CInsMICBot, CInsMICBotPathCost>;
}

TaskResult<CInsMICBot> CInsMICBotScenarioTask::OnTaskUpdate(CInsMICBot* bot)
{
	return Continue();
}
