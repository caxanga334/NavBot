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
#include "tf2bot_mvm_tasks.h"
#include "tf2bot_mvm_monitor.h"

#if SOURCE_ENGINE == SE_TF2
static ConVar cvar_collect_currency("sm_navbot_tf_mvm_collect_currency", "0", FCVAR_GAMEDLL, "Set to 1 to allow bots to collect MvM currency.");
#endif

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
#if SOURCE_ENGINE == SE_TF2
	if (cvar_collect_currency.GetBool() == true)
	{
		CTF2BotMvMMonitorTask::s_collect = true;
	}
	else
	{
		CTF2BotMvMMonitorTask::s_collect = false;
	}
#endif

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotMvMMonitorTask::OnTaskUpdate(CTF2Bot* bot)
{
	auto threat = bot->GetSensorInterface()->GetPrimaryKnownThreat(true);

	// always runs this since there might be currency dropped after the wave has ended
	if (CTF2BotMvMMonitorTask::s_collect && m_currencyScan.IsElapsed())
	{
		m_currencyScan.StartRandom(1.0f, 2.0f);
		std::vector<CHandle<CBaseEntity>> currencypacks;
		currencypacks.reserve(64);
		ScanForDroppedCurrency(currencypacks);

		if (!currencypacks.empty())
		{
			// scouts always collect, other classes collect if no visible enemy
			bool collect = bot->GetMyClassType() == TeamFortress2::TFClass_Scout || threat.get() == nullptr;
			
			// special conditions for spies to help scouts
			if (currencypacks.size() > 12 && bot->GetMyClassType() == TeamFortress2::TFClass_Spy)
			{
				collect = true;
			}

			if (collect)
			{
				return PauseFor(new CTF2BotCollectMvMCurrencyTask(currencypacks), "Going to collect currency!");
			}
		}
	}

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

void CTF2BotMvMMonitorTask::ScanForDroppedCurrency(std::vector<CHandle<CBaseEntity>>& currencyPacks)
{
	for (int i = gpGlobals->maxClients + 1; i < gpGlobals->maxEntities; i++)
	{
		edict_t* edict = gamehelpers->EdictOfIndex(1);

		if (!UtilHelpers::IsValidEdict(edict))
			continue;

		CBaseEntity* entity = edict->GetIServerEntity()->GetBaseEntity();

		if (UtilHelpers::FClassnameIs(entity, "item_currencypack*"))
		{
			bool distributed = false;
			entprops->GetEntPropBool(i, Prop_Send, "m_bDistributed", distributed);

			if (distributed) // this one doesn't need to be collected (IE: killed by sniper)
				continue;

			currencyPacks.emplace_back(entity);
		}
	}
}
