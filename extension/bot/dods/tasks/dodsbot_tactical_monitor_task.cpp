#include <extension.h>
#include <bot/dods/dodsbot.h>
#include "dodsbot_scenario_monitor_task.h"
#include "dodsbot_tactical_monitor_task.h"

AITask<CDoDSBot>* CDoDSBotTacticalMonitorTask::InitialNextTask(CDoDSBot* bot)
{
    return new CDoDSBotScenarioMonitorTask;
}

TaskResult<CDoDSBot> CDoDSBotTacticalMonitorTask::OnTaskUpdate(CDoDSBot* bot)
{
    return Continue();
}
