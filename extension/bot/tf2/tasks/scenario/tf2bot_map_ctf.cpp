#include "bot/tf2/tf2bot.h"
#include "tf2bot_map_ctf.h"

TaskResult<CTF2Bot> CTF2BotCTFMonitorTask::OnTaskUpdate(CTF2Bot* bot)
{
    return Continue();
}
