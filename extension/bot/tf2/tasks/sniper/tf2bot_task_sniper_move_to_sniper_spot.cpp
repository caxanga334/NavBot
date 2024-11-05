#include <extension.h>
#include <bot/tf2/tf2bot.h>
#include <mods/tf2/nav/tfnavarea.h>
#include <mods/tf2/nav/tfnav_waypoint.h>
#include <navmesh/nav_pathfind.h>
#include <mods/tf2/teamfortress2mod.h>
#include "tf2bot_task_sniper_snipe_area.h"
#include "tf2bot_task_sniper_move_to_sniper_spot.h"

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

	GetRandomSnipingSpot(bot, m_goal);
}

void CTF2BotSniperMoveToSnipingSpotTask::GetRandomSnipingSpot(CTF2Bot* bot, Vector& out)
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
		bot->DebugPrintToConsole(BOTDEBUG_ERRORS, 255, 0, 0, "Bot %s failed to find a valid Sniper waypoint to snipe from!", bot->GetDebugIdentifier());
		m_goal = bot->GetAbsOrigin();
		m_waypoint = nullptr;
		return;
	}

	CTFWaypoint* goal = librandom::utils::GetRandomElementFromVector<CTFWaypoint*>(spots);
	goal->Use(bot); // prevent other bots from selecting
	m_waypoint = goal;
	m_goal = goal->GetRandomPoint();
}
