#include NAVBOT_PCH_FILE
#include <extension.h>
#include <util/helpers.h>
#include <util/entprops.h>
#include <bot/tf2/tf2bot.h>
#include <mods/tf2/teamfortress2mod.h>
#include <mods/tf2/tf2lib.h>
#include <bot/interfaces/path/chasenavigator.h>
#include "tf2bot_rd_collect_points_task.h"
#include "tf2bot_rd_destroy_robots_task.h"
#include "tf2bot_rd_defend_core_task.h"
#include "tf2bot_rd_steal_enemy_points_task.h"
#include <bot/tasks_shared/bot_shared_roam.h>
#include "tf2bot_rd_monitor_task.h"

TaskResult<CTF2Bot> CTF2BotRobotDestructionMonitorTask::OnTaskUpdate(CTF2Bot* bot)
{
	std::vector<CHandle<CBaseEntity>> handles;

	if (CTF2BotRDDestroyRobotsTask::IsPossible(bot, handles))
	{
		return PauseFor(new CTF2BotRDDestroyRobotsTask(std::move(handles)), "Going to destroy enemy robots!");
	}

	if (CTF2BotRDCollectPointsTask::IsPossible(bot, handles))
	{
		return PauseFor(new CTF2BotRDCollectPointsTask(std::move(handles)), "Collecting dropped points!");
	}

	CBaseEntity* mycore = nullptr;

	if (CTF2BotRDDefendCoreTask::IsPossible(bot, &mycore))
	{
		return PauseFor(new CTF2BotRDDefendCoreTask(mycore), "Defending stolen core!");
	}

	CBaseEntity* enemyCore = nullptr;

	if (CTF2BotRDStealEnemyPointsTask::IsPossible(bot, &enemyCore))
	{
		return PauseFor(new CTF2BotRDStealEnemyPointsTask(enemyCore), "Going to steal points from the enemy.");
	}

	return PauseFor(new CBotSharedRoamTask<CTF2Bot, CTF2BotPathCost>(bot, 8192.0f), "Nothing to do, roaming!");
}
