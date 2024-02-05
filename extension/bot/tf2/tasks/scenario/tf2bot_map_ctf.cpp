#include <extension.h>
#include <sdkports/sdk_timers.h>
#include <entities/tf2/tf_entities.h>
#include "bot/tf2/tf2bot.h"
#include "tf2bot_map_ctf.h"

TaskResult<CTF2Bot> CTF2BotCTFMonitorTask::OnTaskUpdate(CTF2Bot* bot)
{
	if (bot->IsCarryingAFlag())
	{
		return Continue(); // TO-DO: Deliver flag task
	}
	else
	{
		auto flag = bot->GetFlagToFetch();

		if (flag)
		{
			auto task = new CTF2BotCTFFetchFlagTask(flag);
			return PauseFor(task, "Going to fetch the flag!");
		}
	}

	return Continue();
}

CTF2BotCTFFetchFlagTask::CTF2BotCTFFetchFlagTask(edict_t* flag)
{
	gamehelpers->SetHandleEntity(m_flag, flag);
}

TaskResult<CTF2Bot> CTF2BotCTFFetchFlagTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	auto edict = gamehelpers->GetHandleEntity(m_flag);

	if (!edict)
		return Done("Flag is NULL!");

	tfentities::HCaptureFlag flag(edict);

	m_goalpos = flag.GetPosition();

	CTF2BotPathCost cost(bot);
	
	if (!m_nav.ComputePathToPosition(bot, m_goalpos, cost))
	{
		return Done("Failed to find a path to the flag!");
	}

	m_repathtimer.Start(2.5f);

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotCTFFetchFlagTask::OnTaskUpdate(CTF2Bot* bot)
{
	auto edict = gamehelpers->GetHandleEntity(m_flag);

	if (!edict)
		return Done("Flag is NULL!");

	tfentities::HCaptureFlag flag(edict);

	if (flag.IsStolen())
	{
		return Done("Flag is stolen!");
	}

	if (m_repathtimer.IsElapsed())
	{
		m_repathtimer.Reset();
		m_goalpos = flag.GetPosition();

		CTF2BotPathCost cost(bot);
		if (!m_nav.ComputePathToPosition(bot, m_goalpos, cost))
		{
			return Done("Failed to find a path to the flag!");
		}
	}

	m_nav.Update(bot);
	return Continue();
}
