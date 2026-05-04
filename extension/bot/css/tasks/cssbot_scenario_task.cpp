#include NAVBOT_PCH_FILE
#include <mods/css/nav/css_nav_mesh.h>
#include <mods/css/css_lib.h>
#include <mods/css/css_mod.h>
#include <bot/css/cssbot.h>
#include <bot/tasks_shared/bot_shared_rogue_behavior.h>
#include <bot/tasks_shared/bot_shared_attack_enemy.h>
#include <bot/tasks_shared/bot_shared_collect_items.h>
#include <bot/tasks_shared/bot_shared_roam.h>
#include <bot/tasks_shared/bot_shared_escort_entity.h>
#include <bot/tasks_shared/bot_shared_go_to_position.h>
#include "scenario/cssbot_plant_bomb_task.h"
#include "scenario/cssbot_search_for_armed_bomb_task.h"
#include "scenario/cssbot_defuse_bomb_task.h"
#include "scenario/cssbot_defend_planted_bomb_task.h"
#include "scenario/cssbot_guard_bomb_site_task.h"
#include "scenario/cssbot_deliver_hostage_task.h"
#include "scenario/cssbot_rescue_hostage_task.h"
#include "scenario/cssbot_guard_hostage_task.h"
#include "scenario/cssbot_guard_rescue_zone_task.h"
#include <bot/interfaces/behavior_utils.h>
#include "cssbot_scenario_task.h"

class CCSSBotRoamTask : public CBotSharedRoamTask<CCSSBot, CCSSBotPathCost>
{
public:
	CCSSBotRoamTask(CCSSBot* bot) :
		CBotSharedRoamTask<CCSSBot, CCSSBotPathCost>(bot, 8192.0f, true)
	{
	}

	TaskResult<CCSSBot> OnTaskUpdate(CCSSBot* bot) override
	{
		CCounterStrikeSourceMod* csmod = CCounterStrikeSourceMod::GetCSSMod();

		if (csmod->IsBombActive())
		{
			return Done("Bomb was planted!");
		}

		return CBotSharedRoamTask<CCSSBot, CCSSBotPathCost>::OnTaskUpdate(bot);
	}
};

class CCSSBotFollowBombCarrierTask : public CBotSharedEscortEntityTask<CCSSBot, CCSSBotPathCost>
{
public:
	CCSSBotFollowBombCarrierTask(CCSSBot* bot, CBaseEntity* c4carrier) :
		CBotSharedEscortEntityTask<CCSSBot, CCSSBotPathCost>(bot, c4carrier, 120.0f, 160.0f)
	{
	}

	TaskResult<CCSSBot> OnTaskUpdate(CCSSBot* bot) override
	{
		CCounterStrikeSourceMod* csmod = CCounterStrikeSourceMod::GetCSSMod();

		if (csmod->IsBombActive())
		{
			return Done("Bomb was planted!");
		}

		return CBotSharedEscortEntityTask<CCSSBot, CCSSBotPathCost>::OnTaskUpdate(bot);
	}

	const char* GetName() const override { return "FollowBombCarrier"; }
};

CCSSBotScenarioTask::CCSSBotScenarioTask()
{
}

AITask<CCSSBot>* CCSSBotScenarioTask::InitialNextTask(CCSSBot* bot)
{
	return SelectScenarioTask(bot);
}

TaskResult<CCSSBot> CCSSBotScenarioTask::OnTaskStart(CCSSBot* bot, AITask<CCSSBot>* pastTask)
{
	BOTBEHAVIOR_IMPLEMENT_SIMPLE_ROGUE_CHECK(CCSSBot, CCSSBotPathCost);

	return Continue();
}

TaskResult<CCSSBot> CCSSBotScenarioTask::OnTaskUpdate(CCSSBot* bot)
{
	const CKnownEntity* threat = bot->GetSensorInterface()->GetPrimaryKnownThreat(ISensor::ONLY_VISIBLE_THREATS);

	if (threat && bot->GetBehaviorInterface()->ShouldSeekAndDestroy(bot, threat) != ANSWER_NO)
	{
		return PauseFor(new CBotSharedAttackEnemyTask<CCSSBot, CCSSBotPathCost>(bot), "Stopping to attack visible threat!");
	}

	if (GetNextTask() == nullptr)
	{
		AITask<CCSSBot>* task = SelectScenarioTask(bot);

		if (task)
		{
			StartNewNextTask(bot, task, "New scenario behavior selected!");
		}
	}

	return Continue();
}

QueryAnswerType CCSSBotScenarioTask::ShouldHurry(CBaseBot* me)
{
	CCounterStrikeSourceMod* csmod = CCounterStrikeSourceMod::GetCSSMod();

	if (csmod->IsBombActive())
	{
		CBaseEntity* bomb = csmod->GetActiveBombEntity();

		// hurry up if less than 20 seconds left to disarm the bomb
		if (bomb && csslib::GetC4TimeRemaining(bomb) <= 20.0f)
		{
			return ANSWER_YES;
		}
	}

	return ANSWER_UNDEFINED;
}

static bool CollectItemsValidationHasDefuser(CCSSBot* bot, CBaseEntity* entity)
{
	if (csslib::HasDefuser(bot->GetEntity()))
	{
		return false;
	}

	return true;
}

static bool CollectItemsValidationHasC4(CCSSBot* bot, CBaseEntity* entity)
{
	if (bot->GetInventoryInterface()->FindWeaponByClassname("weapon_c4") != nullptr)
	{
		return false;
	}

	CBaseEntity* c4 = csslib::GetDroppedC4();

	if (!c4)
	{
		return false;
	}

	if (c4 != entity)
	{
		return false;
	}

	return true;
}

AITask<CCSSBot>* CCSSBotScenarioTask::SelectScenarioTask(CCSSBot* bot) const
{
	Vector goal;

	if (CCSSBotPlantBombTask::IsPossible(bot, goal))
	{
		return new CCSSBotPlantBombTask(goal);
	}

	CCounterStrikeSourceMod* csmod = CCounterStrikeSourceMod::GetCSSMod();

	bool inahurry = bot->GetBehaviorInterface()->ShouldHurry(bot) == ANSWER_YES;
	counterstrikesource::CSSTeam myteam = bot->GetMyCSSTeam();

	// bomb was planted
	if (csmod->IsBombActive())
	{
		// CT behavior
		if (myteam == counterstrikesource::CSSTeam::COUNTERTERRORIST)
		{
			// if not in a hurry, see if we can pick up a dropped defuser
			if (!inahurry && !csslib::HasDefuser(bot->GetEntity()))
			{
				NBotSharedCollectItemTask::ItemCollectFilter<CCSSBot, CCSSNavArea> filter{ bot };
				CBaseEntity* defuser = nullptr;

				if (CBotSharedCollectItemsTask<CCSSBot, CCSSBotPathCost>::IsPossible(bot, &filter, "item_defuser", &defuser))
				{
					auto task = new CBotSharedCollectItemsTask<CCSSBot, CCSSBotPathCost>(bot, defuser, NBotSharedCollectItemTask::COLLECT_WALK_OVER);
					auto func = std::bind(CollectItemsValidationHasDefuser, std::placeholders::_1, std::placeholders::_2);
					task->SetValidationFunction(func);
					return task;
				}
			}

			// CTs don't know where is the bomb is.
			if (!csmod->IsTheBombKnownByCTs())
			{
				return new CCSSBotSearchForArmedBombTask;
			}

			return new CCSSBotDefuseBombTask;
		}

		if (myteam == counterstrikesource::CSSTeam::TERRORIST)
		{
			return new CCSSBotDefendPlantedBombTask;
		}
	}
	
	if (csslib::MapHasBombTargets())
	{
		if (myteam == counterstrikesource::CSSTeam::COUNTERTERRORIST)
		{
			if (csmod->GetCSSModSettings()->RollDefendChance())
			{
				return new CCSSBotGuardBombSiteTask;
			}
		}

		if (myteam == counterstrikesource::CSSTeam::TERRORIST)
		{
			CBaseEntity* droppedc4 = csslib::GetDroppedC4();

			if (droppedc4)
			{
				NBotSharedCollectItemTask::ItemCollectFilter<CCSSBot, CCSSNavArea> filter{ bot };
				CBaseEntity* c4weapon = nullptr;

				if (CBotSharedCollectItemsTask<CCSSBot, CCSSBotPathCost>::IsPossible(bot, &filter, "weapon_c4", &c4weapon))
				{
					auto task = new CBotSharedCollectItemsTask<CCSSBot, CCSSBotPathCost>(bot, c4weapon, NBotSharedCollectItemTask::COLLECT_WALK_OVER);
					auto func = std::bind(CollectItemsValidationHasC4, std::placeholders::_1, std::placeholders::_2);
					task->SetValidationFunction(func);
					return task;
				}
			}

			CBaseEntity* c4carrier = csslib::GetC4Carrier();
			
			if (c4carrier && CBaseBot::s_botrng.GetRandomChance(bot->GetDifficultyProfile()->GetTeamwork()))
			{
				return new CCSSBotFollowBombCarrierTask(bot, c4carrier);
			}
		}
	}

	// handle hostage rescue behavior
	if (csslib::GetNumberOfRemainingHostages() > 0)
	{
		if (myteam == counterstrikesource::CSSTeam::COUNTERTERRORIST)
		{
			if (csslib::IsEscortingHostages(bot->GetEntity()))
			{
				return new CCSSBotDeliverHostageTask;
			}

			CBaseEntity* hostage = csslib::GetRandomHostageToRescue();

			if (hostage)
			{
				return new CCSSBotRescueHostageTask(hostage);
			}
		}

		if (myteam == counterstrikesource::CSSTeam::TERRORIST)
		{
			if (csmod->AreTheHostagesBeingRescued())
			{
				return new CCSSBotGuardRescueZoneTask;
			}

			// cap rush chance to 50%
			const int rushchance = std::min(bot->GetDifficultyProfile()->GetAggressiveness(), 50);
			if (CBaseBot::s_botrng.GetRandomChance(rushchance))
			{
				CBaseEntity* rescuezone = UtilHelpers::GetRandomEntityOfClassname("func_hostage_rescue");

				if (rescuezone)
				{
					Vector pos = trace::getground(UtilHelpers::getWorldSpaceCenter(rescuezone));
					return new CBotSharedGoToPositionTask<CCSSBot, CCSSBotPathCost>(bot, pos, "RushRescueZone");
				}
			}

			// random chance to defend
			if (csmod->GetCSSModSettings()->RollDefendChance())
			{
				CBaseEntity* hostage = csslib::GetRandomHostageToRescue();

				if (hostage)
				{
					return new CCSSBotGuardHostageTask(hostage);
				}
			}
		}
	}

	// if nothing to do, roam randomly
	return new CCSSBotRoamTask(bot);
}
