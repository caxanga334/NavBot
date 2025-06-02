#ifndef __NAVBOT_BOT_SHARED_USE_SELF_HEAL_ITEM_H_
#define __NAVBOT_BOT_SHARED_USE_SELF_HEAL_ITEM_H_

#include <extension.h>
#include <bot/basebot.h>
#include <sdkports/sdk_ehandle.h>
#include <sdkports/sdk_timers.h>

template <typename BT>
class CBotSharedUseSelfHealItemTask : public AITask<BT>
{
public:
	CBotSharedUseSelfHealItemTask(CBaseEntity* item, float timeout, bool useprimaryattack) :
		m_item(item), m_timeout(timeout), m_useprimaryattack(useprimaryattack)
	{
	}

	TaskResult<BT> OnTaskStart(BT* bot, AITask<BT>* pastTask) override;
	TaskResult<BT> OnTaskUpdate(BT* bot) override;
	void OnTaskEnd(BT* bot, AITask<BT>* nextTask) override;

	QueryAnswerType ShouldAttack(CBaseBot* me, const CKnownEntity* them) override { return ANSWER_NO; }
	QueryAnswerType ShouldHurry(CBaseBot* me) override { return ANSWER_YES; }
	QueryAnswerType ShouldRetreat(CBaseBot* me) override { return ANSWER_NO; }
	QueryAnswerType ShouldSwitchToWeapon(CBaseBot* me, const CBotWeapon* weapon) override { return ANSWER_NO; }
	QueryAnswerType ShouldAssistTeammate(CBaseBot* me, CBaseEntity* teammate) override { return ANSWER_NO; }

	const char* GetName() const override { return "UseSelfHealItem"; }
private:
	CHandle<CBaseEntity> m_item;
	float m_timeout;
	bool m_useprimaryattack;
	CountdownTimer m_timer;
};

template<typename BT>
inline TaskResult<BT> CBotSharedUseSelfHealItemTask<BT>::OnTaskStart(BT* bot, AITask<BT>* pastTask)
{
	m_timer.Start(m_timeout);

	return AITask<BT>::Continue();
}

template<typename BT>
inline TaskResult<BT> CBotSharedUseSelfHealItemTask<BT>::OnTaskUpdate(BT* bot)
{
	if (m_timer.IsElapsed())
	{
		return AITask<BT>::Done("Timeout timer is elapsed!");
	}

	CBaseEntity* pItem = m_item.Get();

	if (!pItem)
	{
		return AITask<BT>::Done("Item is NULL!");
	}
	
	const CBotWeapon* weapon = bot->GetInventoryInterface()->GetWeaponOfEntity(pItem);

	if (!weapon)
	{
		return AITask<BT>::Done("I don't own this item!");
	}

	if (weapon->IsOutOfAmmo(bot))
	{
		return AITask<BT>::Done("Out of ammo!");
	}

	const CBotWeapon* activeweapon = bot->GetInventoryInterface()->GetActiveBotWeapon();

	if (activeweapon != weapon)
	{
		bot->SelectWeapon(pItem);
		return AITask<BT>::Continue();
	}

	if (m_useprimaryattack)
	{
		bot->GetControlInterface()->PressAttackButton(0.5f);
	}
	else
	{
		bot->GetControlInterface()->PressSecondaryAttackButton(0.5f);
	}

	return AITask<BT>::Continue();
}

template<typename BT>
inline void CBotSharedUseSelfHealItemTask<BT>::OnTaskEnd(BT* bot, AITask<BT>* nextTask)
{
	bot->GetControlInterface()->ReleaseAllAttackButtons();
}

#endif // !__NAVBOT_BOT_SHARED_USE_SELF_HEAL_ITEM_H_
