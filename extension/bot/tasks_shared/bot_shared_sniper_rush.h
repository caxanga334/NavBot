#ifndef __NAVBOT_BOT_SHARED_SNIPER_RUSH_TASK_H_
#define __NAVBOT_BOT_SHARED_SNIPER_RUSH_TASK_H_

#include <bot/bot_shared_utils.h>
#include <bot/interfaces/path/meshnavigator.h>

// General purpose tasks for bots to aggressively rush and snipe people.
template <typename BotClass, typename PathCostClass>
class CBotSharedSniperRushTask : public AITask<BotClass>
{
public:
	CBotSharedSniperRushTask(const Vector& initialGoal, const int maxRandomMoves = 3) :
		m_moveGoal(initialGoal), m_numRandomMoves(0), m_maxRandomMoves(maxRandomMoves), m_keepScope(false)
	{
	}

	TaskResult<BotClass> OnTaskUpdate(BotClass* bot) override
	{
		const CKnownEntity* threat = bot->GetSensorInterface()->GetPrimaryKnownThreat(ISensor::ANY_THREATS);
		m_keepScope = false;

		if (threat != nullptr)
		{
			if (threat->IsVisibleNow() || threat->GetTimeSinceLastVisible() <= 5.0f)
			{
				m_keepScope = true;
			}
		}

		if (!m_keepScope)
		{
			if (m_nav.NeedsRepath())
			{
				PathCostClass cost(bot);

				if (!m_nav.ComputePathToPosition(bot, m_moveGoal, cost))
				{
					if (m_counter.Increase())
					{
						return AITask<BotClass>::Done("Cannot reach goal.");
					}
				}

				m_nav.StartRepathTimer();
			}

			m_nav.Update(bot);
		}

		return AITask<BotClass>::Continue();
	}

	TaskEventResponseResult<BotClass> OnStuck(BotClass* bot) override
	{
		if (m_counter.Increase())
		{
			return AITask<BotClass>::TryDone(PRIORITY_HIGH, "Too many move failures!.");
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
			return AITask<BotClass>::TryDone(PRIORITY_HIGH, "Too many move failures!.");
		}

		return AITask<BotClass>::TryToMaintain(PRIORITY_LOW);
	}

	TaskEventResponseResult<BotClass> OnMoveToSuccess(BotClass* bot, CPath* path) override
	{
		if (++m_numRandomMoves > m_maxRandomMoves)
		{
			return AITask<BotClass>::TryDone(PRIORITY_HIGH, "Reached maximum number of random moves.");
		}

		if (!GetRandomMoveGoal(bot))
		{
			return AITask<BotClass>::TryDone(PRIORITY_HIGH, "Failed to find a new move goal.");
		}

		m_counter.Reset();
		return AITask<BotClass>::TryToMaintain(PRIORITY_LOW);
	}

	QueryAnswerType ShouldAttack(CBaseBot* me, const CKnownEntity* them) override
	{
		return ANSWER_YES; // always attack
	}

	QueryAnswerType ShouldSwitchToWeapon(CBaseBot* me, const CBotWeapon* weapon) override
	{
		return ANSWER_YES;
	}

	QueryAnswerType ShouldScopeWithWeapon(CBaseBot* me, const CBotWeapon* activeWeapon, const bool wantstoscope, const bool isscoped) override
	{
		if (m_keepScope)
		{
			return ANSWER_YES;
		}

		return ANSWER_UNDEFINED;
	}

	const char* GetName() const override { return "SniperRush"; }
private:
	CMeshNavigator m_nav;
	CPathFailCounter m_counter;
	Vector m_moveGoal;
	int m_numRandomMoves;
	int m_maxRandomMoves;
	bool m_keepScope;

	bool GetRandomMoveGoal(BotClass* bot)
	{
		botsharedutils::RandomDestinationCollector collector(bot, 2048.0f);
		collector.Execute();

		if (collector.IsCollectedAreasEmpty())
		{
			return false;
		}

		CNavArea* area = collector.GetRandomCollectedArea();
		m_moveGoal = area->GetCenter();
		return true;
	}
};

#endif // !__NAVBOT_BOT_SHARED_SNIPER_RUSH_TASK_H_
