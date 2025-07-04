#include NAVBOT_PCH_FILE
#include <extension.h>
#include <bot/blackmesa/bmbot.h>
#include <bot/blackmesa/tasks/bmbot_find_armor_task.h>
#include <bot/blackmesa/tasks/bmbot_find_weapon_task.h>
#include <bot/blackmesa/tasks/bmbot_find_ammo_task.h>
#include "bmbot_roam_task.h"
#include "bmbot_scenario_deathmatch_task.h"

TaskResult<CBlackMesaBot> CBlackMesaBotScenarioDeathmatchTask::OnTaskUpdate(CBlackMesaBot* bot)
{
	int randomNumber = CBaseBot::s_botrng.GetRandomInt<int>(1, 10);

	// small random chance to ignore weapons
	if (randomNumber > 3 && CBlackMesaBotFindWeaponTask::IsPossible(bot))
	{
		// if find weapon took less than 5 seconds, it probably failed.
		if (!m_searchWeaponTimer.HasStarted() || m_searchWeaponTimer.IsGreaterThen(5.0f))
		{
			m_searchWeaponTimer.Start();
			return PauseFor(new CBlackMesaBotFindWeaponTask, "Searching for new weapons!");
		}
	}

	blackmesa::BMAmmoIndex ammoInNeed;

	if (CBlackMesaBotFindAmmoTask::IsPossible(bot, ammoInNeed))
	{
		return PauseFor(new CBlackMesaBotFindAmmoTask(ammoInNeed), "Searching for ammo!");
	}

	// small random chance to ignore armor
	if (randomNumber < 9 && CBlackMesaBotFindArmorTask::IsPossible(bot))
	{
		if (!m_searchArmorTimer.HasStarted() || m_searchArmorTimer.IsGreaterThen(5.0f))
		{
			return PauseFor(new CBlackMesaBotFindArmorTask, "Searching for armor!");
		}
	}

	return PauseFor(new CBlackMesaBotRoamTask, "Roaming!");
}
