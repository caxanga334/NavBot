#include <extension.h>
#include <util/entprops.h>
#include <mods/tf2/tf2lib.h>
#include <mods/tf2/teamfortress2mod.h>
#include <mods/tf2/nav/tfnavmesh.h>
#include <mods/tf2/nav/tfnavarea.h>
#include <bot/tf2/tf2bot.h>
#include <bot/tf2/tasks/tf2bot_scenario_task.h>
#include "tf2bot_mvm_idle.h"
#include "tf2bot_mvm_defend.h"
#include "tf2bot_mvm_tasks.h"
#include "tf2bot_mvm_monitor.h"

AITask<CTF2Bot>* CTF2BotMvMMonitorTask::InitialNextTask(CTF2Bot* bot)
{
	auto classTask = CTF2BotScenarioTask::SelectClassTask(bot);

	if (classTask != nullptr)
	{
		return classTask;
	}

	// classes without class specific behavior runs the defend task
	return new CTF2BotMvMDefendTask;
}

TaskResult<CTF2Bot> CTF2BotMvMMonitorTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	return Continue();
}

TaskResult<CTF2Bot> CTF2BotMvMMonitorTask::OnTaskUpdate(CTF2Bot* bot)
{
	auto threat = bot->GetSensorInterface()->GetPrimaryKnownThreat(true);

#if SOURCE_ENGINE == SE_TF2
	// always runs this since there might be currency dropped after the wave has ended
	if (CTF2BotCollectMvMCurrencyTask::IsAllowedToCollectCurrency() && m_currencyScan.IsElapsed())
	{
		m_currencyScan.StartRandom(3.0f, 6.0f);
		std::vector<CHandle<CBaseEntity>> currencypacks;
		currencypacks.reserve(64);
		CTF2BotCollectMvMCurrencyTask::ScanForDroppedCurrency(currencypacks);

		if (!currencypacks.empty())
		{
			// scouts always collect, other classes collect if no visible enemy
			bool collect = bot->GetMyClassType() == TeamFortress2::TFClass_Scout;

			// special conditions for spies to help scouts
			if (currencypacks.size() > 6 && bot->GetMyClassType() == TeamFortress2::TFClass_Spy)
			{
				collect = true;
			}
			else if (!threat && currencypacks.size() > 9)
			{
				// other classes will collect if there are more than 9 dropped
				collect = true;
			}

			if (collect)
			{
				return PauseFor(new CTF2BotCollectMvMCurrencyTask(currencypacks), "Going to collect currency!");
			}
		}
	}
#endif // SOURCE_ENGINE == SE_TF2

	if (entprops->GameRules_GetRoundState() == RoundState_BetweenRounds)
	{
		// mvm idle prevents medic from building uber and engineers from placing buildings
		if (bot->GetMyClassType() == TeamFortress2::TFClass_Engineer || bot->GetMyClassType() == TeamFortress2::TFClass_Medic)
		{
			if (!m_hasUpgradedInThisWave)
			{
				m_hasUpgradedInThisWave = true;
				return PauseFor(new CTF2BotMvMIdleTask, "Going to between waves behavior!");
			}
		}
		else
		{
			m_hasUpgradedInThisWave = true;
			return PauseFor(new CTF2BotMvMIdleTask, "Going to between waves behavior!");
		}

		// for medics and engineers
		if (bot->GetBehaviorInterface()->IsReady(bot) == ANSWER_YES)
		{
			if (!bot->TournamentIsReady() && tf2lib::MVM_ShouldBotsReadyUp())
			{
				bot->ToggleTournamentReadyStatus(true);
			}
		}
	}

	return Continue();
}

TaskEventResponseResult<CTF2Bot> CTF2BotMvMMonitorTask::OnRoundStateChanged(CTF2Bot* bot)
{
	m_hasUpgradedInThisWave = false;

	return TryContinue(PRIORITY_DONT_CARE);
}
