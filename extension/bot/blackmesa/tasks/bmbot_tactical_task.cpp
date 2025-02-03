#include <extension.h>
#include <bot/blackmesa/bmbot.h>
#include <sdkports/sdk_takedamageinfo.h>
#include "bmbot_tactical_task.h"
#include "scenario/bmbot_scenario_deathmatch_task.h"
#include "bmbot_find_armor_task.h"
#include "bmbot_find_health_task.h"
#include "bmbot_find_weapon_task.h"

AITask<CBlackMesaBot>* CBlackMesaBotTacticalTask::InitialNextTask(CBlackMesaBot* bot)
{
	// BM is DM/TDM only
	return new CBlackMesaBotScenarioDeathmatchTask;
}

TaskResult<CBlackMesaBot> CBlackMesaBotTacticalTask::OnTaskStart(CBlackMesaBot* bot, AITask<CBlackMesaBot>* pastTask)
{
	m_healthScanTimer.StartRandom(2.0f, 4.0f);
	m_armorScanTimer.StartRandom(4.0f, 10.0f);
	m_weaponScanTimer.StartRandom(1.0f, 12.0f);

	return Continue();
}

TaskResult<CBlackMesaBot> CBlackMesaBotTacticalTask::OnTaskUpdate(CBlackMesaBot* bot)
{
	if (bot->GetBehaviorInterface()->ShouldRetreat(bot) != ANSWER_NO && bot->GetBehaviorInterface()->ShouldHurry(bot) != ANSWER_YES)
	{
		if (m_healthScanTimer.IsElapsed())
		{
			m_healthScanTimer.StartRandom(2.0f, 4.0f);

			if (CBlackMesaBotFindHealthTask::IsPossible(bot))
			{
				return PauseFor(new CBlackMesaBotFindHealthTask, "Searching for health!");
			}
		}

		if (m_weaponScanTimer.IsElapsed())
		{
			m_weaponScanTimer.StartRandom(1.0f, 12.0f);

			if (CBlackMesaBotFindWeaponTask::IsPossible(bot))
			{
				return PauseFor(new CBlackMesaBotFindWeaponTask, "Searching for weapons!");
			}
		}

		if (m_armorScanTimer.IsElapsed())
		{
			m_armorScanTimer.StartRandom(4.0f, 10.0f);

			if (CBlackMesaBotFindArmorTask::IsPossible(bot))
			{
				return PauseFor(new CBlackMesaBotFindArmorTask, "Searching for armor!");
			}
		}
	}

	return Continue();
}

TaskResult<CBlackMesaBot> CBlackMesaBotTacticalTask::OnTaskResume(CBlackMesaBot* bot, AITask<CBlackMesaBot>* pastTask)
{
	m_healthScanTimer.StartRandom(2.0f, 4.0f);
	m_armorScanTimer.StartRandom(4.0f, 10.0f);
	m_weaponScanTimer.StartRandom(1.0f, 12.0f);

	return Continue();
}
