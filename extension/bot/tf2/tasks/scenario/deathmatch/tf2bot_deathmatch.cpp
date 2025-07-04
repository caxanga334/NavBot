#include NAVBOT_PCH_FILE
#include <extension.h>
#include <bot/interfaces/path/meshnavigator.h>
#include <bot/tf2/tf2bot.h>
#include <bot/tf2/tasks/tf2bot_roam.h>
#include "tf2bot_deathmatch.h"

TaskResult<CTF2Bot> CTF2BotDeathmatchScenarioTask::OnTaskUpdate(CTF2Bot* bot)
{
	return PauseFor(new CTF2BotRoamTask(), "Going to a random destination!");
}
