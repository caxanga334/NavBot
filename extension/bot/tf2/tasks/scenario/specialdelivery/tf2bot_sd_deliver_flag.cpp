#include <cstring>

#include <extension.h>
#include <util/helpers.h>
#include <util/entprops.h>
#include <mods/tf2/tf2lib.h>
#include <bot/tf2/tf2bot.h>
#include <bot/interfaces/path/meshnavigator.h>
#include "tf2bot_sd_deliver_flag.h"

TaskResult<CTF2Bot> CTF2BotSDDeliverFlag::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	CBaseEntity* capturezone = tf2lib::sd::GetSpecialDeliveryCaptureZone();

	if (!capturezone)
	{
		return Done("No capture zone!");
	}

	m_goal = UtilHelpers::getWorldSpaceCenter(capturezone);
	m_capzone = capturezone;

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotSDDeliverFlag::OnTaskUpdate(CTF2Bot* bot)
{
	if (bot->GetItem() == nullptr)
	{
		return Done("Flag delivered!");
	}

	CBaseEntity* capzone = m_capzone.Get();

	if (!capzone)
	{
		capzone = tf2lib::sd::GetSpecialDeliveryCaptureZone();

		if (!capzone)
		{
			return Done("No capture zone!");
		}

		m_goal = UtilHelpers::getWorldSpaceCenter(capzone);
		m_capzone = capzone;
	}

	edict_t* groundent = bot->GetGroundEntity();

	if (groundent && !bot->GetMovementInterface()->IsUsingElevator())
	{
		const char* classname = gamehelpers->GetEntityClassname(groundent);

		// when riding the elevator on sd_doomsday, don't use navigation
		if (std::strcmp(classname, "func_tracktrain") == 0)
		{
			bool disabled = false;
			entprops->GetEntPropBool(gamehelpers->EntityToBCompatRef(capzone), Prop_Data, "m_bDisabled", disabled);

			if (disabled) // while the cap zone is disabled, move towards the elevator center
			{
				Vector goal = UtilHelpers::getWorldSpaceCenter(groundent);
				bot->GetMovementInterface()->MoveTowards(goal, 500);
			}
			else
			{
				bot->GetMovementInterface()->MoveTowards(m_goal, 500);
			}
			
			bot->GetMovementInterface()->ClearStuckStatus("Special Delivery: Riding Elevator!");
			return Continue();
		}
	}

	if (m_repathTimer.IsElapsed())
	{
		m_repathTimer.Start(1.0f);

		CTF2BotPathCost cost(bot);
		m_nav.ComputePathToPosition(bot, m_goal, cost);
	}

	m_nav.Update(bot);

	return Continue();
}
