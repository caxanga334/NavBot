#include <cstring>

#include <extension.h>
#include <manager.h>
#include <util/helpers.h>
#include <util/librandom.h>
#include <mods/tf2/tf2lib.h>
#include <sdkports/sdk_timers.h>
#include <sdkports/sdk_traces.h>
#include <entities/tf2/tf_entities.h>
#include "bot/tf2/tf2bot.h"
#include "tf2bot_special_delivery_monitor_task.h"
#include "tf2bot_sd_deliver_flag.h"
#include "tf2bot_sd_wait_for_flag.h"

TaskResult<CTF2Bot> CTF2BotSDMonitorTask::OnTaskUpdate(CTF2Bot* bot)
{
	edict_t* item = bot->GetItem();

	if (item)
	{
		const char* classname = gamehelpers->GetEntityClassname(item);

		if (std::strcmp(classname, "item_teamflag") == 0)
		{
			return PauseFor(new CTF2BotSDDeliverFlag, "I have the australium, going to deliver it!");
		}
	}

	return PauseFor(new CTF2BotSDWaitForFlagTask, "Fetching the australium!");
}
