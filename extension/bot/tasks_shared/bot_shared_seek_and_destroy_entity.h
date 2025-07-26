#ifndef __NAVBOT_BOT_SHARED_SEEK_AND_DESTROY_ENTITY_H_
#define __NAVBOT_BOT_SHARED_SEEK_AND_DESTROY_ENTITY_H_

#include <functional>
#include <extension.h>
#include <sdkports/sdk_ehandle.h>
#include <sdkports/sdk_timers.h>
#include <bot/basebot.h>
#include <bot/basebot_pathcost.h>
#include <bot/interfaces/path/chasenavigator.h>

/**
 * @brief General purpose task for making bots seek and destroy an enemy.
 * 
 * The task will end if the entity is invalid, dead or no longer considered an enemy by the bot.
 * 
 * An optional validator function can be passed to end the task.
 * @tparam BT Bot class.
 * @tparam CT Bot path cost class.
 */
template <typename BT, typename CT = CBaseBotPathCost>
class CBotSharedSeekAndDestroyEntityTask : public AITask<BT>
{
public:
	CBotSharedSeekAndDestroyEntityTask(BT* bot, CBaseEntity* target, const float losttime = -1.0f) :
		m_pathcost(bot), m_target(target)
	{
		m_targetVisibleTimer.Start();
		m_noLOStime = losttime;
	}

	/**
	 * @brief Sets an extra validation function.
	 * @tparam FUNC bool (CBaseEntity* target) function.
	 * @param functor Functor, return FALSE to end the task. TRUE to continue.
	 */
	template<typename FUNC>
	void SetValidatorFunction(FUNC functor)
	{
		m_validatorFunc = functor;
	}

	TaskResult<BT> OnTaskUpdate(BT* bot) override;

	const char* GetName() const override { return "SeekAndDestroyEntity"; }
private:
	CT m_pathcost;
	CChaseNavigator m_nav;
	CHandle<CBaseEntity> m_target;
	IntervalTimer m_targetVisibleTimer;
	float m_noLOStime;
	std::function<bool(CBaseEntity*)> m_validatorFunc;
};

template<typename BT, typename CT>
inline TaskResult<BT> CBotSharedSeekAndDestroyEntityTask<BT, CT>::OnTaskUpdate(BT* bot)
{
	CBaseEntity* target = m_target.Get();

	if (!target)
	{
		return AITask<BT>::Done("Target entity is NULL!");
	}

	if (!UtilHelpers::IsEntityAlive(target))
	{
		return AITask<BT>::Done("Target entity is DEAD!");
	}

	if (m_validatorFunc)
	{
		bool result = m_validatorFunc(target);

		if (!result)
		{
			return AITask<BT>::Done("Target entity is no longer valid!");
		}
	}

	ISensor* sensor = bot->GetSensorInterface();

	if (sensor->IsIgnored(target) || !sensor->IsEnemy(target))
	{
		return AITask<BT>::Done("Target entity is no longer an enemy!");
	}

	if (sensor->IsAbleToSee(UtilHelpers::getWorldSpaceCenter(target)))
	{
		m_targetVisibleTimer.Start();
	}
	else
	{
		if (m_noLOStime > 0.0f && m_targetVisibleTimer.IsGreaterThen(m_noLOStime))
		{
			return AITask<BT>::Done("Lost LOS to target entity!");
		}
	}

	m_nav.Update(bot, target, m_pathcost);
	return AITask<BT>::Continue();
}

#endif // !__NAVBOT_BOT_SHARED_SEEK_AND_DESTROY_ENTITY_H_
