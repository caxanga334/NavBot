#ifndef __NAVBOT_BOT_SHARED_DROP_WEAPON_TASK_H_
#define __NAVBOT_BOT_SHARED_DROP_WEAPON_TASK_H_

#include <extension.h>
#include <bot/basebot.h>

/**
 * @brief Utility task for making the bot drop a specific weapon.
 * 
 * Note: IInventory::DropHeldWeapon must be implemented for this to work.
 * @tparam BT Bot class.
 */
template <typename BT>
class CBotSharedDropWeaponTask : public AITask<BT>
{
public:
	CBotSharedDropWeaponTask(CBaseEntity* weapon, const float delay = 1.5f) :
		m_weapon(weapon), m_delay(delay)
	{
	}

	CBotSharedDropWeaponTask(const CBotWeapon* weapon, const float delay = 1.5f) :
		m_weapon(weapon->GetEntity()), m_delay(delay)
	{
	}

	TaskResult<BT> OnTaskStart(BT* bot, AITask<BT>* pastTask) override
	{
		if (m_weapon.Get() == nullptr)
		{
			return AITask<BT>::Done("Weapon is NULL!");
		}

		bot->GetInventoryInterface()->RequestRefresh();

		return AITask<BT>::Continue();
	}

	TaskResult<BT> OnTaskUpdate(BT* bot) override
	{
		if (!m_timer.IsElapsed())
		{
			return AITask<BT>::Continue();
		}

		CBaseEntity* weapon = m_weapon.Get();

		if (!weapon)
		{
			return AITask<BT>::Done("Weapon is NULL!");
		}

		IInventory* inv = bot->GetInventoryInterface();
		
		const CBotWeapon* bw = inv->GetWeaponOfEntity(weapon);

		if (!bw)
		{
			return AITask<BT>::Done("Weapon is NULL!");
		}

		if (inv->GetActiveBotWeapon() != bw)
		{
			inv->EquipWeapon(bw);
			m_timer.Start(m_delay);
		}
		else
		{
			inv->DropHeldWeapon();
			return AITask<BT>::Done("Weapon dropped!");
		}

		return AITask<BT>::Continue();
	}

	void OnTaskEnd(BT* bot, AITask<BT>* nextTask) override
	{
		bot->GetInventoryInterface()->RequestRefresh();
	}

	const char* GetName() const override { return "DropWeapon"; }
private:
	CHandle<CBaseEntity> m_weapon;
	float m_delay;
	CountdownTimer m_timer;
};




#endif // !__NAVBOT_BOT_SHARED_DROP_WEAPON_TASK_H_
