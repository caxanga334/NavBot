#include NAVBOT_PCH_FILE
#include <vector>
#include <utility>
#include <extension.h>
#include <util/librandom.h>
#include <bot/tf2/tf2bot.h>
#include <sdkports/sdk_ehandle.h>
#include <mods/tf2/tf2lib.h>
#include <bot/interfaces/path/meshnavigator.h>
#include <bot/tasks_shared/bot_shared_escort_entity.h>
#include "tf2bot_pd_move_to_capture_zone_task.h"
#include "tf2bot_pd_collect_task.h"
#include "tf2bot_pd_search_and_destroy_task.h"
#include "tf2bot_pd_monitor_task.h"

TaskResult<CTF2Bot> CTF2BotPDMonitorTask::OnTaskUpdate(CTF2Bot* bot)
{
	CBaseEntity* capzone = nullptr;

	if (CTF2BotPDMoveToCaptureZoneTask::IsPossible(bot, &capzone))
	{
		return PauseFor(new CTF2BotPDMoveToCaptureZoneTask(capzone), "Delivering my points!");
	}

	std::vector<CHandle<CBaseEntity>> pointsToCollect;

	if (CTF2BotPDCollectTask::IsPossible(bot, pointsToCollect))
	{
		return PauseFor(new CTF2BotPDCollectTask(std::move(pointsToCollect)), "Collecting nearby dropped points!");
	}

	if (bot->GetItem() == nullptr)
	{
		const int teamwork = bot->GetDifficultyProfile()->GetTeamwork() - 20;

		if (teamwork > 0 && randomgen->GetRandomInt<int>(1, 100) <= teamwork)
		{
			CBaseEntity* leader = tf2lib::pd::GetTeamLeader(bot->GetMyTFTeam());

			if (leader)
			{
				return PauseFor(new CBotSharedEscortEntityTask<CTF2Bot, CTF2BotPathCost>(bot, leader, randomgen->GetRandomReal<float>(30.0f, 90.0f), 400.0f), "Following my team's leader!");
			}
		}
	}

	return PauseFor(new CTF2BotPDSearchAndDestroyTask, "Searching for enemy players to destroy!");
}
