#include <vector>

#include <extension.h>
#include <mods/tf2/nav/tfnavmesh.h>
#include <mods/tf2/nav/tfnavarea.h>
#include <navmesh/nav_pathfind.h>
#include <util/librandom.h>
#include <bot/tf2/tf2bot.h>
#include "tf2bot_roam.h"

class RoamCollector : public INavAreaCollector<CTFNavArea>
{
public:
	RoamCollector(CNavArea* start, int team) : INavAreaCollector(static_cast<CTFNavArea*>(start), 6000.0f)
	{
		m_team = team;
	}

	bool ShouldCollect(CTFNavArea* area) override;
private:
	int m_team;
};

bool RoamCollector::ShouldCollect(CTFNavArea* area)
{
	if (area->IsBlocked(m_team))
	{
		return false;
	}

	return true;
}

CTF2BotRoamTask::CTF2BotRoamTask(const Vector& goal)
{
	m_goal = goal;
	m_hasgoal = true;
	m_failcount = 0;
}

TaskResult<CTF2Bot> CTF2BotRoamTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	if (!m_hasgoal)
	{
		FindRandomGoalPosition(bot);
	}

	CTF2BotPathCost cost(bot);
	if (!m_nav.ComputePathToPosition(bot, m_goal, cost))
	{
		return Done("Failed to find a path to the goal position!");
	}

	m_repathtimer.Start(randomgen->GetRandomReal<float>(1.0f, 3.0f));
	return Continue();
}

TaskResult<CTF2Bot> CTF2BotRoamTask::OnTaskUpdate(CTF2Bot* bot)
{
	if (bot->GetRangeTo(m_goal) <= 48.0f)
	{
		return Done("Goal Reached!");
	}

	if (m_repathtimer.IsElapsed())
	{
		m_repathtimer.Start(randomgen->GetRandomReal<float>(1.0f, 3.0f));

		CTF2BotPathCost cost(bot);
		if (!m_nav.ComputePathToPosition(bot, m_goal, cost))
		{
			return Done("Failed to find a path to the goal position!");
		}
	}

	m_nav.Update(bot);

	return Continue();
}

TaskEventResponseResult<CTF2Bot> CTF2BotRoamTask::OnMoveToFailure(CTF2Bot* bot, CPath* path, IEventListener::MovementFailureType reason)
{
	if (++m_failcount > 30)
	{
		return TryDone(PRIORITY_HIGH, "Stuck limit reached, abandoning!");
	}

	bot->GetMovementInterface()->ClearStuckStatus("Repath");

	m_repathtimer.Start(randomgen->GetRandomReal<float>(1.0f, 3.0f));
	CTF2BotPathCost cost(bot);
	m_nav.ComputePathToPosition(bot, m_goal, cost);

	return TryContinue(PRIORITY_LOW);
}

TaskEventResponseResult<CTF2Bot> CTF2BotRoamTask::OnMoveToSuccess(CTF2Bot* bot, CPath* path)
{
	return TryDone(PRIORITY_HIGH, "Goal Reached!");
}

void CTF2BotRoamTask::FindRandomGoalPosition(CTF2Bot* me)
{
	me->UpdateLastKnownNavArea(true);
	auto myarea = me->GetLastKnownNavArea();

	if (myarea == nullptr)
		return;

	RoamCollector collector(myarea, me->GetCurrentTeamIndex());
	collector.Execute();

	if (collector.IsCollectedAreasEmpty())
	{
		return;
	}

	auto goalarea = collector.GetRandomCollectedArea();

	m_goal = goalarea->GetRandomPoint();
}