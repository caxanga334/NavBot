#include NAVBOT_PCH_FILE
#include <bot/zps/zpsbot.h>
#include <mods/zps/zps_mod.h>
#include "scenario/zpsbot_survival_human_task.h"
#include "zpsbot_scenario_task.h"

CZPSBotScenarioTask::CZPSBotScenarioTask()
{
	m_roundisactive = CZombiePanicSourceMod::GetZPSMod()->IsRoundActive();
}

AITask<CZPSBot>* CZPSBotScenarioTask::SelectScenarioTask(CZPSBot* bot)
{
	zps::ZPSGamemodes gamemode = CZombiePanicSourceMod::GetZPSMod()->GetGameMode();

	if (gamemode == zps::ZPSGamemodes::GAMEMODE_SURVIVAL)
	{
		if (bot->GetMyZPSTeam() == zps::ZPSTeam::ZPS_TEAM_SURVIVORS)
		{
			// survivor bot
			return new CZPSBotSurvivalHumanTask;
		}
		else
		{
			// zombie bot
			return nullptr;
		}
	}

	return nullptr;
}

AITask<CZPSBot>* CZPSBotScenarioTask::InitialNextTask(CZPSBot* bot)
{
	// Wait until the round is active to pick a scenario behavior
	if (!m_roundisactive)
	{
		return nullptr;
	}

	return CZPSBotScenarioTask::SelectScenarioTask(bot);
}

TaskResult<CZPSBot> CZPSBotScenarioTask::OnTaskUpdate(CZPSBot* bot)
{
	if (!m_roundisactive)
	{
		if (CZombiePanicSourceMod::GetZPSMod()->IsRoundActive())
		{
			return SwitchTo(new CZPSBotScenarioTask, "Round is active, restarting scenario behavior!");
		}

		return Continue();
	}


	return Continue();
}

QueryAnswerType CZPSBotScenarioTask::ShouldHurry(CBaseBot* me)
{
	if (!m_roundisactive)
	{
		return ANSWER_YES;
	}

	return ANSWER_UNDEFINED;
}

QueryAnswerType CZPSBotScenarioTask::ShouldRetreat(CBaseBot* me)
{
	if (!m_roundisactive)
	{
		return ANSWER_NO;
	}

	return ANSWER_UNDEFINED;
}
