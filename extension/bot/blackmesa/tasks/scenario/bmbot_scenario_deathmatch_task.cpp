#include <extension.h>
#include <bot/blackmesa/bmbot.h>
#include "bmbot_roam_task.h"
#include "bmbot_scenario_deathmatch_task.h"

TaskResult<CBlackMesaBot> CBlackMesaBotScenarioDeathmatchTask::OnTaskUpdate(CBlackMesaBot* bot)
{
	if (bot->GetBehaviorInterface()->ShouldFreeRoam(bot) != ANSWER_NO)
	{
		return PauseFor(new CBlackMesaBotRoamTask, "Roaming!");
	}

	return Continue();
}
