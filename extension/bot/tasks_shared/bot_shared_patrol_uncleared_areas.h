#ifndef __NAVBOT_BOT_SHARED_PATROL_UNCLEARED_AREAS_TASK_H_
#define __NAVBOT_BOT_SHARED_PATROL_UNCLEARED_AREAS_TASK_H_

#include <mods/basemod.h>
#include <bot/bot_shared_utils.h>
#include <navmesh/nav_area.h>
#include "bot_shared_clear_reported_enemy.h"

/**
 * @brief General purpose tasks for bots to patrol and clear areas of enemies.
 * Areas will be marked as cleared so bots know they don't need to patrol those areas for some time.
 * @tparam BT Bot class.
 * @tparam CT Bot path cost class.
 */
template <typename BT, typename CT>
class CBotSharedPatrolUnclearedAreasTask : public AITask<BT>
{
public:
	/**
	 * @brief Searches for a random uncleared area to patrol.
	 * @param[in] bot Bot that will be doing the task.
	 * @param[out] goalArea A randomly selected destination area.
	 * @return True if an area was found to patrol, false otherwise.
	 */
	static bool IsPossible(BT* bot, CNavArea** goalArea)
	{
		botsharedutils::SelectReachableUnclearedArea collector(bot);
		collector.Execute();
		
		if (collector.IsCollectedAreasEmpty())
		{
			return false;
		}

		// 50/50 for random area
		if (CBaseBot::s_botrng.GetRandomChance())
		{
			*goalArea = collector.GetRandomCollectedArea();
		}
		else
		{
			// 50/50 for farthest area
			if (CBaseBot::s_botrng.GetRandomChance())
			{
				*goalArea = collector.GetFarthestCollectedArea();
			}
			else
			{
				*goalArea = collector.GetNearestCollectedArea();
			}
		}
		
		return true;
	}
	/**
	 * @brief Searches for a random uncleared area to patrol.
	 * @tparam C NavAreaCollector, must derive from INavAreaCollector. botsharedutils::SelectReachableUnclearedArea is a good base for custom collectors.
	 * @param bot bot Bot that will be doing the task.
	 * @param collector Nav area collector. Note: No need to run Execute since this function calls it.
	 * @return True if areas were collected, false if the collector is empty.
	 */
	template<typename C>
	static bool IsPossible(BT* bot, C& collector)
	{
		collector.Execute();
		return !collector.IsCollectedAreasEmpty();
	}

	CBotSharedPatrolUnclearedAreasTask(CNavArea* goalArea)
	{
		m_goalArea = goalArea;
		m_clearTimeLimit = extmanager->GetMod()->GetModSettings()->GetMaxAreaClearedTime();
		m_pathFails = 0;
	}

	TaskResult<BT> OnTaskStart(BT* bot, AITask<BT>* pastTask) override
	{
		if (!m_goalArea)
		{
			return AITask<BT>::Done("NULL nav area!");
		}

		if (bot->IsDebugging(BOTDEBUG_TASKS))
		{
			bot->DebugPrintToConsole(255, 255, 0, "%s SELECTED PATROL AREA #%u <%s>\n", 
				bot->GetDebugIdentifier(), m_goalArea->GetID(), UtilHelpers::textformat::FormatVector(m_goalArea->GetCenter()));
			m_goalArea->DrawFilled(255, 255, 0, 127, 10.0f);
		}

		botsharedutils::search::MarkVisibleAreasAsCleared functor(bot);
		functor.Execute();
		m_tac.Enable(bot->GetCombatInterface());
		return AITask<BT>::Continue();
	}

	TaskResult<BT> OnTaskUpdate(BT* bot) override
	{
		if (m_goalArea->WasClearedWithinTime(bot->GetCurrentTeamIndex(), m_clearTimeLimit))
		{
			return AITask<BT>::Done("Area is cleared!");
		}

		if (m_checkReportsTimer.IsElapsed())
		{
			m_checkReportsTimer.Start(2.5f);

			CBaseEntity* target;

			// see if we have anything to investigate
			if (CBotSharedClearReportedEnemyTask<BT, CT>::IsPossible(bot, &target))
			{
				botsharedutils::search::MarkVisibleAreasAsCleared functor(bot);
				functor.Execute();
				return AITask<BT>::PauseFor(new CBotSharedClearReportedEnemyTask<BT, CT>(target), "Investigating reported enemy position!");
			}
		}

		if (m_nav.NeedsRepath())
		{
			CT cost(bot);
			cost.SetRouteType(RouteType::FASTEST_ROUTE);
			
			if (!m_nav.ComputePathToPosition(bot, m_goalArea->GetCenter(), cost))
			{
				if (++m_pathFails >= 20)
				{
					return AITask<BT>::Done("Failed to build a path to the goal nav area!");
				}
			}

			m_nav.StartRepathTimer();
		}

		m_nav.Update(bot);
		return AITask<BT>::Continue();
	}

	bool OnTaskPause(BT* bot, AITask<BT>* nextTask) override
	{
		m_tac.Disable();
		return true;
	}

	TaskResult<BT> OnTaskResume(BT* bot, AITask<BT>* pastTask) override
	{
		if (m_goalArea->WasClearedWithinTime(bot->GetCurrentTeamIndex(), m_clearTimeLimit))
		{
			return AITask<BT>::Done("Area is cleared!");
		}

		m_tac.Enable(bot->GetCombatInterface());
		return AITask<BT>::Continue();
	}

	TaskEventResponseResult<BT> OnNavAreaChanged(BT* bot, CNavArea* oldArea, CNavArea* newArea) override
	{
		// any area that I walk into should be marked as cleared
		if (newArea)
		{
			newArea->MarkAsCleared(bot->GetCurrentTeamIndex());
		}

		return AITask<BT>::TryToMaintain(PRIORITY_LOW);
	}

	TaskEventResponseResult<BT> OnStuck(BT* bot) override
	{
		if (++m_pathFails >= 20)
		{
			return AITask<BT>::TryDone(PRIORITY_HIGH, "Got stuck!");
		}

		m_nav.Invalidate(); // force a repath
		return AITask<BT>::TryToMaintain(PRIORITY_LOW);
	}

	TaskEventResponseResult<BT> OnMoveToFailure(BT* bot, CPath* path, IEventListener::MovementFailureType reason) override
	{
		if (++m_pathFails >= 20)
		{
			return AITask<BT>::TryDone(PRIORITY_HIGH, "Move failed!");
		}

		m_nav.Invalidate(); // force a repath
		return AITask<BT>::TryToMaintain(PRIORITY_LOW);
	}

	TaskEventResponseResult<BT> OnMoveToSuccess(BT* bot, CPath* path) override
	{
		m_goalArea->MarkAsCleared(bot->GetCurrentTeamIndex());
		return AITask<BT>::TryDone(PRIORITY_HIGH, "Goal reached!");
	}

	TaskEventResponseResult<BT> OnSight(BT* bot, CBaseEntity* subject) override
	{
		if (bot->GetSensorInterface()->IsEnemy(subject))
		{
			return AITask<BT>::TryDone(PRIORITY_HIGH, "Enemy spotted!");
		}

		return AITask<BT>::TryContinue();
	}

	const char* GetName() const override { return "PatrolAreas"; }
private:
	CMeshNavigator m_nav;
	CNavArea* m_goalArea;
	float m_clearTimeLimit;
	int m_pathFails;
	combatutils::ToggleAreaClearing m_tac;
	CountdownTimer m_checkReportsTimer;
};


#endif // !__NAVBOT_BOT_SHARED_PATROL_UNCLEARED_AREAS_TASK_H_

