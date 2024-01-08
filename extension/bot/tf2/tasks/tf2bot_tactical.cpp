#include "bot/tf2/tf2bot.h"
#include "tf2bot_scenario.h"
#include "bot/tf2/tasks/tf2bot_tactical.h"


AITask<CTF2Bot>* CTF2BotTacticalTask::InitialNextTask()
{
	return new CTF2BotScenarioTask;
}

TaskResult<CTF2Bot> CTF2BotTacticalTask::OnTaskUpdate(CTF2Bot* bot)
{
	return Continue();
}
