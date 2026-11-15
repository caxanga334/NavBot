#ifndef __NAVBOT_BOT_SHARED_FLANK_ENEMY_TASK_H_
#define __NAVBOT_BOT_SHARED_FLANK_ENEMY_TASK_H_

#include <bot/interfaces/path/meshnavigator.h>
#include <bot/bot_shared_utils.h>

/**
 * @brief General purpose task for flanking an enemy.
 * Moves towards an enemy while attempting to avoid their line of sight.
 * @tparam BotClass Bot class
 * @tparam PathCostClass Bot path cost class
 */
template <typename BotClass, typename PathCostClass>
class CBotSharedFlankEnemyTask : public AITask<BotClass>
{
public:
	/**
	 * @brief Determines if it possible to flank the enemy by checking if any flank paths exists.
	 * @param bot Bot that will be flanking the enemy.
	 * @param enemy Enemy entity.
	 * @return True if a flank path is found, false otherwise.
	 */
	static bool IsPossible(BotClass* bot, CBaseEntity* enemy)
	{
		Vector eyePos;
		QAngle eyeAngles;

		CBaseExtPlayer* player = extmanager->GetPlayerOfEntity(enemy);

		if (player != nullptr)
		{
			eyePos = player->GetEyeOrigin();
			eyeAngles = player->GetEyeAngles();
		}
		else
		{
			eyePos = UtilHelpers::getEntityEyePosition(enemy);
			// most entities uses abs angles as their eye angles, if accuracy is needed, switch this to a vcall of the actual eyeangles function.
			eyeAngles = UtilHelpers::getEntityAngles(enemy);
		}

		botsharedutils::PotentiallyVisibleCollector collector(eyePos, eyeAngles, 4096.0f, bot->GetSensorInterface()->GetEnemyFieldOfView());
		collector.Execute();
		auto& areas = collector.GetCollectedAreas();
		auto mover = bot->GetMovementInterface();
		auto startArea = collector.GetStartArea();
		
		for (auto area : areas)
		{
			if (area == startArea || startArea->HasAnyConnectionsTo(area))
			{
				// don't block the start area and areas adjacent to it
				continue;
			}

			mover->AddDeadArea(area, 0.5f);
		}

		PathCostClass cost(bot);
		CMeshNavigator nav;
		return nav.ComputePathToPosition(bot, UtilHelpers::getEntityOrigin(enemy), cost);
	}

	/**
	 * @brief Flanks an enemy while moving to a given world position.
	 * @param enemy Enemy the bot is flanking.
	 * @param goal Position the bot will move to.
	 * @param endRange How close to the goal position the bot should move to. (Ends the tasks when the range is equal or less to this).
	 * @param refreshAreas If true, periodically updates which areas are visible to the enemy (cheat).
	 * @param stealthMode If true, the bot won't attack any enemies unless they are spotted. (Ends the task early if any enemies sees the bot).
	 */
	CBotSharedFlankEnemyTask(CBaseEntity* enemy, const Vector& goal, float endRange, bool refreshAreas, bool stealthMode = false) :
		m_enemy(enemy), m_goal(goal), m_goalRange(endRange), m_refreshAreas(refreshAreas), m_moveToEnemy(false), m_stealthMode(stealthMode)
	{
	}

	/**
	 * @brief Flanks an enemy, moves directly to the enemy and always know where they are. (cheat)
	 * @param enemy Enemy the bot is flanking.
	 * @param endRange How close to the enemy the bot should move to. (Ends the tasks when the range is equal or less to this).
	 * @param stealthMode If true, the bot won't attack any enemies unless they are spotted. (Ends the task early if any enemies sees the bot).
	 */
	CBotSharedFlankEnemyTask(CBaseEntity* enemy, float endRange, bool stealthMode = false) :
		m_enemy(enemy), m_goal(0.0f, 0.0f, 0.0f), m_goalRange(endRange), m_refreshAreas(true), m_moveToEnemy(true), m_stealthMode(stealthMode)
	{ /* refresh areas always true in this version since the bot is already cheating by knowning where the enemy is. */
	}

	TaskResult<BotClass> OnTaskStart(BotClass* bot, AITask<BotClass>* pastTask) override
	{
		CBaseEntity* enemy = GetEnemy(bot);

		if (enemy == nullptr)
		{
			return AITask<BotClass>::Done("Enemy is NULL or invalid!");
		}

		m_enemyFOV = bot->GetSensorInterface()->GetEnemyFieldOfView();
		m_stealthModeTimer.Start(1.0f);
		UpdateAreas(bot, enemy);
		return AITask<BotClass>::Continue();
	}

	TaskResult<BotClass> OnTaskUpdate(BotClass* bot) override
	{
		CBaseEntity* enemy = GetEnemy(bot);

		if (enemy == nullptr)
		{
			return AITask<BotClass>::Done("Enemy is NULL or invalid!");
		}

		if (m_moveToEnemy)
		{
			m_goal = UtilHelpers::getEntityOrigin(enemy);
		}

		float rangeTo = (m_goal - bot->GetAbsOrigin()).Length();

		if (rangeTo <= m_goalRange)
		{
			return AITask<BotClass>::Done("Goal range reached!");
		}

		if (m_refreshAreas && m_updateAreasTimer.IsElapsed())
		{
			UpdateAreas(bot, enemy);
		}

		if (m_stealthMode && m_stealthModeTimer.IsElapsed())
		{
			m_stealthModeTimer.Start(0.5f);

			if (bot->GetSensorInterface()->IsVisibleToAnyEnemy(2.0f))
			{
				return AITask<BotClass>::Done("Enemy can see me!");
			}
		}

		if (m_nav.NeedsRepath())
		{
			PathCostClass cost(bot);

			if (m_stealthMode)
			{
				cost.SetRouteType(RouteType::SAFEST_ROUTE);
			}

			if (!m_nav.ComputePathToPosition(bot, m_goal, cost) && m_counter.Increase())
			{
				return AITask<BotClass>::Done("No path to goal!");
			}
		}

		m_nav.Update(bot);
		return AITask<BotClass>::Continue();
	}

	void OnTaskEnd(BotClass* bot, AITask<BotClass>* nextTask) override
	{
		if (!m_costAreas.empty())
		{
			auto mover = bot->GetMovementInterface();

			for (auto area : m_costAreas)
			{
				mover->RemoveCostModArea(area);
			}
		}
	}

	TaskEventResponseResult<BotClass> OnStuck(BotClass* bot) override
	{
		if (m_counter.Increase())
		{
			return AITask<BotClass>::TryDone(PRIORITY_HIGH, "Got stuck!");
		}

		return AITask<BotClass>::TryToMaintain(PRIORITY_LOW);
	}

	TaskEventResponseResult<BotClass> OnUnstuck(BotClass* bot) override
	{
		m_counter.Decrease();
		return AITask<BotClass>::TryToMaintain(PRIORITY_LOW);
	}

	TaskEventResponseResult<BotClass> OnMoveToFailure(BotClass* bot, CPath* path, IEventListener::MovementFailureType reason) override
	{
		if (m_counter.Increase())
		{
			return AITask<BotClass>::TryDone(PRIORITY_HIGH, "Move to goal failed!");
		}

		return AITask<BotClass>::TryToMaintain(PRIORITY_LOW);
	}

	TaskEventResponseResult<BotClass> OnMoveToSuccess(BotClass* bot, CPath* path) override
	{
		return AITask<BotClass>::TryDone(PRIORITY_HIGH, "Goal reached");
	}

	QueryAnswerType ShouldAttack(CBaseBot* me, const CKnownEntity* them) override { return m_stealthMode ? ANSWER_NO : ANSWER_YES; }
	QueryAnswerType ShouldHurry(CBaseBot* me) override { return ANSWER_YES; }
	QueryAnswerType ShouldAssistTeammate(CBaseBot* me, CBaseEntity* teammate) override { return ANSWER_NO; }
	QueryAnswerType ShouldSwitchToWeapon(CBaseBot* me, const CBotWeapon* weapon) override { return ANSWER_YES; }

	const char* GetName() const override { return "FlankEnemy"; }

private:
	CMeshNavigator m_nav;
	CPathFailCounter m_counter;
	CHandle<CBaseEntity> m_enemy;
	std::vector<CNavArea*> m_costAreas;
	Vector m_goal;
	float m_goalRange;
	float m_enemyFOV;
	CountdownTimer m_updateAreasTimer;
	CountdownTimer m_stealthModeTimer;
	bool m_refreshAreas;
	bool m_moveToEnemy;
	bool m_stealthMode;

	// returns NULL if the enemy is invalid.
	CBaseEntity* GetEnemy(BotClass* bot) const
	{
		CBaseEntity* enemy = m_enemy.Get();

		if (enemy == nullptr)
		{
			return nullptr;
		}

		if (bot->GetSensorInterface()->IsIgnored(enemy) || !bot->GetSensorInterface()->IsEnemy(enemy))
		{
			return nullptr;
		}

		return enemy;
	}

	void UpdateAreas(BotClass* bot, CBaseEntity* enemy)
	{
		m_updateAreasTimer.Start(3.0f);
		auto mover = bot->GetMovementInterface();

		if (!m_costAreas.empty())
		{
			for (auto area : m_costAreas)
			{
				mover->RemoveCostModArea(area);
			}
		}

		Vector eyePos;
		QAngle eyeAngles;

		CBaseExtPlayer* player = extmanager->GetPlayerOfEntity(enemy);

		if (player != nullptr)
		{
			eyePos = player->GetEyeOrigin();
			eyeAngles = player->GetEyeAngles();
		}
		else
		{
			eyePos = UtilHelpers::getEntityEyePosition(enemy);
			// most entities uses abs angles as their eye angles, if accuracy is needed, switch this to a vcall of the actual eyeangles function.
			eyeAngles = UtilHelpers::getEntityAngles(enemy);
		}

		botsharedutils::PotentiallyVisibleCollector collector(eyePos, eyeAngles, 4096.0f, m_enemyFOV);
		collector.Execute();
		collector.SwapCollectedAreas(m_costAreas);

		for (auto area : m_costAreas)
		{
			mover->AddCostModArea(area, 20.0f, 999999.0f);
		}

		m_nav.InvalidateAndRepath();
	}
};


#endif // !__NAVBOT_BOT_SHARED_FLANK_ENEMY_TASK_H_
