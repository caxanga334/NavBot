#include <cstring>
#include <extension.h>
#include <bot/tf2/tf2bot.h>
#include "tf2bot_task_sniper_main.h"
#include "tf2bot_task_sniper_move_to_sniper_spot.h"

CTF2BotSniperMainTask::CTF2BotSniperMainTask()
{
}

TaskResult<CTF2Bot> CTF2BotSniperMainTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	return Continue();
}

TaskResult<CTF2Bot> CTF2BotSniperMainTask::OnTaskUpdate(CTF2Bot* bot)
{
	/* time to go sniping */

	return PauseFor(new CTF2BotSniperMoveToSnipingSpotTask, "Sniping!");
}
