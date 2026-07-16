#ifndef __NAVBOT_BOT_SHARED_USE_ENTITY_TASK_H_
#define __NAVBOT_BOT_SHARED_USE_ENTITY_TASK_H_

#include <mods/basemod.h>

template <typename BotClass, typename PathCostClass>
class CBotSharedUseEntityTask : public AITask<BotClass>
{
public:
	/**
	 * @brief Constructor.
	 * @param entity Entity the bot will use.
	 */
	CBotSharedUseEntityTask(CBaseEntity* entity) :
		m_entity(entity)
	{
		m_rangeToEntity = std::numeric_limits<float>::max();
	}

	/**
	 * @brief Constructor.
	 * @tparam Func Validator function bool (BotClass* bot, CBaseEntity* entity)
	 * @param entity Entity the bot will use.
	 * @param fun Validator function.
	 */
	template <typename Func>
	CBotSharedUseEntityTask(CBaseEntity* entity, Func fun) :
		m_entity(entity)
	{
		m_rangeToEntity = std::numeric_limits<float>::max();
		m_validator = fun;
	}

	TaskResult<BotClass> OnTaskUpdate(BotClass* bot) override
	{
		if (m_timeout.HasStarted() && m_timeout.IsElapsed())
		{
			return AITask<BotClass>::Done("Task timed out!");
		}

		CBaseEntity* entity = m_entity.Get();

		if (bot->GetMovementInterface()->IsControllingMovements())
		{
			return AITask<BotClass>::Continue();
		}

		if (!entity)
		{
			return AITask<BotClass>::Done("Entity is NULL!");
		}

		if (m_validator)
		{
			bool result = m_validator(bot, entity);

			if (!result)
			{
				return AITask<BotClass>::Done("Task is no longer valid!");
			}
		}
		
		Vector eyePos = bot->GetEyeOrigin();
		Vector entityPos = UtilHelpers::getWorldSpaceCenter(entity);
		m_rangeToEntity = (entityPos - eyePos).Length();

		if (m_rangeToEntity <= CBaseExtPlayer::PLAYER_USE_RADIUS)
		{
			IPlayerController* input = bot->GetControlInterface();
			input->AimAt(entityPos, IPlayerController::LOOK_PRIORITY, 0.5f, "Looking at USE entity!");
			CBaseEntity* obstruction = nullptr;

			if (!m_timeout.HasStarted())
			{
				m_timeout.Start(5.0f);
			}

			if (input->IsAimOnTarget() && !modhelpers->IsUseObstructed(bot->GetEntity(), entity, &obstruction))
			{
				input->PressUseButton();
				return AITask<BotClass>::Done("Use button pressed!");
			}

			if (obstruction)
			{
				if (bot->IsAbleToBreak(obstruction))
				{
					m_timeout.Invalidate();
					bot->GetMovementInterface()->BreakObstacle(obstruction);
					return AITask<BotClass>::Continue();
				}
			}
		}

		if (m_nav.NeedsRepath())
		{
			PathCostClass cost(bot);
			
			if (!m_nav.ComputePathToPosition(bot, entityPos, cost))
			{
				if (m_counter.Increase())
				{
					return AITask<BotClass>::Done("Too many pathing failures!");
				}
			}
		}

		m_nav.Update(bot);
		return AITask<BotClass>::Continue();
	}

	TaskEventResponseResult<BotClass> OnStuck(BotClass* bot) override
	{
		m_nav.Invalidate();

		if (m_counter.Increase())
		{
			return AITask<BotClass>::TryDone(PRIORITY_CRITICAL, "Too many pathing failures!");
		}

		return AITask<BotClass>::TryToMaintain(PRIORITY_MEDIUM);
	}

	TaskEventResponseResult<BotClass> OnMoveToFailure(BotClass* bot, CPath* path, IEventListener::MovementFailureType reason) override
	{
		if (m_counter.Increase())
		{
			return AITask<BotClass>::TryDone(PRIORITY_CRITICAL, "Too many pathing failures!");
		}

		return AITask<BotClass>::TryToMaintain(PRIORITY_MEDIUM);
	}

	const char* GetName() const override { return "UseEntity"; }

private:
	CHandle<CBaseEntity> m_entity;
	CMeshNavigator m_nav;
	CPathFailCounter m_counter;
	CountdownTimer m_timeout;
	float m_rangeToEntity;
	std::function<bool(BotClass*, CBaseEntity*)> m_validator;
};

#endif // !__NAVBOT_BOT_SHARED_USE_ENTITY_TASK_H_
