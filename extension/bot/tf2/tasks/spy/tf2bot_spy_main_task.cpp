#include <extension.h>
#include <bot/tf2/tf2bot.h>
#include <mods/tf2/teamfortress2mod.h>
#include <mods/tf2/tf2lib.h>
#include <util/entprops.h>
#include "tf2bot_spy_main_task.h"
#include "tf2bot_spy_tasks.h"

AITask<CTF2Bot>* CTF2BotSpyMainTask::InitialNextTask(CTF2Bot* bot)
{
	return new CTF2BotSpyInfiltrateTask;
}

TaskResult<CTF2Bot> CTF2BotSpyMainTask::OnTaskUpdate(CTF2Bot* bot)
{
	if (GetNextTask() == nullptr)
	{
		return SwitchTo(new CTF2BotSpyMainTask, "Restarting!");
	}

	return Continue();
}
