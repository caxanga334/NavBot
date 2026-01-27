#include NAVBOT_PCH_FILE
#include <bot/insmic/insmicbot.h>
#include <mods/insmic/insmicmod.h>
#include <bot/tasks_shared/bot_shared_roam.h>
#include "insmicbot_checkpoint_capture_objective_task.h"
#include "insmicbot_checkpoint_patrol_objective_task.h"
#include "insmicbot_checkpoint_monitor_task.h"

TaskResult<CInsMICBot> CInsMICBotCheckPointMonitorTask::OnTaskUpdate(CInsMICBot* bot)
{
	insmic::InsMICTeam myteam = bot->GetMyInsurgencyTeam();
	CInsMICMod* mod = CInsMICMod::GetInsurgencyMod();
	const CModSettings* settings = mod->GetModSettings();
	CBaseEntity* attack = mod->GetObjectiveToCapture(myteam);
	CBaseEntity* defend = mod->GetObjectiveToDefend(myteam);

	if (attack && defend)
	{
		if (settings->RollDefendChance())
		{
			return PauseFor(new CInsMICBotCheckPointPatrolObjectiveTask(defend), "Patrolling owned objective!");
		}

		return PauseFor(new CInsMICBotCheckPointCaptureObjectiveTask(attack), "Going to capture an objective!");
	}

	if (attack)
	{
		return PauseFor(new CInsMICBotCheckPointCaptureObjectiveTask(attack), "Going to capture an objective!");
	}

	if (defend)
	{
		return PauseFor(new CInsMICBotCheckPointPatrolObjectiveTask(defend), "Patrolling owned objective!");
	}

	return PauseFor(new CBotSharedRoamTask<CInsMICBot, CInsMICBotPathCost>(bot, 10000.0f, true), "Nothing to do, roaming!");
}
