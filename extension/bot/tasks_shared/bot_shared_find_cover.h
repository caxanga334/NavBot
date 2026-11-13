#ifndef __NAVBOT_BOT_SHARED_FIND_COVER_TASK_H_
#define __NAVBOT_BOT_SHARED_FIND_COVER_TASK_H_

#include <navmesh/nav_mesh.h>
#include <bot/bot_shared_utils.h>
#include <bot/interfaces/path/meshnavigator.h>

// Searches for cover and moves to it
template <typename BotClass, typename PathCostClass>
class CBotSharedFindCoverTask : public AITask<BotClass>
{
public:
	/**
	 * @brief Determines if cover is available.
	 * @param bot Bot taking cover.
	 * @param fromPos Optional position to take cover from. if NULL, finds cover from the primary threat.
	 * @return Cover nav area. NULL if no cover is available.
	 */
	static CNavArea* IsPossible(BotClass* bot, const Vector* fromPos)
	{
		if (fromPos != nullptr)
		{
			botsharedutils::SearchForCover<CNavArea, CBaseBot> search(bot, *fromPos);
			return search.FindCover();
		}

		botsharedutils::SearchForCover<CNavArea, CBaseBot> search(bot);
		return search.FindCover();
	}

	CBotSharedFindCoverTask() :
		m_goal(0.0f, 0.0f, 0.0f)
	{
	}

	CBotSharedFindCoverTask(CNavArea* coverArea)
	{
		m_goal = coverArea->GetCenter();
	}

	TaskResult<BotClass> OnTaskUpdate(BotClass* bot) override
	{
		if (m_nav.NeedsRepath())
		{
			PathCostClass cost(bot);

			if (!m_nav.ComputePathToPosition(bot, m_goal, cost))
			{
				if (m_counter.Increase())
				{
					return AITask<BotClass>::Done("No path to goal!");
				}
			}
		}

		m_nav.Update(bot);
		return AITask<BotClass>::Continue();
	}

	TaskEventResponseResult<BotClass> OnInjured(BotClass* bot, const CTakeDamageInfo& info) override
	{
		// FindCover may be a result of an OnInjured event.
		// Try to stop the event from propagating
		return AITask<BotClass>::TryToMaintain(PRIORITY_LOW);
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
		return AITask<BotClass>::TryToMaintain(PRIORITY_HIGH);
	}

	TaskEventResponseResult<BotClass> OnMoveToFailure(BotClass* bot, CPath* path, IEventListener::MovementFailureType reason) override
	{
		if (m_counter.Increase())
		{
			return AITask<BotClass>::TryDone(PRIORITY_HIGH, "Movement failed!");
		}

		return AITask<BotClass>::TryToMaintain(PRIORITY_LOW);
	}

	TaskEventResponseResult<BotClass> OnMoveToSuccess(BotClass* bot, CPath* path) override
	{
		return AITask<BotClass>::TryDone(PRIORITY_HIGH, "Cover reached!");
	}

	QueryAnswerType ShouldAttack(CBaseBot* me, const CKnownEntity* them) override { return ANSWER_YES; }
	QueryAnswerType ShouldSeekAndDestroy(CBaseBot* me, const CKnownEntity* them) override { return ANSWER_NO; }
	QueryAnswerType ShouldHurry(CBaseBot* me) override { return ANSWER_YES; }
	QueryAnswerType ShouldRetreat(CBaseBot* me) override { return ANSWER_NO; }
	QueryAnswerType ShouldAssistTeammate(CBaseBot* me, CBaseEntity* teammate) override { return ANSWER_NO; }
	QueryAnswerType ShouldSwitchToWeapon(CBaseBot* me, const CBotWeapon* weapon) override { return ANSWER_YES; }

	const char* GetName() const override { return "FindCover"; }

protected:
	void SetMoveGoal(const Vector& goal) { m_goal = goal; }

private:
	Vector m_goal;
	CMeshNavigator m_nav;
	CPathFailCounter m_counter;
};


template <typename BotClass, typename PathCostClass>
class CBotSharedCoverReloadTask : public CBotSharedFindCoverTask<BotClass, PathCostClass>
{
public:
	CBotSharedCoverReloadTask() :
		m_noCoverFound(false), CBotSharedFindCoverTask<BotClass, PathCostClass>()
	{
	}

	TaskResult<BotClass> OnTaskStart(BotClass* bot, AITask<BotClass>* pastTask) override
	{
		botsharedutils::SearchForCover<CNavArea, CBaseBot> search(bot);
		CNavArea* cover = search.FindCover();

		if (cover != nullptr)
		{
			CBotSharedFindCoverTask<BotClass, PathCostClass>::SetMoveGoal(cover->GetCenter());
		}
		else
		{
			m_noCoverFound = true;
		}

		bot->GetControlInterface()->PressReloadButton();
		return CBotSharedFindCoverTask<BotClass, PathCostClass>::OnTaskStart(bot, pastTask);
	}

	TaskResult<BotClass> OnTaskUpdate(BotClass* bot) override
	{
		const CBotWeapon* weapon = bot->GetInventoryInterface()->GetActiveBotWeapon();

		if (weapon == nullptr)
		{
			return AITask<BotClass>::Done("NULL weapon!");
		}

		if (weapon->IsLoaded() && !m_waitTimer.HasStarted())
		{ 
			// Currently, bots don't know if the weapon reloads the entire clip at once (IE: TF2 SMG) or one by one (IE: TF2 shotgun)
			// So we add a small wait time
			m_waitTimer.Start(2.0f);
		}

		if (m_waitTimer.HasStarted() && m_waitTimer.IsElapsed())
		{
			return AITask<BotClass>::Done("Done reloading!");
		}

		if (!m_noCoverFound)
		{
			// call base to move the bot
			return CBotSharedFindCoverTask<BotClass, PathCostClass>::OnTaskUpdate(bot);
		}

		return AITask<BotClass>::Continue();
	}

	const char* GetName() const override { return "FindCoverAndReload"; }
private:
	bool m_noCoverFound;
	CountdownTimer m_waitTimer;
};

template <typename BotClass, typename PathCostClass>
class CBotSharedFindCoverAndWaitTask : public CBotSharedFindCoverTask<BotClass, PathCostClass>
{
public:
	/**
	 * @brief Constructor
	 * @param coverArea Nav area to take cover at.
	 * @param waitTime Amount of time in seconds to wait in cover.
	 */
	CBotSharedFindCoverAndWaitTask(CNavArea* coverArea, const float waitTime) :
		CBotSharedFindCoverTask<BotClass, PathCostClass>(coverArea), m_wait(waitTime)
	{
	}

	TaskResult<BotClass> OnTaskStart(BotClass* bot, AITask<BotClass>* pastTask) override
	{
		return CBotSharedFindCoverTask<BotClass, PathCostClass>::OnTaskStart(bot, pastTask);
	}

	TaskResult<BotClass> OnTaskUpdate(BotClass* bot) override
	{
		if (m_timer.HasStarted())
		{
			if (m_timer.IsElapsed())
			{
				return AITask<BotClass>::Done("Wait timer elapsed!");
			}

			return AITask<BotClass>::Continue();
		}

		return CBotSharedFindCoverTask<BotClass, PathCostClass>::OnTaskUpdate(bot);
	}

	TaskEventResponseResult<BotClass> OnMoveToSuccess(BotClass* bot, CPath* path) override
	{
		if (!m_timer.HasStarted())
		{
			m_timer.Start(m_wait);
		}

		return AITask<BotClass>::TryToMaintain(PRIORITY_CRITICAL);
	}

	const char* GetName() const override { return "FindCoverAndWait"; }
private:
	float m_wait;
	CountdownTimer m_timer;
};

#endif // __NAVBOT_BOT_SHARED_FIND_COVER_TASK_H_