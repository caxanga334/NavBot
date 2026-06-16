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

bool CDynamicPriotityEnemyClassnameMatches::Configure(const std::vector<std::string>& args)
{
	if (!CBaseDynamicPriority::Configure(args)) { return false; }

	if (args.size() < 2) { return false; }

	const std::string& arg2 = args[1];

	if (arg2.empty()) { return false; }

	m_classnamepattern = arg2;
	return true;
}

int CDynamicPriotityEnemyClassnameMatches::GetPriorityValue(const CBaseBot* bot, const CBotWeapon* weapon, const CKnownEntity* threat) const
{
	if (m_classnamepattern.empty()) { return 0; }

	if (threat->IsEntityOfClassname(m_classnamepattern.c_str()))
	{
		return GetPriority();
	}

	return 0;
}
