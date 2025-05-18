#include <extension.h>
#include <util/helpers.h>
#include <util/entprops.h>
#include <bot/tf2/tf2bot.h>
#include <mods/tf2/teamfortress2mod.h>
#include <mods/tf2/tf2lib.h>
#include <bot/interfaces/path/chasenavigator.h>
#include "tf2bot_wait_for_jack_task.h"
#include "tf2bot_pick_jack_task.h"
#include "tf2bot_passtime_goal_task.h"
#include "tf2bot_seek_and_destroy_jack_carrier_task.h"
#include <bot/tasks_shared/bot_shared_escort_entity.h>
#include <bot/tasks_shared/bot_shared_roam.h>
#include "tf2bot_passtime_monitor_task.h"

TaskResult<CTF2Bot> CTF2BotPassTimeMonitorTask::OnTaskUpdate(CTF2Bot* bot)
{
	if (bot->IsCarryingThePassTimeJack())
	{
		return PauseFor(new CTF2BotPassTimeGoalTask, "I have the jack! Going to score it!");
	}

	if (!tf2lib::passtime::IsJackActive())
	{
		return PauseFor(new CTF2BotWaitForJackTask, "Moving to the jack spawn position!");
	}

	CBaseEntity* jack = tf2lib::passtime::GetJack();

	if (jack)
	{
		CBaseEntity* carrier = tf2lib::passtime::GetJackCarrier(jack);

		if (carrier)
		{
			if (tf2lib::GetEntityTFTeam(carrier) != bot->GetMyTFTeam())
			{
				return PauseFor(new CTF2BotSeekAndDestroyJackCarrierTask, "Going after the jack carrier!");
			}
			else if (bot->GetDifficultyProfile()->GetTeamwork() >= 25 && CBaseBot::s_botrng.GetRandomInt<int>(0, 2) > 0)
			{
				return PauseFor(new CBotSharedEscortEntityTask<CTF2Bot, CTF2BotPathCost>(bot, carrier, 15.0f, 500.0f), "Escorting teammate carrying the jack!");
			}
		}
		else
		{
			return PauseFor(new CTF2BotPickJackTask, "Going after the jack!");
		}
	}

	return PauseFor(new CBotSharedRoamTask<CTF2Bot, CTF2BotPathCost>(bot, 10000.0f, true), "Nothing to do, roaming!");
}
