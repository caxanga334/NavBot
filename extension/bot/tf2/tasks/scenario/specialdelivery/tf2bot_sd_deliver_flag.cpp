#include NAVBOT_PCH_FILE
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

	entprops->GetEntPropBool(m_capzone.GetEntryIndex(), Prop_Data, "m_bDisabled", m_capwasdisabled);

	CBaseEntity* flagzone = nullptr;
	auto functor = [&flagzone, &bot](int index, edict_t* edict, CBaseEntity* entity) {
		if (entity)
		{
			int teamNum = 0;
			entprops->GetEntProp(index, Prop_Data, "m_iTeamNum", teamNum);

			if (teamNum <= TEAM_SPECTATOR || teamNum == static_cast<int>(bot->GetMyTFTeam()))
			{
				bool disabled = false;
				entprops->GetEntPropBool(index, Prop_Data, "m_bDisabled", disabled);

				if (!disabled)
				{
					flagzone = entity;
					return false; // stop loop
				}
			}
		}

		return true;
	};

	UtilHelpers::ForEachEntityOfClassname("func_flagdetectionzone", functor);

	m_flagzone = flagzone;

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
		return Done("No capture zone!");
	}

	CBaseEntity* flagzone = m_flagzone.Get();

	bool disabled = false;
	entprops->GetEntPropBool(m_capzone.GetEntryIndex(), Prop_Data, "m_bDisabled", disabled);

	// capture zone is disabled, move to the 'elevator'
	if (disabled && flagzone)
	{
		m_goal = UtilHelpers::getWorldSpaceCenter(flagzone);
	}
	else
	{
		if (m_capwasdisabled)
		{
			m_capwasdisabled = false;
			m_repathTimer.Invalidate(); // force a repath
		}

		m_goal = UtilHelpers::getWorldSpaceCenter(capzone);
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
