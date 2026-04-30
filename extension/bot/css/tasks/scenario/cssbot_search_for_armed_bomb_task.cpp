#include NAVBOT_PCH_FILE
#include <mods/css/nav/css_nav_mesh.h>
#include <mods/css/css_lib.h>
#include <mods/css/css_mod.h>
#include <bot/css/cssbot.h>
#include <bot/interfaces/path/meshnavigator.h>
#include "cssbot_defuse_bomb_task.h"
#include "cssbot_search_for_armed_bomb_task.h"

TaskResult<CCSSBot> CCSSBotSearchForArmedBombTask::OnTaskStart(CCSSBot* bot, AITask<CCSSBot>* pastTask)
{
	CBaseEntity* site = UtilHelpers::GetRandomEntityOfClassname("func_bomb_target");

	if (!site)
	{
		return Done("No bomb site entity!");
	}

	Vector pos = UtilHelpers::getWorldSpaceCenter(site);
	CNavArea* area = TheNavMesh->GetNearestNavArea(pos, CPath::PATH_GOAL_MAX_DISTANCE_TO_AREA * 2.0f, false, true, static_cast<int>(bot->GetMyCSSTeam()));

	if (!area)
	{
		return Done("NULL nearest nav area!");
	}

	area->GetClosestPointOnArea(&pos, &m_goal);
	CCSSBotPathCost cost{ bot };
	
	if (m_nav.ComputePathToPosition(bot, m_goal, cost))
	{
		m_nav.StartRepathTimer();
	}

	return Continue();
}

TaskResult<CCSSBot> CCSSBotSearchForArmedBombTask::OnTaskUpdate(CCSSBot* bot)
{
	CCounterStrikeSourceMod* csmod = CCounterStrikeSourceMod::GetCSSMod();

	if (!csmod->IsBombActive())
	{
		return Done("Bomb is no longer armed!");
	}

	CBaseEntity* bomb = csmod->GetActiveBombEntity();

	if (!bomb)
	{
		return Done("NULL active bomb entity!");
	}

	if (bot->GetRangeTo(bomb) <= bot->GetSensorInterface()->GetMaxHearingRange())
	{
		csmod->MarkBombAsKnown();
	}

	if (bot->GetRangeTo(m_goal) <= (bot->GetSensorInterface()->GetMaxHearingRange() * 0.5f))
	{
		// this site is cleared but the bomb is not here, so mark the bomb as known
		// works fine for most maps since they only have two bomb sites.
		csmod->MarkBombAsKnown();
	}

	if (csmod->IsTheBombKnownByCTs())
	{
		return SwitchTo(new CCSSBotDefuseBombTask, "Found bomb, going to defuse it!");
	}

	if (!m_nav.IsValid() || m_nav.NeedsRepath())
	{
		CCSSBotPathCost cost{ bot };
		m_nav.ComputePathToPosition(bot, m_goal, cost);
		m_nav.StartRepathTimer();
	}

	m_nav.Update(bot);

	return Continue();
}

TaskEventResponseResult<CCSSBot> CCSSBotSearchForArmedBombTask::OnMoveToSuccess(CCSSBot* bot, CPath* path)
{
	// in case the hearing check fails
	CCounterStrikeSourceMod::GetCSSMod()->MarkBombAsKnown();
	return TryContinue(PRIORITY_LOW);
}
