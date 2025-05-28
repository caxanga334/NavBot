#include <extension.h>
#include <util/entprops.h>
#include <bot/dods/dodsbot.h>
#include <mods/dods/dayofdefeatsourcemod.h>
#include "dodsbot_scenario_monitor_task.h"
#include "dodsbot_tactical_monitor_task.h"
#include <bot/tasks_shared/bot_shared_roam.h>
#include "scenario/dodsbot_attack_control_point_task.h"
#include "scenario/dodsbot_defuse_bomb_task.h"
#include <bot/tasks_shared/bot_shared_defend_spot.h>

AITask<CDoDSBot>* CDoDSBotScenarioMonitorTask::InitialNextTask(CDoDSBot* bot)
{
	CDayOfDefeatSourceMod* mod = CDayOfDefeatSourceMod::GetDODMod();
	const int defrate = mod->GetModSettings()->GetDefendRate();

	// Random Defend chance
	if (CBaseBot::s_botrng.GetRandomInt<int>(1, 100) <= defrate)
	{
		CBaseEntity* defuse = nullptr;
		const CDayOfDefeatSourceMod::DoDControlPoint* defend = nullptr;
		
		FindControlPointToDefend(bot, &defuse, &defend);

		if (defuse)
		{
			return new CDoDSBotDefuseBombTask(defuse);
		}

		if (defend)
		{
			return new CBotSharedDefendSpotTask<CDoDSBot, CDoDSBotPathCost>(bot, UtilHelpers::getWorldSpaceCenter(defend->point.Get()), 
				CBaseBot::s_botrng.GetRandomReal<float>(30.0f, 60.0f), true);
		}
	}

	const CDayOfDefeatSourceMod::DoDControlPoint* pointToAttack = nullptr;

	if (CDoDSBotAttackControlPointTask::IsPossible(bot, &pointToAttack))
	{
		return new CDoDSBotAttackControlPointTask(pointToAttack);
	}

	return new CBotSharedRoamTask<CDoDSBot, CDoDSBotPathCost>(bot, 4096.0f, true);
}

TaskResult<CDoDSBot> CDoDSBotScenarioMonitorTask::OnTaskUpdate(CDoDSBot* bot)
{
	if (GetNextTask() == nullptr)
	{
		return SwitchTo(new CDoDSBotScenarioMonitorTask, "Restarting Scenario Monitor!");
	}

	return Continue();
}

TaskEventResponseResult<CDoDSBot> CDoDSBotScenarioMonitorTask::OnRoundStateChanged(CDoDSBot* bot)
{
	return TrySwitchTo(new CDoDSBotScenarioMonitorTask, PRIORITY_CRITICAL, "Round state changed, restart ScenarioMonitor!");
}

void CDoDSBotScenarioMonitorTask::FindControlPointToDefend(CDoDSBot* bot, CBaseEntity** defuse, const CDayOfDefeatSourceMod::DoDControlPoint** defend)
{
	CDayOfDefeatSourceMod* mod = CDayOfDefeatSourceMod::GetDODMod();
	std::vector<const CDayOfDefeatSourceMod::DoDControlPoint*> points;
	points.reserve(MAX_CONTROL_POINTS);
	mod->CollectControlPointsToDefend(bot->GetMyDoDTeam(), points);

	if (!points.empty())
	{
		const CDODObjectiveResource* objres = mod->GetDODObjectiveResource();

		// search for any points that contain bombs that needs to be defused
		for (const CDayOfDefeatSourceMod::DoDControlPoint* point : points)
		{
			// IsBombPlanted was commented out because it's always false, m_iState will tell us if there is an active bomb
			if (objres->GetNumBombsRequired(point->index) > 0 /* && objres->IsBombPlanted(point->index) */)
			{
				for (auto& handle : point->bomb_targets)
				{
					CBaseEntity* target = handle.Get();

					if (target)
					{
						int state = 0;
						entprops->GetEntProp(handle.GetEntryIndex(), Prop_Send, "m_iState", state);

						if (state == static_cast<int>(dayofdefeatsource::DoDBombTargetState::BOMB_TARGET_ARMED))
						{
							*defuse = target;
							return;
						}
					}
				}
			}
		}

		const CDayOfDefeatSourceMod::DoDControlPoint* controlpoint = points[CBaseBot::s_botrng.GetRandomInt<std::size_t>(0U, points.size() - 1U)];
		*defend = controlpoint;
	}
}
