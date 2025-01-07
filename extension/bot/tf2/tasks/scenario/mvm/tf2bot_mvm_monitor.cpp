#include <extension.h>
#include <util/entprops.h>
#include <mods/tf2/tf2lib.h>
#include <mods/tf2/teamfortress2mod.h>
#include <mods/tf2/nav/tfnavmesh.h>
#include <mods/tf2/nav/tfnavarea.h>
#include <bot/tf2/tf2bot.h>
#include <bot/tf2/tasks/tf2bot_tactical.h>
#include "tf2bot_mvm_idle.h"
#include "tf2bot_mvm_defend.h"
#include "tf2bot_mvm_monitor.h"

AITask<CTF2Bot>* CTF2BotMvMMonitorTask::InitialNextTask(CTF2Bot* bot)
{
	auto classTask = CTF2BotTacticalTask::SelectClassTask(bot);

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
