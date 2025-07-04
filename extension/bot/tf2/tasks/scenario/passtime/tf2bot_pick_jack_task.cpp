#include NAVBOT_PCH_FILE
#include <extension.h>
#include <util/helpers.h>
#include <util/entprops.h>
#include <bot/tf2/tf2bot.h>
#include <mods/tf2/teamfortress2mod.h>
#include <mods/tf2/tf2lib.h>
#include <bot/interfaces/path/meshnavigator.h>
#include <sdkports/sdk_ehandle.h>
#include <bot/tasks_shared/bot_shared_attack_enemy.h>
#include "tf2bot_seek_and_destroy_jack_carrier_task.h"
#include <bot/tasks_shared/bot_shared_escort_entity.h>
#include <bot/tasks_shared/bot_shared_roam.h>
#include "tf2bot_passtime_goal_task.h"
#include "tf2bot_pick_jack_task.h"

TaskResult<CTF2Bot> CTF2BotPickJackTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	m_jack = tf2lib::passtime::GetJack();

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotPickJackTask::OnTaskUpdate(CTF2Bot* bot)
{
	if (bot->IsCarryingThePassTimeJack())
	{
		return SwitchTo(new CTF2BotPassTimeGoalTask, "I have the jack!");
	}

	CBaseEntity* jack = m_jack.Get();
	
	if (!jack)
	{
		return Done("NULL jack entity!");
	}

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
		else
		{
			return PauseFor(new CBotSharedRoamTask<CTF2Bot, CTF2BotPathCost>(bot, 10000.0f, true), "Nothing to do, roaming!");
		}
	}

	const CKnownEntity* threat = bot->GetSensorInterface()->GetPrimaryKnownThreat(true);

	if (threat)
	{
		return PauseFor(new CBotSharedAttackEnemyTask<CTF2Bot, CTF2BotPathCost>(bot), "Attacking visible enemy!");
	}

	const Vector& origin = UtilHelpers::getEntityOrigin(jack);
	CTF2BotPathCost cost(bot);
	m_nav.Update(bot, origin, cost);

	return Continue();
}
