#ifndef __NAVBOT_BOT_SHARED_SELECT_DM_BEHAVIOR_TASK_
#define __NAVBOT_BOT_SHARED_SELECT_DM_BEHAVIOR_TASK_

#include <mods/basemod.h>
#include "bot_shared_simple_deathmatch_behavior.h"
#include "bot_shared_team_deathmatch_behavior.h"

// Selects between FFA DM and TDM based on what the mod interface tell us about the current gamemode type
template <typename BT, typename CT>
class CBotSharedSelectDMBehaviorTask : public AITask<BT>
{
public:


	TaskResult<BT> OnTaskStart(BT* bot, AITask<BT>* pastTask)
	{
		if (extmanager->GetMod()->IsTeamBasedGame())
		{
			return AITask<BT>::SwitchTo(new CBotSharedTeamDeathmatchBehaviorTask<BT, CT>, "Starting team deathmatch behavior!");
		}

		return AITask<BT>::SwitchTo(new CBotSharedSimpleDMBehaviorTask<BT, CT>, "Starting free for all deathmatch behavior!");
	}

	TaskResult<BT> OnTaskUpdate(BT* bot) override
	{
		return AITask<BT>::Done();
	}

	const char* GetName() const override { return "SelectDMBehavior"; }

private:

};

#endif // !__NAVBOT_BOT_SHARED_SELECT_DM_BEHAVIOR_TASK_
