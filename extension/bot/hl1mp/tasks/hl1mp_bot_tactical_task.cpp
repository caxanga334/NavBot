#include NAVBOT_PCH_FILE
#include <mods/basemod.h>
#include <navmesh/nav_mesh.h>
#include <bot/hl1mp/hl1mp_bot.h>
#include <bot/interfaces/path/meshnavigator.h>
#include <bot/bot_shared_utils.h>
#include <bot/tasks_shared/bot_shared_find_health.h>
#include <bot/tasks_shared/bot_shared_find_armor.h>
#include "hl1mp_bot_use_charger_task.h"
#include "hl1mp_bot_scenario_task.h"
#include "hl1mp_bot_tactical_task.h"

class CHL1MPHealthArmorFilter : public botsharedutils::search::SearchReachableEntities<CHL1MPBot, CNavArea>
{
public:
	CHL1MPHealthArmorFilter(CHL1MPBot* bot) :
		botsharedutils::search::SearchReachableEntities<CHL1MPBot, CNavArea>(bot, extmanager->GetMod()->GetModSettings()->GetCollectItemMaxDistance())
	{
	}

private:
	bool IsEntityValid(CBaseEntity* entity, CNavArea* area) override
	{
		if (botsharedutils::search::SearchReachableEntities<CHL1MPBot, CNavArea>::IsEntityValid(entity, area))
		{
			entities::HBaseEntity be(entity);

			return !be.IsEffectActive(EF_NODRAW);
		}

		return false;
	}
};


CHL1MPBotTacticalTask::CHL1MPBotTacticalTask()
{
}

AITask<CHL1MPBot>* CHL1MPBotTacticalTask::InitialNextTask(CHL1MPBot* bot)
{
	return new CHL1MPBotScenarioTask;
}

TaskResult<CHL1MPBot> CHL1MPBotTacticalTask::OnTaskUpdate(CHL1MPBot* bot)
{
	if (bot->GetBehaviorInterface()->ShouldRetreat(bot) != ANSWER_NO)
	{
		if (bot->GetHealthState() != CBaseBot::HealthState::HEALTH_OK && m_healthscantimer.IsElapsed())
		{
			m_healthscantimer.StartRandom(2.0f, 5.0f);
			CBaseEntity* charger = nullptr;
			
			if (CHL1MPBotUseChargerTask::IsPossible(bot, CHL1MPBotUseChargerTask::ChargerType::HEALTH_CHARGER, &charger))
			{
				return PauseFor(new CHL1MPBotUseChargerTask(charger, CHL1MPBotUseChargerTask::ChargerType::HEALTH_CHARGER), "Using health charger!");
			}

			CBaseEntity* healthkit = nullptr;
			CHL1MPHealthArmorFilter filter{ bot };
			filter.AddSearchPattern("item_healthkit");

			if (CBotSharedFindHealthTask<CHL1MPBot, CHL1MPBotPathCost>::IsPossible(bot, &healthkit, filter))
			{
				return PauseFor(new CBotSharedFindHealthTask<CHL1MPBot, CHL1MPBotPathCost>(bot, healthkit), "I'm low on health!");
			}
		}

		if (bot->GetDifficultyProfile()->GetGameAwareness() > 15 && bot->GetArmorAmount() < 90 && m_armorscantimer.IsElapsed())
		{
			m_armorscantimer.StartRandom(2.0f, 5.0f);
			CBaseEntity* charger = nullptr;

			if (CHL1MPBotUseChargerTask::IsPossible(bot, CHL1MPBotUseChargerTask::ChargerType::ARMOR_CHARGER, &charger))
			{
				return PauseFor(new CHL1MPBotUseChargerTask(charger, CHL1MPBotUseChargerTask::ChargerType::ARMOR_CHARGER), "Using armor charger!");
			}

			CBaseEntity* armor = nullptr;
			CHL1MPHealthArmorFilter filter{ bot };
			filter.AddSearchPattern("item_battery");

			if (CBotSharedFindArmorTask<CHL1MPBot, CHL1MPBotPathCost>::IsPossible(bot, &armor, filter))
			{
				return PauseFor(new CBotSharedFindArmorTask<CHL1MPBot, CHL1MPBotPathCost>(bot, armor), "I'm in need of armor!");
			}
		}
	}

	return Continue();
}

TaskResult<CHL1MPBot> CHL1MPBotTacticalTask::OnTaskResume(CHL1MPBot* bot, AITask<CHL1MPBot>* pastTask)
{
	m_healthscantimer.Start(0.5f);
	m_armorscantimer.Start(0.5f);

	return Continue();
}

QueryAnswerType CHL1MPBotTacticalTask::ShouldRetreat(CBaseBot* me)
{
	if (me->GetHealthState() != CBaseBot::HealthState::HEALTH_OK)
	{
		return ANSWER_YES;
	}

	if (me->GetArmorAmount() < 90)
	{
		return ANSWER_YES;
	}

	return ANSWER_NO;
}
