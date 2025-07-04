#include NAVBOT_PCH_FILE
#include <extension.h>
#include <util/helpers.h>
#include <util/entprops.h>
#include <util/librandom.h>
#include <mods/tf2/nav/tfnavmesh.h>
#include <mods/tf2/nav/tfnavarea.h>
#include <mods/tf2/tf2lib.h>
#include <bot/bot_shared_utils.h>
#include <bot/tf2/tf2bot.h>
#include <bot/interfaces/path/meshnavigator.h>
#include <bot/tf2/tasks/tf2bot_attack.h>
#include "tf2bot_pd_move_to_capture_zone_task.h"
#include "tf2bot_pd_collect_task.h"
#include "tf2bot_pd_search_and_destroy_task.h"


TaskResult<CTF2Bot> CTF2BotPDSearchAndDestroyTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	if (!SelectRandomGoal(bot))
	{
		return Done("Failed to find a random goal!");
	}

	m_checkdelivertimer.Start(5.0f);
	m_checkpointstimer.Start(10.0f);

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotPDSearchAndDestroyTask::OnTaskUpdate(CTF2Bot* bot)
{
	if (m_checkdelivertimer.IsElapsed())
	{
		m_checkdelivertimer.StartRandom(1.5f, 4.0f);

		CBaseEntity* capzone = nullptr;

		if (CTF2BotPDMoveToCaptureZoneTask::IsPossible(bot, &capzone))
		{
			return SwitchTo(new CTF2BotPDMoveToCaptureZoneTask(capzone), "Delivering my points!");
		}
	}

	if (m_checkpointstimer.IsElapsed())
	{
		m_checkpointstimer.StartRandom(2.0f, 4.0f);

		std::vector<CHandle<CBaseEntity>> pointsToCollect;

		if (CTF2BotPDCollectTask::IsPossible(bot, pointsToCollect))
		{
			return SwitchTo(new CTF2BotPDCollectTask(std::move(pointsToCollect)), "Collecting nearby dropped points!");
		}
	}

	// Very aggressive bots go after enemies they encounter
	if ((bot->GetDifficultyProfile()->GetAggressiveness() - 75) > 0)
	{
		const CKnownEntity* threat = bot->GetSensorInterface()->GetPrimaryKnownThreat(true);

		if (threat)
		{
			return PauseFor(new CTF2BotAttackTask(threat->GetEntity()), "Attacking visible threat!");
		}
	}

	if (m_repathtimer.IsElapsed())
	{
		m_repathtimer.Start(2.0f);
		CTF2BotPathCost cost(bot);
		m_nav.ComputePathToPosition(bot, m_goal, cost);
	}

	m_nav.Update(bot);

	return Continue();
}

TaskEventResponseResult<CTF2Bot> CTF2BotPDSearchAndDestroyTask::OnMoveToSuccess(CTF2Bot* bot, CPath* path)
{
	return TryDone(PRIORITY_LOW, "Goal reached");
}

bool CTF2BotPDSearchAndDestroyTask::SelectRandomGoal(CTF2Bot* bot)
{
	if (randomgen->GetRandomInt<int>(1, 10) <= 3)
	{
		CTFNavArea* area = TheTFNavMesh()->GetRandomSpawnRoomExitArea(static_cast<int>(tf2lib::GetEnemyTFTeam(bot->GetMyTFTeam())));

		if (area)
		{
			m_goal = area->GetCenter();
			return true;
		}
	}

	// aggressive bots will seek the enemy's team leader
	const int aggression = bot->GetDifficultyProfile()->GetAggressiveness() - 40;
	if (aggression > 0 && randomgen->GetRandomInt<int>(1, 100) <= aggression)
	{
		CBaseEntity* leader = tf2lib::pd::GetTeamLeader(tf2lib::GetEnemyTFTeam(bot->GetMyTFTeam()));

		if (leader)
		{
			const Vector& origin = UtilHelpers::getEntityOrigin(leader);
			CNavArea* area = TheNavMesh->GetNearestNavArea(origin, 256.0f);

			// very basic reachable check
			if (area && bot->GetMovementInterface()->IsAreaTraversable(area))
			{
				m_goal = origin;
				return true;
			}
		}
	}

	botsharedutils::RandomDestinationCollector collector(bot);
	collector.Execute();

	if (!collector.IsCollectedAreasEmpty())
	{
		CNavArea* area = collector.GetRandomCollectedArea();
		m_goal = area->GetCenter();
		return true;
	}

	return false;
}
