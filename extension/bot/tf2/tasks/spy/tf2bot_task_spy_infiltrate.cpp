#include <extension.h>
#include <bot/tf2/tf2bot.h>
#include <mods/tf2/nav/tfnavarea.h>
#include <navmesh/nav_pathfind.h>
#include "tf2bot_task_spy_infiltrate.h"

TaskResult<CTF2Bot> CTF2BotSpyInfiltrateTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	return Continue();
}

TaskResult<CTF2Bot> CTF2BotSpyInfiltrateTask::OnTaskUpdate(CTF2Bot* bot)
{
	if (!bot->IsDisguised())
	{
		DisguiseMe(bot);
	}

	return Continue();
}

void CTF2BotSpyInfiltrateTask::DisguiseMe(CTF2Bot* me)
{
	if (m_disguiseCooldown.IsElapsed())
	{
		me->DisguiseAs(static_cast<TeamFortress2::TFClassType>(randomgen->GetRandomInt<int>(1, 9)));
		m_disguiseCooldown.Start(2.0f);
	}
}
