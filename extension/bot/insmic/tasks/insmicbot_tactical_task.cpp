#include NAVBOT_PCH_FILE
#include <bot/tasks_shared/bot_shared_prereq_destroy_ent.h>
#include <bot/tasks_shared/bot_shared_prereq_move_to_pos.h>
#include <bot/tasks_shared/bot_shared_prereq_use_ent.h>
#include <bot/tasks_shared/bot_shared_prereq_wait.h>
#include <bot/tasks_shared/bot_shared_pursue_and_destroy.h>
#include <bot/interfaces/behavior_utils.h>
#include <bot/insmic/insmicbot.h>
#include "insmicbot_scenario_task.h"
#include "insmicbot_tactical_task.h"

CInsMICBotTacticalTask::CInsMICBotTacticalTask()
{
}

AITask<CInsMICBot>* CInsMICBotTacticalTask::InitialNextTask(CInsMICBot* bot)
{
	return new CInsMICBotScenarioTask;
}

TaskResult<CInsMICBot> CInsMICBotTacticalTask::OnTaskUpdate(CInsMICBot* bot)
{
	return Continue();
}

TaskEventResponseResult<CInsMICBot> CInsMICBotTacticalTask::OnNavAreaChanged(CInsMICBot* bot, CNavArea* oldArea, CNavArea* newArea)
{
	BOTBEHAVIOR_IMPLEMENT_PREREQUISITE_CHECK(CInsMICBot, CInsMICBotPathCost);

	return TryContinue();
}
