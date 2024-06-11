#include <extension.h>
#include <bot/tf2/tf2bot.h>
#include <bot/tf2/tasks/tf2bot_scenario.h>
#include "tf2bot_engineer_main.h"

TaskResult<CTF2Bot> CTF2BotEngineerMainTask::OnTaskUpdate(CTF2Bot* bot)
{
	return Continue();
}
