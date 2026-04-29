#include NAVBOT_PCH_FILE
#include <mods/css/css_lib.h>
#include <mods/css/css_mod.h>
#include <bot/css/cssbot.h>
#include <bot/tasks_shared/bot_shared_attack_enemy.h>
#include "scenario/cssbot_plant_bomb_task.h"
#include "cssbot_scenario_task.h"

CCSSBotScenarioTask::CCSSBotScenarioTask()
{
}

AITask<CCSSBot>* CCSSBotScenarioTask::InitialNextTask(CCSSBot* bot)
{
	return SelectScenarioTask(bot);
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

AITask<CCSSBot>* CCSSBotScenarioTask::SelectScenarioTask(CCSSBot* bot) const
{
	Vector goal;

	if (CCSSBotPlantBombTask::IsPossible(bot, goal))
	{
		return new CCSSBotPlantBombTask(goal);
	}

	return nullptr;
}
