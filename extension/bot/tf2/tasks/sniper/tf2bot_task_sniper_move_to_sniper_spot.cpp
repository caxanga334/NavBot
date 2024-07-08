#include <extension.h>
#include <bot/tf2/tf2bot.h>
#include <mods/tf2/nav/tfnavarea.h>
#include <navmesh/nav_pathfind.h>
#include <mods/tf2/teamfortress2mod.h>
#include "tf2bot_task_sniper_snipe_area.h"
#include "tf2bot_task_sniper_move_to_sniper_spot.h"

class SniperSpotAreaCollector : public INavAreaCollector<CTFNavArea>
{
public:
	SniperSpotAreaCollector(CTF2Bot* me, CTFNavArea* startArea) :
		INavAreaCollector<CTFNavArea>(startArea, 8000.0f)
	{
		m_me = me;
	}

	bool ShouldCollect(CTFNavArea* area) override;
private:
	CTF2Bot* m_me;
};

bool SniperSpotAreaCollector::ShouldCollect(CTFNavArea* area)
{
	if (area->GetSizeX() < 16.0f || area->GetSizeY() < 16.0f)
	{
		// don't snipe in small areas
		return false;
	}

	if (area->IsBlocked(m_me->GetCurrentTeamIndex()))
	{
		// don't snipe on blocked areas
		return false;
	}

	return true;
}

TaskResult<CTF2Bot> CTF2BotSniperMoveToSnipingSpotTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	FindSniperSpot(bot);

	CTF2BotPathCost cost(bot);
	m_nav.ComputePathToPosition(bot, m_goal, cost);

	m_repathTimer.StartRandom();
	m_sniping = false;

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotSniperMoveToSnipingSpotTask::OnTaskUpdate(CTF2Bot* bot)
{
	auto mod = CTeamFortress2Mod::GetTF2Mod();

	// non defenders bots don't move during setup
	if (mod->IsInSetup() && mod->GetTeamRole(bot->GetMyTFTeam()) != TeamFortress2::TEAM_ROLE_DEFENDERS)
	{
		return Continue(); 
	}

	if (bot->GetRangeTo(m_goal) <= 36.0f)
	{
		// reached sniping goal
		m_sniping = true;
		return PauseFor(new CTF2BotSniperSnipeAreaTask(), "Sniping!");
	}

	if (m_repathTimer.IsElapsed() || !m_nav.IsValid())
	{
		m_repathTimer.StartRandom();

		CTF2BotPathCost cost(bot);
		m_nav.ComputePathToPosition(bot, m_goal, cost);
	}

	m_nav.Update(bot);

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotSniperMoveToSnipingSpotTask::OnTaskResume(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	if (m_sniping)
	{
		FindSniperSpot(bot);
		m_sniping = false;
	}

	return Continue();
}

void CTF2BotSniperMoveToSnipingSpotTask::FindSniperSpot(CTF2Bot* bot)
{
	// TO-DO: Add game mode sniping spots

	GetRandomSnipingSpot(bot, m_goal);
}

void CTF2BotSniperMoveToSnipingSpotTask::GetRandomSnipingSpot(CTF2Bot* bot, Vector& out)
{
	SniperSpotAreaCollector collector(bot, static_cast<CTFNavArea*>(bot->GetLastKnownNavArea()));

	collector.Execute();

	if (collector.IsCollectedAreasEmpty())
	{
		out = bot->GetAbsOrigin();
		return;
	}

	std::vector<CTFNavArea*> hintAreas;
	hintAreas.reserve(collector.GetCollectedAreasCount());

	auto& vec = collector.GetCollectedAreas();

	for (auto area : vec)
	{
		if (area->HasTFAttributes(CTFNavArea::TFNAV_SNIPER_HINT) && !area->IsTFAttributesRestrictedForTeam(bot->GetMyTFTeam()))
		{
			hintAreas.push_back(area);
		}
	}

	if (!hintAreas.empty())
	{
		CTFNavArea* area = hintAreas[randomgen->GetRandomInt<size_t>(0, hintAreas.size() - 1)];
		out = area->GetCenter();
	}
	else
	{
		CTFNavArea* area = vec[randomgen->GetRandomInt<size_t>(0, vec.size() - 1)];
		out = area->GetCenter();
	}

	if (bot->IsDebugging(BOTDEBUG_TASKS))
	{
		bot->DebugPrintToConsole(BOTDEBUG_TASKS, 30, 160, 0, "%s: Selected sniper spot: %3.2f, %3.2f, %3.2f \n", bot->GetDebugIdentifier(), out.x, out.y, out.z);
	}
}
