#ifndef __NAVBOT_BOT_SHARED_BREAK_ENTITY_TASK_H_
#define __NAVBOT_BOT_SHARED_BREAK_ENTITY_TASK_H_

#include <mods/basemod.h>

/**
 * @brief General purpose task for breaking entities.
 * This task moves the bot close to the entity. Breaking is handled by the movement interface 'BreakObstacle' action.
 * 
 * @tparam BotClass Bot class.
 * @tparam PathCostClass Path cost class.
 */
template <typename BotClass, typename PathCostClass>
class CBotSharedBreakEntityTask : public AITask<BotClass>
{
public:
	CBotSharedBreakEntityTask(CBaseEntity* entity) :
		m_target(entity)
	{
	}

	TaskResult<BotClass> OnTaskStart(BotClass* bot, AITask<BotClass>* pastTask) override
	{
		CBaseEntity* entity = m_target.Get();

		if (!entity)
		{
			return AITask<BotClass>::Done("NULL target entity!");
		}

		constexpr auto maxhealth = std::numeric_limits<int>::max() - 1;

		if (!modhelpers->IsEntityDamageable(entity, maxhealth))
		{
			return AITask<BotClass>::Done("Entity doesn't take damage!");
		}

		if (!modhelpers->IsEntityDamageableBy(entity, bot->GetEntity()))
		{
			return AITask<BotClass>::Done("Bot can't damage the target entity!");
		}

		Vector center = UtilHelpers::getWorldSpaceCenter(entity);
		Vector pos = trace::getground(center);
		PathCostClass cost(bot);

		if (!m_nav.ComputePathToPosition(bot, pos, cost))
		{
			if (bot->IsDebugging(BOTDEBUG_TASKS))
			{
				bot->DebugPrintToConsole(255, 0, 0, "%s BreakEntityTask: No complete path to entity %s at <%s>! \n",
					bot->GetDebugIdentifier(), UtilHelpers::textformat::FormatEntity(entity), UtilHelpers::textformat::FormatVector(center));
			}
		}

		m_nav.StartRepathTimer();
		return AITask<BotClass>::Continue();
	}

	TaskResult<BotClass> OnTaskUpdate(BotClass* bot) override
	{
		if (bot->GetMovementInterface()->IsBreakingObstacle())
		{
			return AITask<BotClass>::Continue();
		}

		CBaseEntity* entity = m_target.Get();

		if (!entity)
		{
			return AITask<BotClass>::Done("NULL target entity!");
		}

		if (modhelpers->IsBreakableBroken(entity))
		{
			return AITask<BotClass>::Done("Entity is broken!");
		}

		Vector center = UtilHelpers::getWorldSpaceCenter(entity);
		Vector pos = trace::getground(center);
		CBaseEntity* obstacle = nullptr;

		bool traversable = bot->GetMovementInterface()->IsPotentiallyTraversable(bot->GetAbsOrigin(), pos, nullptr, true, &obstacle);

		if (traversable || obstacle == entity)
		{
			bot->GetMovementInterface()->BreakObstacle(entity);
			return AITask<BotClass>::Continue();
		}

		if (bot->GetRangeTo(center) <= 150.0f)
		{
			bot->GetMovementInterface()->BreakObstacle(entity);
			return AITask<BotClass>::Continue();
		}

		if (m_nav.NeedsRepath())
		{
			PathCostClass cost(bot);

			m_nav.ComputePathToPosition(bot, pos, cost);
			m_nav.StartRepathTimer();
		}

		m_nav.Update(bot);
		return AITask<BotClass>::Continue();
	}

	const char* GetName() const override { return "BreakEntity"; }

private:
	CMeshNavigator m_nav;
	CHandle<CBaseEntity> m_target;
};

#endif // !__NAVBOT_BOT_SHARED_BREAK_ENTITY_TASK_H_
