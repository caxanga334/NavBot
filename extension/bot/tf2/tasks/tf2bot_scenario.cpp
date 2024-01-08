#include "bot/tf2/tf2bot.h"
#include "tf2bot_scenario.h"

TaskResult<CTF2Bot> CTF2BotScenarioTask::OnTaskUpdate(CTF2Bot* bot)
{
	return Continue();
}
