#include <cstring>
#include <extension.h>
#include <mods/tf2/teamfortress2mod.h>
#include <bot/tf2/tf2bot.h>
#include <bot/tf2/tasks/scenario/tf2bot_map_ctf.h>
#include <bot/tf2/tasks/scenario/specialdelivery/tf2bot_sd_deliver_flag.h>
#include "tf2bot_task_sniper_main.h"
#include "tf2bot_task_sniper_move_to_sniper_spot.h"
#include "tf2bot_task_sniper_push.h"

CTF2BotSniperMainTask::CTF2BotSniperMainTask()
{
}

TaskResult<CTF2Bot> CTF2BotSniperMainTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	return Continue();
}

TaskResult<CTF2Bot> CTF2BotSniperMainTask::OnTaskUpdate(CTF2Bot* bot)
{
	// aggressive sniper bots have 50% to push towards the objective
	if (bot->GetDifficultyProfile()->GetAggressiveness() >= 50 && CBaseBot::s_botrng.GetRandomInt<int>(0, 1) == 1)
	{
		return PauseFor(new CTF2BotSniperPushTask, "Pushing to the objective!");
	}

	/* time to go sniping */
	return PauseFor(new CTF2BotSniperMoveToSnipingSpotTask, "Sniping!");
}

TaskEventResponseResult<CTF2Bot> CTF2BotSniperMainTask::OnFlagTaken(CTF2Bot* bot, CBaseEntity* player)
{
	if (bot->GetEntity() == player)
	{
		switch (CTeamFortress2Mod::GetTF2Mod()->GetCurrentGameMode())
		{
		case TeamFortress2::GameModeType::GM_CTF:
		{
			return TryPauseFor(new CTF2BotCTFDeliverFlagTask, PRIORITY_HIGH, "I have the flag!");
		}
		case TeamFortress2::GameModeType::GM_SD:
		{
			return TryPauseFor(new CTF2BotSDDeliverFlag, PRIORITY_HIGH, "I have the australium!");
		}
		default:
			break;
		}
	}

	return TryContinue();
}
