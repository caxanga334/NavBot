#include <vector>
#include <extension.h>
#include <util/entprops.h>
#include <mods/tf2/tf2lib.h>
#include <mods/tf2/teamfortress2mod.h>
#include <mods/tf2/nav/tfnavmesh.h>
#include <mods/tf2/nav/tfnavarea.h>
#include <bot/tf2/tf2bot.h>
#include "tf2bot_mvm_upgrade.h"
#include "tf2bot_mvm_idle.h"

TaskResult<CTF2Bot> CTF2BotMvMIdleTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	return Continue();
}

TaskResult<CTF2Bot> CTF2BotMvMIdleTask::OnTaskUpdate(CTF2Bot* bot)
{
	if (entprops->GameRules_GetRoundState() == RoundState_RoundRunning)
	{
		// Wave has started!
		// TO-DO: Switch to combat task
		return Continue();
	}

	// Wave has not started yet
	if (entprops->GameRules_GetRoundState() == RoundState_BetweenRounds)
	{
		// Should the bot buy upgrades?
		if (!bot->GetUpgradeManager().IsManagerReady() || bot->GetUpgradeManager().ShouldGoToAnUpgradeStation())
		{
			// Buy upgrades
			return PauseFor(new CTF2BotMvMUpgradeTask, "Going to use an upgrade station!");
		}

		if (m_isready)
		{
			// Bot is not ready yet, toggle ready mode
			if (!bot->TournamentIsReady() && tf2lib::MVM_ShouldBotsReadyUp())
			{
				bot->ToggleTournamentReadyStatus(true);
			}
		}
		else
		{
			CTFNavArea* area = static_cast<CTFNavArea*>(bot->GetLastKnownNavArea());

			if (area && area->HasMVMAttributes(CTFNavArea::MVMNAV_FRONTLINES))
			{
				// bot is at a frontline area
				m_isready = true;
			}
		}
	}

	// TO-DO: Move to defensive positions near the robot spawn
	// TO-DO: Engineer
	// TO-DO: Add ready up logic

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotMvMIdleTask::OnTaskResume(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	CTF2BotMvMUpgradeTask* upgradetask = dynamic_cast<CTF2BotMvMUpgradeTask*>(pastTask);

	if (upgradetask != nullptr)
	{
		m_isready = false; // we just used an upgrade station, go back to the frontlines
	}

	return Continue();
}

TaskEventResponseResult<CTF2Bot> CTF2BotMvMIdleTask::OnMoveToFailure(CTF2Bot* bot, CPath* path, IEventListener::MovementFailureType reason)
{
	return TryContinue();
}

TaskEventResponseResult<CTF2Bot> CTF2BotMvMIdleTask::OnMoveToSuccess(CTF2Bot* bot, CPath* path)
{
	return TryContinue();
}
