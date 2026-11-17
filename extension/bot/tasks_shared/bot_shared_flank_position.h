#ifndef __NAVBOT_BOT_SHARED_FLANK_POSITION_TASK_H_
#define __NAVBOT_BOT_SHARED_FLANK_POSITION_TASK_H_

#include <bot/interfaces/path/meshnavigator.h>
#include <bot/bot_shared_utils.h>

template <typename BotClass, typename PathCostClass>
class CBotSharedFlankPositionTask : public AITask<BotClass>
{
public:
	/**
	 * @brief Determines if the task is possible by checking if an alternative path exists
	 * @param bot Bot that will be doing this task.
	 * @param goal Position to flank.
	 * @param eyePos Observer eye position used to collect potentially visible areas.
	 * @param eyeDir Observer eye direction.
	 * @param maxViewDistance Maximum view distance for collecing areas.
	 * @param fov Observer field of view in degrees.
	 * @param visibleAreas Visible areas are stored here.
	 * @return True if a path exists, false otherwise.
	 */
	static bool IsPossible(BotClass* bot, const Vector& goal, const Vector& eyePos, 
		const Vector& eyeDir, const float maxViewDistance, const float fov, std::vector<CNavArea*>& visibleAreas)
	{
		botsharedutils::PotentiallyVisibleCollector collector(eyePos, eyeDir, maxViewDistance, fov);
		collector.Execute();
		auto mover = bot->GetMovementInterface();
		auto& areas = collector.GetCollectedAreas();
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
		bool result = nav.ComputePathToPosition(bot, goal, cost);

		for (auto area : areas)
		{
			mover->RemoveDeadArea(area);
		}

		collector.SwapCollectedAreas(visibleAreas);
		return result;
	}

	/**
	 * @brief Constructor that accepts a vector of areas to avoid. (IE: The one you get from IsPossible)
	 * @param goal Position the bot will move to.
	 * @param visibleAreas Vector of nav areas the pathfinding should avoid.
	 * @param stealthMode If true, the bot will not attack enemies, only when an enemy sees them. If false, the tasks ends when an enemy becomes visible.
	 */
	CBotSharedFlankPositionTask(const Vector& goal, std::vector<CNavArea*>&& visibleAreas, bool stealthMode = false) :
		m_goal(goal), m_costAreas(visibleAreas), m_stealthMode(stealthMode)
	{
	}

	/**
	 * @brief Constructor that builds a list of visible areas.
	 * @param goal Position the bot will move to.
	 * @param eyePos Observer eye position.
	 * @param eyeDir Observer eye direction.
	 * @param maxViewDistance Observer maximum view range.
	 * @param fov Observer FOV in degrees.
	 * @param stealthMode If true, the bot will not attack enemies, only when an enemy sees them. If false, the tasks ends when an enemy becomes visible.
	 */
	CBotSharedFlankPositionTask(const Vector& goal, const Vector& eyePos, const Vector& eyeDir, const float maxViewDistance, const float fov, bool stealthMode = false) :
		m_goal(goal), m_stealthMode(stealthMode)
	{
		botsharedutils::PotentiallyVisibleCollector collector(eyePos, eyeDir, maxViewDistance, fov);
		collector.Execute();
		collector.SwapCollectedAreas(m_costAreas);
	}

	TaskResult<BotClass> OnTaskStart(BotClass* bot, AITask<BotClass>* pastTask) override
	{
		auto mover = bot->GetMovementInterface();

		for (auto area : m_costAreas)
		{
			mover->AddCostModArea(area, 20.0f, 999999.0f);
		}

		return AITask<BotClass>::Continue();
	}

	TaskResult<BotClass> OnTaskUpdate(BotClass* bot) override
	{
		auto threat = bot->GetSensorInterface()->GetPrimaryKnownThreat(ISensor::ONLY_VISIBLE_THREATS);

		if (threat != nullptr)
		{
			if (!m_stealthMode)
			{
				return AITask<BotClass>::Done("Enemy visible!");
			}

			if (threat->CanTheySeeMe())
			{
				return AITask<BotClass>::Done("I've been spotted by an enemy!");
			}
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

	bool OnTaskPause(BotClass* bot, AITask<BotClass>* nextTask) override
	{
		RemoveCostModAreas(bot);
		return true;
	}

	TaskResult<BotClass> OnTaskResume(BotClass* bot, AITask<BotClass>* pastTask) override
	{
		AddCostModAreas(bot);
		return AITask<BotClass>::Continue();
	}

	void OnTaskEnd(BotClass* bot, AITask<BotClass>* nextTask) override
	{
		RemoveCostModAreas(bot);
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

	const char* GetName() const override { return "FlankPosition"; }

private:
	CMeshNavigator m_nav;
	CPathFailCounter m_counter;
	std::vector<CNavArea*> m_costAreas;
	Vector m_goal;
	CountdownTimer m_stealthModeTimer;
	bool m_stealthMode;

	// adds areas without updating the vector
	void AddCostModAreas(BotClass* bot) const
	{
		if (!m_costAreas.empty())
		{
			auto mover = bot->GetMovementInterface();

			for (auto area : m_costAreas)
			{
				mover->AddCostModArea(area, 20.0f, 999999.0f);
			}
		}
	}

	void RemoveCostModAreas(BotClass* bot) const
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
};

#endif // !__NAVBOT_BOT_SHARED_FLANK_POSITION_TASK_H_
