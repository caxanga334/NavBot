#include <extension.h>
#include <bot/tf2/tf2bot.h>
#include <mods/tf2/nav/tfnavarea.h>
#include <mods/tf2/nav/tfnav_waypoint.h>
#include <navmesh/nav_pathfind.h>
#include <mods/tf2/teamfortress2mod.h>
#include "tf2bot_task_sniper_snipe_area.h"
#include "tf2bot_task_sniper_move_to_sniper_spot.h"

class TF2SniperSpotAreaCollector : public INavAreaCollector<CTFNavArea>
{
public:
	TF2SniperSpotAreaCollector(CTF2Bot* me, CTFNavArea* startArea) :
		INavAreaCollector<CTFNavArea>(startArea, 8000.0f)
	{
		m_me = me;
	}

	bool ShouldCollect(CTFNavArea* area) override;
private:
	CTF2Bot* m_me;
};

bool TF2SniperSpotAreaCollector::ShouldCollect(CTFNavArea* area)
{
	if (area->GetSizeX() < 16.0f || area->GetSizeY() < 16.0f)
	{
		// don't snipe in small areas
		return false;
	}

	if (area->HasTFPathAttributes(CTFNavArea::TFNavPathAttributes::TFNAV_PATH_DYNAMIC_SPAWNROOM))
	{
		// don't snipe inside spawnrooms
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

	if (bot->GetRangeTo(m_goal) <= 16.0f)
	{
		// reached sniping goal
		m_sniping = true;
		return PauseFor(new CTF2BotSniperSnipeAreaTask(m_waypoint), "Sniping!");
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

	GetRandomSnipingSpot(bot);
}

void CTF2BotSniperMoveToSnipingSpotTask::GetRandomSnipingSpot(CTF2Bot* bot)
{
	std::vector<CTFWaypoint*> spots;
	auto& thewaypoints = CTeamFortress2Mod::GetTF2Mod()->GetAllSniperWaypoints();

	for (auto waypoint : thewaypoints)
	{
		if (waypoint->IsEnabled() && waypoint->IsAvailableToTeam(static_cast<int>(bot->GetMyTFTeam())) && waypoint->CanBeUsedByBot(bot))
		{
			spots.push_back(waypoint);
		}
	}

	if (spots.empty())
	{
		m_goal = GetRandomSnipingPosition(bot);
		m_waypoint = nullptr;
		return;
	}

	CTFWaypoint* goal = librandom::utils::GetRandomElementFromVector<CTFWaypoint*>(spots);
	goal->Use(bot); // prevent other bots from selecting
	m_waypoint = goal;
	m_goal = goal->GetRandomPoint();
}

Vector CTF2BotSniperMoveToSnipingSpotTask::GetRandomSnipingPosition(CTF2Bot* bot)
{
	Vector startPos = GetSnipingSearchStartPosition(bot);
	CNavArea* start = TheNavMesh->GetNearestNavArea(startPos, 256.0f);

	if (start == nullptr)
	{
		start = bot->GetLastKnownNavArea();

		if (start == nullptr)
		{
			return bot->GetAbsOrigin();
		}
	}

	TF2SniperSpotAreaCollector collector(bot, static_cast<CTFNavArea*>(start));
	collector.Execute();

	if (collector.IsCollectedAreasEmpty())
	{
		return bot->GetAbsOrigin();
	}

	CTFNavArea* randomArea = collector.GetRandomCollectedArea();
	return randomArea->GetRandomPoint();
}

Vector CTF2BotSniperMoveToSnipingSpotTask::GetSnipingSearchStartPosition(CTF2Bot* bot)
{
	return bot->GetAbsOrigin();
}
