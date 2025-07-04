#include NAVBOT_PCH_FILE
#include <extension.h>
#include <bot/tf2/tf2bot.h>
#include "tf2bot_maintask.h"
#include "tf2bot_dead.h"

TaskResult<CTF2Bot> CTF2BotDeadTask::OnTaskUpdate(CTF2Bot* bot)
{
	if (bot->IsAlive())
	{
		return SwitchTo(new CTF2BotMainTask, "I'm alive!");
	}

	return Continue();
}
