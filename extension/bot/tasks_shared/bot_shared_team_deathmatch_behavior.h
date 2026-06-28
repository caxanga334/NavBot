#ifndef __NAVBOT_BOT_SHARED_TEAM_DEATHMATCH_BEHAVIOR_TASK_
#define __NAVBOT_BOT_SHARED_TEAM_DEATHMATCH_BEHAVIOR_TASK_

#include "bot_shared_clear_reported_enemy.h"
#include "bot_shared_patrol_uncleared_areas.h"
#include "bot_shared_default_combat_tasks.h"
#include "bot_shared_roam.h"

// Generic team deathmatch behavior for bots
template <typename BT, typename CT>
class CBotSharedTeamDeathmatchBehaviorTask : public AITask<BT>
{
public:

	TaskResult<BT> OnTaskUpdate(BT* bot) override
	{
		const CKnownEntity* threat = bot->GetSensorInterface()->GetPrimaryKnownThreat(ISensor::ONLY_VISIBLE_THREATS);

		if (threat)
		{
			return AITask<BT>::PauseFor(new CBotSharedDefaultCombatBehaviorTask<BT, CT>, "Attacking visible enemy!");
		}

		CBaseEntity* enemy = nullptr;
		if (CBotSharedClearReportedEnemyTask<BT, CT>::IsPossible(bot, &enemy))
		{
			return AITask<BT>::PauseFor(new CBotSharedClearReportedEnemyTask<BT, CT>(enemy), "Investigating reported enemy position!");
		}
		
		// Only patrol after 5 seconds since last spawn. Gives times for the bots to get out of their spawn room.
		if (bot->GetTimeSinceLastSpawn() >= 5.0f)
		{
			CNavArea* patrolArea = nullptr;
			if (CBotSharedPatrolUnclearedAreasTask<BT, CT>::IsPossible(bot, &patrolArea))
			{
				return AITask<BT>::PauseFor(new CBotSharedPatrolUnclearedAreasTask<BT, CT>(patrolArea), "Patrolling for enemies!");
			}
		}

		return AITask<BT>::PauseFor(new CBotSharedRoamTask<BT, CT>(bot, 8192.0f, true), "Roaming around!");
	}

	const char* GetName() const override { return "TeamDeathmatch"; }

private:

};


#endif // !__NAVBOT_BOT_SHARED_TEAM_DEATHMATCH_BEHAVIOR_TASK_
