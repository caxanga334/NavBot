#include NAVBOT_PCH_FILE
#include <bot/basebot.h>
#include "dynamic_priority_manager.h"
#include "dynamic_priorities.h"

bool CBaseDynamicPriority::Configure(const std::vector<std::string>& args)
{
	if (args.empty()) { return false; }

	// first argument is the priority value

	try
	{
		int value = std::stoi(args[0]);
		SetPriority(value);
	}
	catch (...)
	{
		return false;
	}

	return true;
}

int CDynamicPriorityHealth::GetPriorityValue(const CBaseBot* bot, const CBotWeapon* weapon, const CKnownEntity* threat) const
{
	return CalculatePriority(bot->GetHealthPercentage());
}

int CDynamicPriorityEnemyRange::GetPriorityValue(const CBaseBot* bot, const CBotWeapon* weapon, const CKnownEntity* threat) const
{
	return CalculatePriority(bot->GetCombatInterface()->GetCachedCombatData().enemy_range);
}

int CDynamicPriorityHasSecondaryAmmo::GetPriorityValue(const CBaseBot* bot, const CBotWeapon* weapon, const CKnownEntity* threat) const
{
	if (weapon->hasSecondaryAmmo(bot))
	{
		return GetPriority();
	}

	return 0;
}

int CDynamicPriotityBotAggression::GetPriorityValue(const CBaseBot* bot, const CBotWeapon* weapon, const CKnownEntity* threat) const
{
	return CalculatePriority(bot->GetDifficultyProfile()->GetAggressiveness());
}
