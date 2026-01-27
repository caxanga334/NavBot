#include NAVBOT_PCH_FILE
#include <bot/insmic/insmicbot.h>
#include <mods/insmic/insmicmod.h>
#include <bot/interfaces/path/meshnavigator.h>
#include <bot/bot_shared_utils.h>
#include <coordsize.h>
#include "insmicbot_checkpoint_patrol_objective_task.h"

CInsMICBotCheckPointPatrolObjectiveTask::CInsMICBotCheckPointPatrolObjectiveTask(CBaseEntity* objective) :
	m_objective(objective), m_moveTo(0.0f, 0.0f, 0.0f)
{
}

TaskResult<CInsMICBot> CInsMICBotCheckPointPatrolObjectiveTask::OnTaskStart(CInsMICBot* bot, AITask<CInsMICBot>* pastTask)
{
	CBaseEntity* entity = m_objective.Get();

	if (!entity)
	{
		return Done("NULL objective entity!");
	}

	std::vector<CNavArea*> objareas;
	TheNavMesh->CollectAreasTouchingEntity(entity, true, objareas);

	if (objareas.empty())
	{
		return Done("No nav areas inside objective entity bounds!");
	}

	botsharedutils::IsReachableAreas search{ bot, MAX_COORD_FLOAT };
	search.Execute();

	if (search.IsCollectedAreasEmpty())
	{
		return Done("Cannot determine if the objective is reachable!");
	}

	std::vector<CNavArea*> reachable;

	for (CNavArea* area : objareas)
	{
		if (search.IsReachable(area))
		{
			reachable.push_back(area);
		}
	}

	if (reachable.empty())
	{
		return Done("Cannot reach objective via nav mesh!");
	}

	CNavArea* area = librandom::utils::GetRandomElementFromVector(reachable);
	m_moveTo = area->GetRandomPoint();

	return Continue();
}

TaskResult<CInsMICBot> CInsMICBotCheckPointPatrolObjectiveTask::OnTaskUpdate(CInsMICBot* bot)
{
	CBaseEntity* entity = m_objective.Get();

	if (!entity)
	{
		return Done("NULL objective entity!");
	}

	if (insmiclib::GetObjectiveOwnerTeam(entity) != bot->GetMyInsurgencyTeam())
	{
		return Done("Objective captured by the enemy team!");
	}

	if (!m_nav.IsValid() || m_nav.NeedsRepath())
	{
		CInsMICBotPathCost cost{ bot };
		m_nav.ComputePathToPosition(bot, m_moveTo, cost);
		m_nav.StartRepathTimer();
	}

	m_nav.Update(bot);
	return Continue();
}

TaskEventResponseResult<CInsMICBot> CInsMICBotCheckPointPatrolObjectiveTask::OnMoveToSuccess(CInsMICBot* bot, CPath* path)
{
	return TryDone(PRIORITY_HIGH, "Patrol point reached");
}
