#include NAVBOT_PCH_FILE
#include <mods/blackmesa/blackmesadm_mod.h>
#include <bot/blackmesa/bmbot.h>
#include <bot/blackmesa/tasks/bmbot_find_armor_task.h>
#include <bot/blackmesa/tasks/bmbot_find_weapon_task.h>
#include <bot/blackmesa/tasks/bmbot_find_ammo_task.h>
#include <bot/tasks_shared/bot_shared_patrol_uncleared_areas.h>
#include <bot/tasks_shared/bot_shared_clear_reported_enemy.h>
#include <bot/tasks_shared/bot_shared_roam.h>
#include "bmbot_scenario_deathmatch_task.h"

TaskResult<CBlackMesaBot> CBlackMesaBotScenarioDeathmatchTask::OnTaskUpdate(CBlackMesaBot* bot)
{
	int randomNumber = CBaseBot::s_botrng.GetRandomInt<int>(1, 10);

	// small random chance to ignore weapons
	if (randomNumber > 3)
	{
		// if find weapon took less than 5 seconds, it probably failed.
		if (!m_searchWeaponTimer.HasStarted() || m_searchWeaponTimer.IsGreaterThen(15.0f))
		{
			m_searchWeaponTimer.Start();
			CBaseEntity* weapon = nullptr;
			if (CBlackMesaBotFindWeaponTask::IsPossible(bot, &weapon))
			{
				return PauseFor(new CBlackMesaBotFindWeaponTask(weapon), "Searching for new weapons!");
			}
		}
	}

	blackmesa::BMAmmoIndex ammoInNeed;
	CBaseEntity* ammoEntity;

	if (CBlackMesaBotFindAmmoTask::IsPossible(bot, &ammoEntity, ammoInNeed))
	{
		return PauseFor(new CBlackMesaBotFindAmmoTask(ammoEntity, ammoInNeed), "Searching for ammo!");
	}

	// small random chance to ignore armor
	if (randomNumber < 9)
	{
		if (!m_searchArmorTimer.HasStarted() || m_searchArmorTimer.IsGreaterThen(15.0f))
		{
			m_searchArmorTimer.Start();

			CBaseEntity* item = nullptr;
			if (CBlackMesaBotFindArmorTask::IsPossible(bot, &item))
			{
				return PauseFor(new CBlackMesaBotFindArmorTask(item), "Searching for armor!");
			}
		}
	}

	// in DM mode, all bots are on the same team and these tasks uses information shared with the entire team.
	if (CBlackMesaDeathmatchMod::GetBMMod()->IsTeamPlay())
	{
		CBaseEntity* target;

		if (CBotSharedClearReportedEnemyTask<CBlackMesaBot, CBlackMesaBotPathCost>::IsPossible(bot, &target))
		{
			return PauseFor(new CBotSharedClearReportedEnemyTask<CBlackMesaBot, CBlackMesaBotPathCost>(target), "Investigating enemy position!");
		}

		CNavArea* area;

		if (CBaseBot::s_botrng.GetRandomChance(66) && CBotSharedPatrolUnclearedAreasTask<CBlackMesaBot, CBlackMesaBotPathCost>::IsPossible(bot, &area))
		{
			return PauseFor(new CBotSharedPatrolUnclearedAreasTask<CBlackMesaBot, CBlackMesaBotPathCost>(area), "Patrolling for enemies!");
		}
	}

	return PauseFor(new CBotSharedRoamTask<CBlackMesaBot, CBlackMesaBotPathCost>(bot, 8192.0f, true), "Roaming!");
}
