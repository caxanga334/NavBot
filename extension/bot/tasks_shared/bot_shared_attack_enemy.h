#ifndef __NAVBOT_BOT_SHARED_ATTACK_ENEMY_TASK_H_
#define __NAVBOT_BOT_SHARED_ATTACK_ENEMY_TASK_H_

#include <extension.h>
#include <bot/basebot.h>
#include <bot/basebot_pathcost.h>
#include <bot/interfaces/path/chasenavigator.h>

/**
 * @brief Shared task for bots to attack visible enemies.
 * 
 * Bots will chase enemies for a limited time. Tasks ends when no threat is visible.
 * @tparam BT Bot class
 * @tparam CT Path cost class
 */
template <typename BT, typename CT = CBaseBotPathCost>
class CBotSharedAttackEnemyTask : public AITask<BT>
{
public:
	/**
	 * @brief Constructor
	 * @param bot Bot that will do this task
	 * @param chaseTime How long to keep chasing an out of sight enemy
	 */
	CBotSharedAttackEnemyTask(BT* bot, const float chaseTime = 3.0f) :
		m_pathcost(bot), m_nav(CChaseNavigator::SubjectLeadType::DONT_LEAD_SUBJECT)
	{
		m_chaseTime = chaseTime;
	}

	TaskResult<BT> OnTaskUpdate(BT* bot) override;

	const char* GetName() const override { return "AttackEnemy"; }
private:
	CT m_pathcost;
	CChaseNavigator m_nav;
	float m_chaseTime;
};

template<typename BT, typename CT>
inline TaskResult<BT> CBotSharedAttackEnemyTask<BT, CT>::OnTaskUpdate(BT* bot)
{
	const CKnownEntity* threat = bot->GetSensorInterface()->GetPrimaryKnownThreat(false);

	if (!threat)
	{
		return AITask<BT>::Done("No threat!");
	}

	if (!threat->IsVisibleNow() && threat->GetTimeSinceLastInfo() >= m_chaseTime)
	{
		return AITask<BT>::Done("Threat has escaped!");
	}

	float moveRange = 750.0f;
	const CBotWeapon* weapon = bot->GetInventoryInterface()->GetActiveBotWeapon();

	if (weapon)
	{
		moveRange = weapon->GetWeaponInfo()->GetAttackRange();
	}

	CBaseEntity* pEntity = threat->GetEntity();

	if (bot->GetRangeTo(pEntity) >= moveRange || !threat->IsVisibleNow())
	{
		m_nav.Update(bot, pEntity, m_pathcost);
	}

	return AITask<BT>::Continue();
}

#endif // !__NAVBOT_BOT_SHARED_ATTACK_ENEMY_TASK_H_
