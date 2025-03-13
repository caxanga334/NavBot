#ifndef NAVBOT_BOT_SHARED_PURSUE_AND_DESTROY_TASK_H_
#define NAVBOT_BOT_SHARED_PURSUE_AND_DESTROY_TASK_H_

#include <extension.h>
#include <util/helpers.h>
#include <bot/basebot.h>
#include <bot/basebot_pathcost.h>
#include <bot/interfaces/path/meshnavigator.h>
#include <navmesh/nav_mesh.h>
#include <navmesh/nav_area.h>
#include <sdkports/sdk_timers.h>
#include <sdkports/sdk_ehandle.h>

template <typename BT, typename CT = CBaseBotPathCost>
class CBotSharedPursueAndDestroyTask : public AITask<BT>
{
public:
	/**
	 * @brief Task constructor.
	 * @param bot Bot that will be performing this task.
	 * @param enemy Enemy the bot will chase.
	 * @param maxChaseLength Maximum time to chase this enemy.
	 * @param hasSixthSense If set to true, the bot will always known where the enemy is.
	 */
	CBotSharedPursueAndDestroyTask(BT* bot, CBaseEntity* enemy, const float maxChaseLength = 60.0f, bool hasSixthSense = false) :
		m_pathCost(bot), m_enemy(enemy), m_moveGoal(0.0f, 0.0f, 0.0f)
	{
		m_chaseLength = maxChaseLength;
		m_hasSixthSense = hasSixthSense;
		m_lostLOS = false;
		m_sawLKP = false;
		m_patrolDone = false;
		m_pathfails = 0;
	}

	TaskResult<BT> OnTaskStart(BT* bot, AITask<BT>* pastTask) override;
	TaskResult<BT> OnTaskUpdate(BT* bot) override;

	const char* GetName() const override { return "PursueAndDestroy"; }

private:
	float m_chaseLength;
	bool m_hasSixthSense;
	bool m_lostLOS;
	bool m_sawLKP;
	bool m_patrolDone;
	int m_pathfails;
	CT m_pathCost;
	CMeshNavigator m_nav;
	CountdownTimer m_repathTimer;
	CountdownTimer m_timeout;
	Vector m_moveGoal;
	CHandle<CBaseEntity> m_enemy;
};

template<typename BT, typename CT>
inline TaskResult<BT> CBotSharedPursueAndDestroyTask<BT, CT>::OnTaskStart(BT* bot, AITask<BT>* pastTask)
{
	CBaseEntity* enemy = m_enemy.Get();

	if (!enemy || !UtilHelpers::IsEntityAlive(enemy))
	{
		return AITask<BT>::Done("Enemy is dead or invalid!");
	}

	// make sure the enemy is in the bot's sensor database
	auto known = bot->GetSensorInterface()->AddKnownEntity(enemy);
	
	if (known)
	{
		known->UpdatePosition();
	}

	m_timeout.Start(m_chaseLength);

	return AITask<BT>::Continue();
}

template<typename BT, typename CT>
inline TaskResult<BT> CBotSharedPursueAndDestroyTask<BT, CT>::OnTaskUpdate(BT* bot)
{
	if (m_chaseLength > 0.0f && m_timeout.IsElapsed())
	{
		return AITask<BT>::Done("Chase timer elapsed, giving up!");
	}

	CBaseEntity* enemy = m_enemy.Get();

	if (!enemy || !UtilHelpers::IsEntityAlive(enemy))
	{
		return AITask<BT>::Done("Enemy is dead or invalid!");
	}

	auto known = bot->GetSensorInterface()->GetKnown(enemy);

	if (!known)
	{
		if (m_hasSixthSense)
		{
			// make sure the enemy is in the bot's sensor database
			auto known = bot->GetSensorInterface()->AddKnownEntity(enemy);

			if (known)
			{
				known->UpdatePosition();
			}
		}
		else
		{
			return AITask<BT>::Done("Bot lost track of enemy!");
		}
	}

	if (m_hasSixthSense || known->IsVisibleNow())
	{
		m_lostLOS = false;
		m_sawLKP = false;
		m_patrolDone = false;
		m_moveGoal = UtilHelpers::getEntityOrigin(enemy);
	}
	else if (!m_lostLOS && !m_sawLKP)
	{
		// move to LKP
		m_lostLOS = true;
		m_moveGoal = known->GetLastKnownPosition();
	}
	else if (m_lostLOS && m_sawLKP && !m_patrolDone)
	{
		// Find a position based on the last known target velocity
		Vector LKP = known->GetLastKnownPosition();
		Vector speed = known->GetLastKnownVelocity();
		m_moveGoal = LKP + (speed * (known->GetTimeSinceLastVisible() + 0.3f));
		CNavArea* area = TheNavMesh->GetNearestNavArea(m_moveGoal, 1024.0f);

		if (!area)
		{
			return AITask<BT>::Done("Failed to get patrol area!");
		}

		m_moveGoal = area->GetRandomPoint();
		m_patrolDone = true;
	}

	if (!m_nav.IsValid() || m_repathTimer.IsElapsed())
	{
		m_repathTimer.Start(1.0f);

		if (!m_nav.ComputePathToPosition(bot, m_moveGoal, m_pathCost))
		{
			if (++m_pathfails > 10)
			{
				return AITask<BT>::Done("Too many pathing failures! Giving up.");
			}
		}
	}

	if (m_patrolDone)
	{
		if (bot->GetRangeTo(m_moveGoal) <= 256.0f)
		{
			bot->GetControlInterface()->AimAt(m_moveGoal, IPlayerController::LOOK_ALERT, 1.0f, "Looking at patrol position!");

			if (bot->GetSensorInterface()->IsAbleToSee(m_moveGoal))
			{
				return AITask<BT>::Done("Enemy eluded me!");
			}
		}
	}
	else if (m_lostLOS)
	{
		if (bot->GetRangeTo(m_moveGoal) <= 256.0f)
		{
			bot->GetControlInterface()->AimAt(m_moveGoal, IPlayerController::LOOK_ALERT, 1.0f, "Looking at the enemy last known position position!");

			if (bot->GetSensorInterface()->IsAbleToSee(m_moveGoal))
			{
				m_sawLKP = true;
			}
		}
	}
	else
	{
		if (bot->GetRangeTo(m_moveGoal) <= 900.0f)
		{
			bot->GetControlInterface()->AimAt(m_moveGoal, IPlayerController::LOOK_ALERT, 1.0f, "Looking for the enemy!");
		}
	}

	return AITask<BT>::Continue();
}

#endif // !NAVBOT_BOT_SHARED_PURSUE_AND_DESTROY_TASK_H_
