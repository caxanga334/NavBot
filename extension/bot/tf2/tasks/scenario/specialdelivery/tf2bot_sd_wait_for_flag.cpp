#include <extension.h>
#include <util/helpers.h>
#include <mods/tf2/tf2lib.h>
#include <bot/tf2/tf2bot.h>
#include <bot/interfaces/path/meshnavigator.h>
#include <entities/tf2/tf_entities.h>
#include <sdkports/sdk_ehandle.h>
#include <bot/tasks_shared/bot_shared_pursue_and_destroy.h>
#include <bot/tasks_shared/bot_shared_escort_entity.h>
#include <bot/tasks_shared/bot_shared_roam.h>
#include "tf2bot_sd_deliver_flag.h"
#include "tf2bot_sd_wait_for_flag.h"

TaskResult<CTF2Bot> CTF2BotSDWaitForFlagTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	CBaseEntity* flag = tf2lib::sd::GetSpecialDeliveryFlag();

	if (!flag)
	{
		return Done("No flag!");
	}

	m_flag = flag;

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotSDWaitForFlagTask::OnTaskUpdate(CTF2Bot* bot)
{
	CBaseEntity* ent = m_flag.Get();

	if (!ent)
	{
		return Done("No flag!");
	}

	tfentities::HCaptureFlag flag(ent);

	if (flag.IsStolen())
	{
		CBaseEntity* owner = flag.GetOwnerEntity();

		if (owner == bot->GetEntity())
		{
			return SwitchTo(new CTF2BotSDDeliverFlag, "I have the australium, going to deliver it!");
		}

		if (tf2lib::GetEntityTFTeam(owner) != bot->GetMyTFTeam())
		{
			// Enemy is carrying the australium, go after them
			return SwitchTo(new CBotSharedPursueAndDestroyTask<CTF2Bot, CTF2BotPathCost>(bot, owner, 120.0f, true), "Pursing australium carrier!");
		}
		else
		{
			// 20% roam, 80% escort teammate
			if (CBaseBot::s_botrng.GetRandomInt<int>(1, 5) == 5)
			{
				return SwitchTo(new CBotSharedRoamTask<CTF2Bot, CTF2BotPathCost>(bot), "Teammate has the australium, roaming around!");
			}
			else
			{
				// Escort teammate carrying the australium
				return SwitchTo(new CBotSharedEscortEntityTask<CTF2Bot, CTF2BotPathCost>(bot, owner), "Escorting teammate carrying the australium!");
			}
		}
	}

	if (flag.IsDropped() || flag.IsHome())
	{
		if (m_repathTimer.IsElapsed())
		{
			m_repathTimer.Start(1.0f);
			Vector goal = flag.GetAbsOrigin();

			CTF2BotPathCost cost(bot);
			m_nav.ComputePathToPosition(bot, goal, cost, 0.0f, true);
		}

		m_nav.Update(bot);
	}

	return Continue();
}
