#ifndef __NAVBOT_BOT_SHARED_SIMPLE_DEATHMATCH_BEHAVIOR_TASK_H_
#define __NAVBOT_BOT_SHARED_SIMPLE_DEATHMATCH_BEHAVIOR_TASK_H_

#include <extension.h>
#include <manager.h>
#include <mods/basemod.h>
#include <bot/basebot.h>
#include <bot/basebot_pathcost.h>
#include "bot_shared_attack_enemy.h"
#include "bot_shared_search_area.h"
#include "bot_shared_defend_spot.h"
#include "bot_shared_roam.h"

/**
 * @brief Simple deathmatch behavior. Moves randomly and attacks visible enemies.
 * @tparam BT Bot class.
 * @tparam CT Bot path cost class.
 */
template <typename BT, typename CT = CBaseBotPathCost>
class CBotSharedSimpleDMBehaviorTask : public AITask<BT>
{
public:
	TaskResult<BT> OnTaskUpdate(BT* bot) override;

	const char* GetName() const override { return "Deathmatch"; }
};

template<typename BT, typename CT>
inline TaskResult<BT> CBotSharedSimpleDMBehaviorTask<BT, CT>::OnTaskUpdate(BT* bot)
{
	const CKnownEntity* known = bot->GetSensorInterface()->GetPrimaryKnownThreat();

	if (known)
	{
		if (known->IsVisibleNow())
		{
			return AITask<BT>::PauseFor(new CBotSharedAttackEnemyTask<BT, CT>(bot), "Attacking visible threat!");
		}
		else
		{
			return AITask<BT>::PauseFor(new CBotSharedSearchAreaTask<BT, CT>(bot, known->GetLastKnownPosition()), "Searching for enemy!");
		}
	}

	CBaseMod* mod = extmanager->GetMod();
	bool camp = mod->GetModSettings()->RollDefendChance(); // use defend chance as a random camp chance

	if (camp)
	{
		// Use defend task to simulate camping
		// TO-DO: Proper camping task that selects a corner/hiding spot?
		const Vector origin = bot->GetAbsOrigin();
		return AITask<BT>::PauseFor(new CBotSharedDefendSpotTask<BT, CT>(bot, origin, -1.0f, true), "Camping!");
	}

	return AITask<BT>::PauseFor(new CBotSharedRoamTask<BT, CT>(bot, 10000.0f, true, -1.0f, true), "Roaming!");
}

#endif // !__NAVBOT_BOT_SHARED_SIMPLE_DEATHMATCH_BEHAVIOR_TASK_H_
