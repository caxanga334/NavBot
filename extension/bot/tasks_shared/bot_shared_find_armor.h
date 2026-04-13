#ifndef __NAVBOT_BOT_SHARED_FIND_ARMOR_TASK_H_
#define __NAVBOT_BOT_SHARED_FIND_ARMOR_TASK_H_

#include <bot/basebot_pathcost.h>

template <typename BT, typename CT = CBaseBotPathCost>
class CBotSharedFindArmorTask : public AITask<BT>
{
public:
	/**
	 * @brief Determines if the find armor task is possible right now.
	 * @tparam F An entity search filter that derives from botsharedutils::search::SearchReachableEntities
	 * @param bot Bot that will be using this task.
	 * @param entity The nearest armor source to the bot will be stored here.
	 * @param filter Entity search filter.
	 * @param cost Optional float to store the travel cost.
	 * @return True if the task is possible, false otherwise.
	 */
	template <typename F>
	static bool IsPossible(BT* bot, CBaseEntity** entity, F& filter, float* cost = nullptr)
	{
		filter.DoSearch();

		if (filter.IsResultEmpty())
		{
			return false;
		}

		CBaseEntity* best = nullptr;
		float current = std::numeric_limits<float>::max();

		for (auto& result : filter.GetSearchResult())
		{
			if (result.second < current)
			{
				current = result.second;
				best = result.first;
			}
		}

		if (best)
		{
			*entity = best;

			if (cost)
			{
				*cost = current;
			}

			return true;
		}

		return false;
	}

	CBotSharedFindArmorTask(BT* bot, CBaseEntity* entity, const int maxarmor = 100) :
		m_pathcost(bot), m_entity(entity), m_maxarmor(maxarmor)
	{
		m_pathcost.SetRouteType(RouteType::SAFEST_ROUTE);
	}

	TaskResult<BT> OnTaskStart(BT* bot, AITask<BT>* pastTask) override
	{
		CBaseEntity* entity = m_entity.Get();

		if (!entity)
		{
			return AITask<BT>::Done("Armor source entity is NULL!");
		}

		entities::HBaseEntity be(entity);

		if (be.IsEffectActive(EF_NODRAW))
		{
			return AITask<BT>::Done("Armor source entity is invalid! (NODRAW)");
		}

		if (bot->GetArmorAmount() >= m_maxarmor)
		{
			return AITask<BT>::Done("I am at full armor!");
		}

		const Vector& goal = UtilHelpers::getEntityOrigin(entity);
		if (!m_nav.ComputePathToPosition(bot, goal, m_pathcost))
		{
			return AITask<BT>::Done("Failed to build a full path to the armor source entity!");
		}

		if (bot->IsDebugging(BOTDEBUG_TASKS))
		{
			bot->DebugPrintToConsole(255, 105, 180, "%s FIND ARMOR TASK STARTED! GOAL ENTITY %s AT %s.\n",
				bot->GetDebugIdentifier(), UtilHelpers::textformat::FormatEntity(entity), UtilHelpers::textformat::FormatVector(goal));
		}

		return AITask<BT>::Continue();
	}

	TaskResult<BT> OnTaskUpdate(BT* bot) override
	{
		CBaseEntity* entity = m_entity.Get();

		if (!entity)
		{
			return AITask<BT>::Done("Armor source entity is NULL!");
		}

		entities::HBaseEntity be(entity);

		if (be.IsEffectActive(EF_NODRAW))
		{
			return AITask<BT>::Done("Armor source entity is invalid! (NODRAW)");
		}

		if (bot->GetArmorAmount() >= m_maxarmor)
		{
			return AITask<BT>::Done("I am at full armor!");
		}

		if (!m_nav.IsValid() || m_nav.NeedsRepath())
		{
			const Vector& goal = UtilHelpers::getEntityOrigin(entity);
			m_nav.ComputePathToPosition(bot, goal, m_pathcost);
			m_nav.StartRepathTimer();
		}

		m_nav.Update(bot);
		return AITask<BT>::Continue();
	}

	TaskEventResponseResult<BT> OnStuck(BT* bot) override
	{
		CBaseEntity* entity = m_entity.Get();

		if (entity)
		{
			const Vector& goal = UtilHelpers::getEntityOrigin(entity);

			if (!m_nav.ComputePathToPosition(bot, goal, m_pathcost))
			{
				return AITask<BT>::TryDone(PRIORITY_LOW, "No path to item!");
			}
		}

		return AITask<BT>::TryContinue(PRIORITY_LOW);
	}

	TaskEventResponseResult<BT> OnMoveToFailure(BT* bot, CPath* path, IEventListener::MovementFailureType reason) override
	{
		CBaseEntity* entity = m_entity.Get();

		if (entity)
		{
			const Vector& goal = UtilHelpers::getEntityOrigin(entity);

			if (!m_nav.ComputePathToPosition(bot, goal, m_pathcost))
			{
				return AITask<BT>::TryDone(PRIORITY_LOW, "No path to item!");
			}
		}

		return AITask<BT>::TryContinue(PRIORITY_LOW);
	}

	TaskEventResponseResult<BT> OnMoveToSuccess(BT* bot, CPath* path) override
	{
		// assume we collected it
		return AITask<BT>::TryDone(PRIORITY_LOW, "Goal reached!");
	}

	QueryAnswerType ShouldHurry(CBaseBot* me) override { return ANSWER_YES; }

	const char* GetName() const override { return "FindArmor"; }

private:
	CMeshNavigator m_nav;
	CT m_pathcost;
	CHandle<CBaseEntity> m_entity;
	int m_maxarmor;
};

#endif // !__NAVBOT_BOT_SHARED_FIND_ARMOR_TASK_H_
