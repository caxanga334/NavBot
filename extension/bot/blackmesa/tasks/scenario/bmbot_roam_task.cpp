#include NAVBOT_PCH_FILE
#include <extension.h>
#include <bot/blackmesa/bmbot.h>
#include <mods/blackmesa/nav/bm_nav_mesh.h>
#include <navmesh/nav_pathfind.h>
#include "bmbot_roam_task.h"

class BMRandomRoamArea : public INavAreaCollector<CNavArea>
{
public:
	BMRandomRoamArea(CNavArea* area);

	bool ShouldCollect(CNavArea* area) override;
};

BMRandomRoamArea::BMRandomRoamArea(CNavArea* area) :
	INavAreaCollector<CNavArea>(area, 8192)
{
}

bool BMRandomRoamArea::ShouldCollect(CNavArea* area)
{
	// don't go to small areas
	if (area->GetSizeX() < 24.0f || area->GetSizeY() < 24.0f)
	{
		return false;
	}

	return true;
}

TaskResult<CBlackMesaBot> CBlackMesaBotRoamTask::OnTaskStart(CBlackMesaBot* bot, AITask<CBlackMesaBot>* pastTask)
{
	Vector start = bot->GetAbsOrigin();

	if (!SelectRandomGoal(start))
	{
		return Done("Failed to find a random destination nav area!");
	}

	CBlackMesaBotPathCost cost(bot);
	if (!m_nav.ComputePathToPosition(bot, m_goal, cost))
	{
		return Done("No path to random roam goal!");
	}

	m_repathTimer.Start(1.0f);
	m_failCount = 0;

	return Continue();
}

TaskResult<CBlackMesaBot> CBlackMesaBotRoamTask::OnTaskUpdate(CBlackMesaBot* bot)
{
	if (m_repathTimer.IsElapsed())
	{
		m_repathTimer.Start(1.0f);
		CBlackMesaBotPathCost cost(bot);
		if (!m_nav.ComputePathToPosition(bot, m_goal, cost))
		{
			return Done("No path to random roam goal!");
		}
	}

	m_nav.Update(bot);

	return Continue();
}

TaskEventResponseResult<CBlackMesaBot> CBlackMesaBotRoamTask::OnMoveToFailure(CBlackMesaBot* bot, CPath* path, IEventListener::MovementFailureType reason)
{
	m_repathTimer.Invalidate();
	
	if (++m_failCount >= 20)
	{
		return TryDone(PRIORITY_HIGH, "Too many path failures, aborting!");
	}

	return TryContinue();
}

TaskEventResponseResult<CBlackMesaBot> CBlackMesaBotRoamTask::OnMoveToSuccess(CBlackMesaBot* bot, CPath* path)
{
	return TryDone(PRIORITY_HIGH, "Goal reached!");
}

bool CBlackMesaBotRoamTask::SelectRandomGoal(const Vector& start)
{
	CNavArea* startArea = TheNavMesh->GetNearestNavArea(start, 1024.0f);

	if (!startArea)
	{
		return false;
	}

	BMRandomRoamArea collector(startArea);

	collector.Execute();

	if (collector.IsCollectedAreasEmpty())
	{
		return false;
	}

	CNavArea* goalArea = collector.GetRandomCollectedArea();
	m_goal = goalArea->GetCenter();

	return true;
}


