#include NAVBOT_PCH_FILE
#include <vector>

#include <extension.h>
#include <mods/tf2/nav/tfnavmesh.h>
#include <mods/tf2/nav/tfnavarea.h>
#include <navmesh/nav_pathfind.h>
#include <util/librandom.h>
#include <bot/tf2/tf2bot.h>
#include "tf2bot_roam.h"
#include <bot/tasks_shared/bot_shared_attack_enemy.h>

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
		if (!FindRandomGoalPosition(bot))
		{
			return Done("Failed to find a random destination!");
		}
	}

	CTF2BotPathCost cost(bot);
	if (!m_nav.ComputePathToPosition(bot, m_goal, cost))
	{
		return Done("Failed to find a path to the goal position!");
	}

	m_nav.StartRepathTimer();
	return Continue();
}

TaskResult<CTF2Bot> CTF2BotRoamTask::OnTaskUpdate(CTF2Bot* bot)
{
	if (bot->GetRangeTo(m_goal) <= 48.0f)
	{
		return Done("Goal Reached!");
	}

	auto threat = bot->GetSensorInterface()->GetPrimaryKnownThreat(true);

	if (threat)
	{
		if (randomgen->GetRandomInt<int>(1, 100) <= bot->GetDifficultyProfile()->GetAggressiveness())
		{
			m_nav.Invalidate();
			m_nav.ForceRepath();
			return PauseFor(new CBotSharedAttackEnemyTask<CTF2Bot, CTF2BotPathCost>(bot, 5.0f), "Attacking visible threat!");
		}
	}

	if (m_nav.NeedsRepath())
	{
		m_nav.StartRepathTimer();

		CTF2BotPathCost cost(bot);
		if (!m_nav.ComputePathToPosition(bot, m_goal, cost))
		{
			return Done("Failed to find a path to the goal position!");
		}
	}

	m_nav.Update(bot);

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotRoamTask::OnTaskResume(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	// if the bot was using a rnadom position, find another after resuming.
	if (!m_hasgoal)
	{
		if (!FindRandomGoalPosition(bot))
		{
			return Done("Failed to find a random destination!");
		}
	}

	return Continue();
}

TaskEventResponseResult<CTF2Bot> CTF2BotRoamTask::OnMoveToFailure(CTF2Bot* bot, CPath* path, IEventListener::MovementFailureType reason)
{
	if (++m_failcount > 30)
	{
		return TryDone(PRIORITY_HIGH, "Stuck limit reached, abandoning!");
	}

	m_nav.StartRepathTimer();
	CTF2BotPathCost cost(bot);
	m_nav.ComputePathToPosition(bot, m_goal, cost);

	return TryContinue(PRIORITY_LOW);
}

TaskEventResponseResult<CTF2Bot> CTF2BotRoamTask::OnMoveToSuccess(CTF2Bot* bot, CPath* path)
{
	return TryDone(PRIORITY_HIGH, "Goal Reached!");
}

bool CTF2BotRoamTask::FindRandomGoalPosition(CTF2Bot* me)
{
	me->UpdateLastKnownNavArea(true);
	auto myarea = me->GetLastKnownNavArea();

	if (myarea == nullptr)
		return false;

	RoamCollector collector(myarea, me->GetCurrentTeamIndex());
	collector.Execute();

	if (collector.IsCollectedAreasEmpty())
	{
		return false;
	}

	auto goalarea = collector.GetRandomCollectedArea();

	m_goal = goalarea->GetRandomPoint();

	return true;
}