#include NAVBOT_PCH_FILE
#include <bot/tasks_shared/bot_shared_prereq_tasks.h>
#include <bot/tasks_shared/bot_shared_pursue_and_destroy.h>
#include <bot/tasks_shared/bot_shared_plugin_command_tasks.h>
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
	TaskEventResponseResult<CInsMICBot> result = botprereqtasks::ImplementPrereqCheck<CInsMICBot, CInsMICBotTacticalTask, CNavArea, CInsMICBotPathCost>(this, bot, newArea);

	if (result.IsRequestingChange())
	{
		return result;
	}

	return TryContinue();
}

TaskEventResponseResult<CInsMICBot> CInsMICBotTacticalTask::OnPluginCommand(CInsMICBot* bot, IEventListener::PluginCommandTypes type, const IEventListener::PluginCommandData& data)
{
	return plugincommandtask::ImplementPluginCommandTasks<CInsMICBotTacticalTask, CInsMICBot, CInsMICBotPathCost>(this, bot, type, data);
}
