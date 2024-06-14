#include <extension.h>
#include <bot/tf2/tf2bot.h>
#include "tf2bot_engineer_nest.h"
#include "tf2bot_engineer_main.h"

AITask<CTF2Bot>* CTF2BotEngineerMainTask::InitialNextTask(CTF2Bot* bot)
{
	return new CTF2BotEngineerNestTask;
}

TaskResult<CTF2Bot> CTF2BotEngineerMainTask::OnTaskUpdate(CTF2Bot* bot)
{
	/*
	* @TODO
	* - Help allied engineers
	* - Listen and respond to help requests from other engineers
	*/

	return Continue();
}
