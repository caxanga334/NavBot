#include <extension.h>
#include <util/helpers.h>
#include <util/entprops.h>
#include <bot/tf2/tf2bot.h>
#include <mods/tf2/tf2lib.h>
#include <bot/interfaces/path/meshnavigator.h>
#include "tf2bot_passtime_goal_task.h"

TaskResult<CTF2Bot> CTF2BotPassTimeGoalTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	// TO-DO: Add support for other goal types
	// endzone is very easy, hoop is easy too, just need to implement aiming
	// tower gets a little complicated with pathing

	CBaseEntity* goal = tf2lib::passtime::GetGoalEntity(TeamFortress2::PassTimeGoalType::TYPE_ENDZONE, bot->GetMyTFTeam());

	if (!goal)
	{
		return Done("Failed to find goal entity!");
	}

	m_goalent = goal;

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotPassTimeGoalTask::OnTaskUpdate(CTF2Bot* bot)
{
	CBaseEntity* goal = m_goalent.Get();

	if (!goal)
	{
		return Done("Goal ent is NULL!");
	}

	if (!bot->IsCarryingThePassTimeJack())
	{
		return Done("Goal scored or jack is lost!");
	}

	if (m_repathtimer.IsElapsed())
	{
		m_repathtimer.Start(1.0f);
		Vector pos = UtilHelpers::getWorldSpaceCenter(goal);
		CTF2BotPathCost cost(bot);
		m_nav.ComputePathToPosition(bot, pos, cost, 0.0f, true);
	}

	m_nav.Update(bot);

	return Continue();
}
