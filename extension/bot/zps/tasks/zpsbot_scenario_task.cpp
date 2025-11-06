#include NAVBOT_PCH_FILE
#include <bot/zps/zpsbot.h>
#include <mods/zps/zps_mod.h>
#include <mods/zps/nav/zps_nav_mesh.h>
#include <bot/tasks_shared/bot_shared_collect_items.h>
#include "scenario/zpsbot_survival_human_task.h"
#include "scenario/zpsbot_survival_zombie_task.h"
#include "zpsbot_find_weapon_task.h"
#include "zpsbot_scenario_task.h"

CZPSBotScenarioTask::CZPSBotScenarioTask()
{
	m_roundisactive = CZombiePanicSourceMod::GetZPSMod()->IsRoundActive();
	m_ammoSearchTimer.StartRandom(5.0f, 25.0f);
	m_weaponSearchTimer.StartRandom(5.0f, 12.0f);
	m_healthSearchTimer.StartRandom(10.0f, 20.0f);
	m_armorSearchTimer.StartRandom(10.0f, 20.0f);
}

AITask<CZPSBot>* CZPSBotScenarioTask::SelectScenarioTask(CZPSBot* bot)
{
	zps::ZPSGamemodes gamemode = CZombiePanicSourceMod::GetZPSMod()->GetGameMode();

	if (gamemode == zps::ZPSGamemodes::GAMEMODE_SURVIVAL)
	{
		if (bot->GetMyZPSTeam() == zps::ZPSTeam::ZPS_TEAM_SURVIVORS)
		{
			// survivor bot
			return new CZPSBotSurvivalHumanTask;
		}

		// zombie bot
		return new CZPSBotSurvivalZombieTask;
	}

	return nullptr;
}

AITask<CZPSBot>* CZPSBotScenarioTask::InitialNextTask(CZPSBot* bot)
{
	// Wait until the round is active to pick a scenario behavior
	if (!m_roundisactive)
	{
		return nullptr;
	}

	return CZPSBotScenarioTask::SelectScenarioTask(bot);
}

TaskResult<CZPSBot> CZPSBotScenarioTask::OnTaskStart(CZPSBot* bot, AITask<CZPSBot>* pastTask)
{
	return Continue();
}

TaskResult<CZPSBot> CZPSBotScenarioTask::OnTaskUpdate(CZPSBot* bot)
{
	if (!m_roundisactive)
	{
		if (CZombiePanicSourceMod::GetZPSMod()->IsRoundActive())
		{
			return SwitchTo(new CZPSBotScenarioTask, "Round is active, restarting scenario behavior!");
		}

		return Continue();
	}

	if (bot->GetBehaviorInterface()->ShouldRetreat(bot) != ANSWER_NO && bot->GetBehaviorInterface()->ShouldHurry(bot) != ANSWER_YES)
	{
		// Survivors only
		if (bot->GetMyZPSTeam() == zps::ZPSTeam::ZPS_TEAM_SURVIVORS)
		{
			if (m_ammoSearchTimer.IsElapsed())
			{
				m_ammoSearchTimer.StartRandom(7.0f, 15.0f);

				const CBotWeapon* low = bot->GetInventoryInterface()->GetWeaponWithLowAmmo();

				if (low && low->GetWeaponInfo()->HasAmmoSourceEntityClassname())
				{
					NBotSharedCollectItemTask::ItemCollectFilter<CZPSBot, CZPSNavArea> filter{ bot };
					const char* pattern = low->GetWeaponInfo()->GetAmmoSourceEntityClassname().c_str();
					CBaseEntity* item = nullptr;

					if (CBotSharedCollectItemsTask<CZPSBot, CZPSBotPathCost>::IsPossible(bot, &filter, pattern, &item))
					{
						auto task = new CBotSharedCollectItemsTask<CZPSBot, CZPSBotPathCost>(bot, item, NBotSharedCollectItemTask::COLLECT_PRESS_USE);
						return PauseFor(task, "Going for ammo!");
					}
				}
			}
			else if (m_weaponSearchTimer.IsElapsed())
			{
				m_weaponSearchTimer.StartRandom(12.0f, 30.0f);

				CBaseEntity* weapon = nullptr;

				if (CZPSBotFindWeaponTask::IsPossible(bot, &weapon))
				{
					return PauseFor(new CZPSBotFindWeaponTask(weapon), "Searching for weapons.");
				}
			}
			else if (m_healthSearchTimer.IsElapsed() && bot->GetHealthState() != CBaseBot::HealthState::HEALTH_OK)
			{
				m_healthSearchTimer.Start(3.0f);

				NBotSharedCollectItemTask::ItemCollectFilter<CZPSBot, CZPSNavArea> filter{ bot };
				std::vector<std::string> patterns{ "item_healthkit", "item_pills" };
				std::vector<CBaseEntity*> output;

				if (CBotSharedCollectItemsTask<CZPSBot, CZPSBotPathCost>::IsPossible(bot, &filter, patterns, output))
				{
					CBaseEntity* item = librandom::utils::GetRandomElementFromVector(output);
					auto task = new CBotSharedCollectItemsTask<CZPSBot, CZPSBotPathCost>(bot, item, NBotSharedCollectItemTask::COLLECT_PRESS_USE);
					auto func = [](CZPSBot* bot, CBaseEntity* entity) {
						if (bot->GetHealthState() == CBaseBot::HealthState::HEALTH_OK)
						{
							return false;
						}

						return true;
					};
					task->SetValidationFunction(func);
					return PauseFor(task, "Going for health!");
				}
			}
			else if (m_armorSearchTimer.IsElapsed() && bot->GetArmorAmount() < 50)
			{
				/* max armor is 100, cap at 50 so others can get armor too */
				m_armorSearchTimer.StartRandom(15.0f, 40.0f);

				NBotSharedCollectItemTask::ItemCollectFilter<CZPSBot, CZPSNavArea> filter{ bot };
				CBaseEntity* item = nullptr;

				if (CBotSharedCollectItemsTask<CZPSBot, CZPSBotPathCost>::IsPossible(bot, &filter, "item_armor", &item))
				{
					auto task = new CBotSharedCollectItemsTask<CZPSBot, CZPSBotPathCost>(bot, item, NBotSharedCollectItemTask::COLLECT_PRESS_USE);
					auto func = [](CZPSBot* bot, CBaseEntity* entity) {
						if (bot->GetArmorAmount() >= 99)
						{
							return false;
						}

						return true;
					};
					task->SetValidationFunction(func);
					return PauseFor(task, "Going for armor!");
				}
			}
		}
	}

	return Continue();
}

TaskResult<CZPSBot> CZPSBotScenarioTask::OnTaskResume(CZPSBot* bot, AITask<CZPSBot>* pastTask)
{
	if (m_ammoSearchTimer.IsElapsed()) { m_ammoSearchTimer.StartRandom(10.0f, 30.0f); }
	if (m_weaponSearchTimer.IsElapsed()) { m_weaponSearchTimer.StartRandom(10.0f, 30.0f); }
	if (m_healthSearchTimer.IsElapsed()) { m_healthSearchTimer.StartRandom(10.0f, 20.0f); }
	if (m_armorSearchTimer.IsElapsed()) { m_armorSearchTimer.StartRandom(10.0f, 20.0f); }

	return Continue();
}

QueryAnswerType CZPSBotScenarioTask::ShouldHurry(CBaseBot* me)
{
	if (!m_roundisactive)
	{
		return ANSWER_YES;
	}

	return ANSWER_UNDEFINED;
}

QueryAnswerType CZPSBotScenarioTask::ShouldRetreat(CBaseBot* me)
{
	if (!m_roundisactive)
	{
		return ANSWER_NO;
	}

	return ANSWER_UNDEFINED;
}
