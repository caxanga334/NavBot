#include <extension.h>
#include <util/entprops.h>
#include <sdkports/sdk_traces.h>
#include <mods/tf2/tf2lib.h>
#include <mods/tf2/teamfortress2mod.h>
#include <mods/tf2/nav/tfnavmesh.h>
#include <mods/tf2/nav/tfnavarea.h>
#include <navmesh/nav_pathfind.h>
#include <bot/tf2/tf2bot.h>
#include <bot/interfaces/path/meshnavigator.h>
#include <entities/tf2/tf_entities.h>
#include <bot/tf2/tasks/tf2bot_find_health_task.h>
#include "tf2bot_mvm_guard_dropped_bomb_task.h"

class CTF2BotSelectMvMBombGuardArea : public INavAreaCollector<CTFNavArea>
{
public:
	CTF2BotSelectMvMBombGuardArea(CTFNavArea* startArea, const Vector& bomb) :
		INavAreaCollector<CTFNavArea>(startArea, 1024.0f)
	{
		m_bombPos = bomb;
	}

	bool ShouldCollect(CTFNavArea* area) override;

private:
	Vector m_bombPos;
};

bool CTF2BotSelectMvMBombGuardArea::ShouldCollect(CTFNavArea* area)
{
	if (area->HasTFPathAttributes(CTFNavArea::TFNAV_PATH_DYNAMIC_SPAWNROOM))
	{
		return false;
	}
	else if (area->GetSizeX() < 24.0f || area->GetSizeY() < 24.0f)
	{
		return false;
	}

	Vector endpos = area->GetCenter();
	endpos.z += 38.0f;

	trace_t tr;
	trace::line(m_bombPos, endpos, MASK_VISIBLE, tr);

	if (!tr.DidHit())
	{
		return true;
	}

	return false;
}

CTF2BotMvMGuardDroppedBombTask::CTF2BotMvMGuardDroppedBombTask(CBaseEntity* bomb) :
	m_bomb(bomb), m_goal(0.0f, 0.0f, 0.0f), m_reached(false)
{
	m_scanTimer.Start(3.0f);
}

TaskResult<CTF2Bot> CTF2BotMvMGuardDroppedBombTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	CBaseEntity* bomb = m_bomb.Get();

	if (!bomb)
	{
		return Done("Bomb is gone!");
	}

	const Vector& pos = UtilHelpers::getEntityOrigin(bomb);
	CNavArea* area = TheNavMesh->GetNearestNavArea(pos, 512.0f);

	if (!area)
	{
		return Done("Failed to get nav area nearest to the bomb!");
	}

	CTF2BotSelectMvMBombGuardArea collector(static_cast<CTFNavArea*>(area), pos);
	collector.Execute();

	if (collector.IsCollectedAreasEmpty())
	{
		return Done("Failed to find a suitable nav area to guard the dropped bomb from!");
	}

	CTFNavArea* goal = collector.GetRandomCollectedArea();
	m_goal = goal->GetRandomPoint();
	CTF2BotPathCost cost(bot);
	m_nav.ComputePathToPosition(bot, m_goal, cost);
	m_repathTimer.Start(2.0f);

	CBaseEntity* health = nullptr;

	if (bot->GetHealthPercentage() < 0.9f && CTF2BotFindHealthTask::IsPossible(bot, &health))
	{
		return PauseFor(new CTF2BotFindHealthTask(health), "Healing up first!");
	}

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotMvMGuardDroppedBombTask::OnTaskUpdate(CTF2Bot* bot)
{
	CBaseEntity* bomb = m_bomb.Get();

	if (!bomb)
	{
		return Done("Bomb is gone!");
	}

	CBaseEntity* priority_bomb = tf2lib::mvm::GetMostDangerousFlag(true);

	if (priority_bomb && priority_bomb != bomb)
	{
		return Done("Abandoning post to defend against another bomb!");
	}

	CBaseEntity* tank = tf2lib::mvm::GetMostDangerousTank();

	if (tank)
	{
		return Done("Going to defend against tank!");
	}

	tfentities::HCaptureFlag flag(bomb);

	if (flag.IsStolen())
	{
		return Done("Bomb was picked up, moving to defend!");
	}

	if (!m_reached)
	{
		if (m_repathTimer.IsElapsed())
		{
			m_repathTimer.Start(2.0f);
			CTF2BotPathCost cost(bot);
			m_nav.ComputePathToPosition(bot, m_goal, cost);
		}

		m_nav.Update(bot);
	}
	else
	{
		if (m_scanTimer.IsElapsed())
		{
			m_scanTimer.Start(1.5f);

			float nearest = 999999.0f;
			const float maxRange = bot->GetSensorInterface()->GetMaxHearingRange();
			bool found = false;
			Vector lookAt;

			UtilHelpers::ForEachPlayer([&bot, &nearest, &lookAt, &found, &maxRange](int client, edict_t* entity, SourceMod::IGamePlayer* player) {
				if (tf2lib::GetEntityTFTeam(client) == TeamFortress2::TFTeam_Blue && UtilHelpers::IsPlayerAlive(client))
				{
					float range = bot->GetRangeTo(entity);

					if (range <= maxRange)
					{
						if (range < nearest)
						{
							found = true;
							nearest = range;
							lookAt = UtilHelpers::getEntityOrigin(entity);
							lookAt.z += 64.0f;
						}
					}
				}
			});

			if (found)
			{
				bot->GetControlInterface()->AimAt(lookAt, IPlayerController::LOOK_ALERT, 1.0f, "Looking at audible MvM robot!");
			}
		}
	}

	return Continue();
}

TaskEventResponseResult<CTF2Bot> CTF2BotMvMGuardDroppedBombTask::OnMoveToSuccess(CTF2Bot* bot, CPath* path)
{
	if (!IsPaused())
	{
		m_reached = true;
	}

	return TryContinue();
}
