#ifndef NAVBOT_BOT_SHARED_ATTACK_NEAREST_ENEMY_TASK_H_
#define NAVBOT_BOT_SHARED_ATTACK_NEAREST_ENEMY_TASK_H_

#include <extension.h>
#include <bot/basebot.h>
#include <bot/basebot_pathcost.h>
#include <bot/interfaces/path/meshnavigator.h>
#include <sdkports/sdk_timers.h>

/**
 * @brief Shared general purpose AI task for attacking the nearest enemy.
 * 
 * Bots will move towards the enemy if they are outside their weapon's maximum attack range.
 * 
 * The tasks when no enemies are currently visible.
 * @tparam BT 
 * @tparam CT 
 */
template <typename BT, typename CT = CBaseBotPathCost>
class CBotSharedAttackNearestEnemyTask : public AITask<BT>
{
public:
	CBotSharedAttackNearestEnemyTask(BT* bot) :
		m_pathCost(bot), m_goal(0.0f, 0.0f, 0.0f)
	{
	}

	TaskResult<BT> OnTaskUpdate(BT* bot) override;

	const char* GetName() const override { return "AttackNearestEnemy"; }

private:
	CT m_pathCost;
	CMeshNavigator m_nav;
	CountdownTimer m_repathTimer;
	Vector m_goal;
};

template<typename BT, typename CT>
inline TaskResult<BT> CBotSharedAttackNearestEnemyTask<BT, CT>::OnTaskUpdate(BT* bot)
{
	auto threat = bot->GetSensorInterface()->GetPrimaryKnownThreat(true);

	if (!threat)
	{
		return AITask<BT>::Done("No visible threat!");
	}

	const CBotWeapon* weapon = bot->GetInventoryInterface()->GetActiveBotWeapon();
	bool move = false;

	if (weapon)
	{
		const WeaponInfo* info = weapon->GetWeaponInfo();
		auto& attackinfo = info->GetAttackInfo(WeaponInfo::PRIMARY_ATTACK);

		if (attackinfo.IsMelee())
		{
			move = true;
		}
		else
		{
			const float range = bot->GetRangeTo(threat->GetEntity());

			if (range > attackinfo.GetMaxRange())
			{
				move = true;
			}
		}
	}

	if (move)
	{
		if (!m_nav.IsValid() || m_repathTimer.IsElapsed())
		{
			m_repathTimer.Start(0.5f);
			m_goal = threat->GetLastKnownPosition();
			m_nav.ComputePathToPosition(bot, m_goal, m_pathCost);
		}

		m_nav.Update(bot);
	}

	return AITask<BT>::Continue();
}

#endif // !NAVBOT_BOT_SHARED_ATTACK_NEAREST_ENEMY_TASK_H_