#ifndef __NAVBOT_BOT_SHARED_INVESTIGATE_KNOWN_ENTITY_TASK_H_
#define __NAVBOT_BOT_SHARED_INVESTIGATE_KNOWN_ENTITY_TASK_H_

/**
 * @brief Task for investigating known entities not yet seen by the bot.
 * @tparam BT Bot class
 * @tparam CT Bot path cost class
 */
template <typename BT, typename CT>
class CBotSharedInvestigateKnownTask : public AITask<BT>
{
public:
	/**
	 * @brief Determines if this task is possible.
	 * @param bot Bot that will be running this task.
	 * @param target Target entity to investigate if found.
	 * @return True if an entity to investigate was found, false otherwise.
	 */
	static bool IsPossible(BT* bot, CBaseEntity** target)
	{
		const CKnownEntity* known = bot->GetSensorInterface()->GetNearestUnseenKnownEntity();

		if (!known)
		{
			return false;
		}

		*target = known->GetEntity();
		return true;
	}

	CBotSharedInvestigateKnownTask(CBaseEntity* target) :
		m_target(target), m_goal(0.0f, 0.0f, 0.0f)
	{
	}

	TaskResult<BT> OnTaskStart(BT* bot, AITask<BT>* pastTask) override
	{
		CBaseEntity* target = m_target.Get();

		if (!target)
		{
			return AITask<BT>::Done("Investigate entity is NULL!");
		}

		const CKnownEntity* known = bot->GetSensorInterface()->GetKnown(target);

		if (!known)
		{
			return AITask<BT>::Done("Target is no longer known by the bot!");
		}

		m_goal = known->GetLastKnownPosition();
		CT cost(bot);
		if (!m_nav.ComputePathToPosition(bot, m_goal, cost))
		{
			// forget about it so we can move onto other targets
			bot->GetSensorInterface()->ForgetKnownEntity(target);
			bot->GetSharedMemoryInterface()->ForgetEntity(target);
			return AITask<BT>::Done("Failed to build a path to the target's last known position.");
		}

		// mark areas as cleared while we search
		m_tac.Enable(bot->GetCombatInterface());
		m_nav.StartRepathTimer();
		return AITask<BT>::Continue();
	}

	TaskResult<BT> OnTaskUpdate(BT* bot) override
	{
		CBaseEntity* target = m_target.Get();

		if (!target)
		{
			return AITask<BT>::Done("Investigate entity is NULL!");
		}

		const CKnownEntity* known = bot->GetSensorInterface()->GetKnown(target);

		if (!known)
		{
			return AITask<BT>::Done("Target is no longer known by the bot!");
		}

		// see if we have any updated info on this target
		if ((known->GetLastKnownPosition() - m_goal).IsLengthGreaterThan(16.0f))
		{
			m_goal = known->GetLastKnownPosition();
			m_nav.InvalidateAndRepath();
		}

		if (m_nav.NeedsRepath())
		{
			CT cost(bot);
			cost.SetRouteType(RouteType::FASTEST_ROUTE);
			m_nav.ComputePathToPosition(bot, m_goal, cost);
			m_nav.StartRepathTimer();
		}

		m_nav.Update(bot);
		return AITask<BT>::Continue();
	}

	void OnTaskEnd(BT* bot, AITask<BT>* nextTask) override
	{
		m_tac.Disable();
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

	TaskEventResponseResult<BT> OnMoveToSuccess(BT* bot, CPath* path) override
	{
		CBaseEntity* target = m_target.Get();

		if (target)
		{
			bot->GetSensorInterface()->MarkEntityLKPAsInvestigated(target);
		}

		return AITask<BT>::TryDone(PRIORITY_HIGH, "Goal reached");
	}

	TaskEventResponseResult<BT> OnSight(BT* bot, CBaseEntity* subject) override
	{
		if (bot->GetSensorInterface()->IsEnemy(subject))
		{
			return AITask<BT>::TryDone(PRIORITY_HIGH, "Enemy spotted!");
		}

		return AITask<BT>::TryContinue();
	}

	const char* GetName() const override { return "InvestigateKnownLKP"; }
private:
	CHandle<CBaseEntity> m_target;
	CMeshNavigator m_nav;
	Vector m_goal;
	combatutils::ToggleAreaClearing m_tac;
};


#endif // !__NAVBOT_BOT_SHARED_INVESTIGATE_KNOWN_ENTITY_TASK_H_