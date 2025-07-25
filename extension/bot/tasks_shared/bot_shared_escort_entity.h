#ifndef NAVBOT_BOT_SHARED_ESCORT_ENTITY_TASK_H_
#define NAVBOT_BOT_SHARED_ESCORT_ENTITY_TASK_H_

#include <extension.h>
#include <util/helpers.h>
#include <bot/basebot.h>
#include <bot/basebot_pathcost.h>
#include <bot/interfaces/path/meshnavigator.h>
#include <sdkports/sdk_timers.h>
#include <sdkports/sdk_ehandle.h>

/**
 * @brief Generic task for following an entity.
 * @tparam BT Bot class.
 * @tparam CT Bot path cost class.
 */
template <typename BT, typename CT = CBaseBotPathCost>
class CBotSharedEscortEntityTask : public AITask<BT>
{
public:
	/**
	 * @brief Constructor.
	 * @param bot Bot that will run this task.
	 * @param entity Entity to escort.
	 * @param timeout Time to follow this entity, if negative, follow until the entity is killed.
	 * @param escortDistance Maximum distance between the entity and the bot.
	 */
	CBotSharedEscortEntityTask(BT* bot, CBaseEntity* entity, const float timeout = 90.0f, const float escortDistance = 256.0f) :
		m_pathCost(bot)
	{
		m_escortDistance = escortDistance;
		m_timeoutDuration = timeout;
		m_ent = entity;
	}

	TaskResult<BT> OnTaskStart(BT* bot, AITask<BT>* pastTask) override;
	TaskResult<BT> OnTaskUpdate(BT* bot) override;

	// Already bush escorting something, don't help teammates.
	QueryAnswerType ShouldAssistTeammate(CBaseBot* me, CBaseEntity* teammate) override { return ANSWER_NO; }

	const char* GetName() const override { return "EscortEntity"; }

private:
	CT m_pathCost;
	CMeshNavigator m_nav;
	CountdownTimer m_timeoutTimer;
	float m_escortDistance;
	float m_timeoutDuration;
	CHandle<CBaseEntity> m_ent;
};

template<typename BT, typename CT>
inline TaskResult<BT> CBotSharedEscortEntityTask<BT, CT>::OnTaskStart(BT* bot, AITask<BT>* pastTask)
{
	if (m_ent.Get() == nullptr)
	{
		return AITask<BT>::Done("Escort goal entity is NULL!");
	}

	m_timeoutTimer.Start(m_timeoutDuration);

	return AITask<BT>::Continue();
}

template<typename BT, typename CT>
inline TaskResult<BT> CBotSharedEscortEntityTask<BT, CT>::OnTaskUpdate(BT* bot)
{
	if (m_timeoutDuration > 0.0f && m_timeoutTimer.IsElapsed())
	{
		return AITask<BT>::Done("Timeout timer elapsed!");
	}

	CBaseEntity* entity = m_ent.Get();

	if (!entity || !UtilHelpers::IsEntityAlive(entity))
	{
		return AITask<BT>::Done("Escort entity is invalid or dead!");
	}

	Vector pos = UtilHelpers::getWorldSpaceCenter(entity);
	float range = bot->GetRangeTo(pos);

	if (range > m_escortDistance)
	{
		if (!m_nav.IsValid() || m_nav.NeedsRepath())
		{
			m_nav.StartRepathTimer(1.0f);
			m_nav.ComputePathToPosition(bot, pos, m_pathCost);
		}

		m_nav.Update(bot);
	}

	return AITask<BT>::Continue();
}

#endif // !NAVBOT_BOT_SHARED_ESCORT_ENTITY_TASK_H_
