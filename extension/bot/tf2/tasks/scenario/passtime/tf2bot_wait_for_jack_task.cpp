#include NAVBOT_PCH_FILE
#include <extension.h>
#include <util/helpers.h>
#include <util/entprops.h>
#include <bot/tf2/tf2bot.h>
#include <mods/tf2/teamfortress2mod.h>
#include <mods/tf2/tf2lib.h>
#include <bot/interfaces/path/meshnavigator.h>
#include <sdkports/sdk_ehandle.h>
#include <sdkports/sdk_traces.h>
#include <bot/tasks_shared/bot_shared_roam.h>
#include <bot/tasks_shared/bot_shared_attack_enemy.h>
#include "tf2bot_pick_jack_task.h"
#include "tf2bot_wait_for_jack_task.h"

TaskResult<CTF2Bot> CTF2BotWaitForJackTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	if (!tf2lib::passtime::GetJackSpawnPosition(&m_goal))
	{
		return SwitchTo(new CBotSharedRoamTask<CTF2Bot, CTF2BotPathCost>(bot, 10000.0f, true), "Failed to find jack spawn position, roaming!");
	}

	m_goal = trace::getground(m_goal);

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotWaitForJackTask::OnTaskUpdate(CTF2Bot* bot)
{
	const CKnownEntity* threat = bot->GetSensorInterface()->GetPrimaryKnownThreat(true);

	if (threat)
	{
		return PauseFor(new CBotSharedAttackEnemyTask<CTF2Bot, CTF2BotPathCost>(bot), "Attacking visible enemy!");
	}

	if (tf2lib::passtime::IsJackActive())
	{
		return SwitchTo(new CTF2BotPickJackTask, "Going after the jack!");
	}

	if (m_repathtimer.IsElapsed())
	{
		m_repathtimer.Start(2.0f);
		CTF2BotPathCost cost(bot, SAFEST_ROUTE);
		m_nav.ComputePathToPosition(bot, m_goal, cost);
	}

	m_nav.Update(bot);

	return Continue();
}
