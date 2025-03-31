#include <extension.h>
#include <bot/dods/dodsbot.h>
#include "dodsbot_scenario_monitor_task.h"
#include "dodsbot_tactical_monitor_task.h"

TaskResult<CDoDSBot> CDoDSBotScenarioMonitorTask::OnTaskUpdate(CDoDSBot* bot)
{
	return Continue();
}
