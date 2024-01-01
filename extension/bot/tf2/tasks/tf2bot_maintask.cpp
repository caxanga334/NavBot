#include "bot/tf2/tf2bot.h"
#include "bot/tf2/tasks/tf2bot_maintask.h"

TaskResult<CTF2Bot> CTF2BotMainTask::OnTaskUpdate(CTF2Bot* bot)
{
	return Continue();
}
