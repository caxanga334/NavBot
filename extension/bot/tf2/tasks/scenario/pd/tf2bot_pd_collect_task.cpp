#include NAVBOT_PCH_FILE
#include <vector>
#include <extension.h>
#include <util/helpers.h>
#include <util/entprops.h>
#include <sdkports/sdk_ehandle.h>
#include <mods/tf2/tf2lib.h>
#include <bot/tf2/tf2bot.h>
#include <bot/interfaces/path/meshnavigator.h>
#include "tf2bot_pd_collect_task.h"

CTF2BotPDCollectTask::CTF2BotPDCollectTask(std::vector<CHandle<CBaseEntity>>&& toCollect) :
	m_points(toCollect)
{
	m_index = 0U;
}

bool CTF2BotPDCollectTask::IsPossible(CTF2Bot* bot, std::vector<CHandle<CBaseEntity>>& points)
{

	std::vector<CHandle<CBaseEntity>> collect;
	collect.reserve(64);
	auto functor = [&bot, &collect](int index, edict_t* edict, CBaseEntity* entity) {
		if (entity)
		{
			bool disabled = false;
			entprops->GetEntPropBool(index, Prop_Send, "m_bDisabled", disabled);

			if (disabled)
			{
				return true;
			}

			int owner = INVALID_EHANDLE_INDEX;
			entprops->GetEntPropEnt(index, Prop_Send, "m_hOwnerEntity", owner);

			// owned flags are being carried by someone
			if (owner != INVALID_EHANDLE_INDEX)
			{
				return true;
			}

			float range = (bot->GetAbsOrigin() - UtilHelpers::getEntityOrigin(entity)).Length();

			// only collect nearby flags
			if (range > 1024.0f)
			{
				return true;
			}

			collect.emplace_back(entity);
		}

		return true;
	};

	UtilHelpers::ForEachEntityOfClassname("item_teamflag", functor);

	if (!collect.empty())
	{
		points.swap(collect);
		return true;
	}

	return false;
}

TaskResult<CTF2Bot> CTF2BotPDCollectTask::OnTaskUpdate(CTF2Bot* bot)
{
	CBaseEntity* flag = nullptr;

	while (true)
	{
		flag = m_points[m_index].Get();

		// null ent
		if (!flag)
		{
			if (++m_index >= m_points.size())
			{
				return Done("No more points to collect!");
			}

			continue;
		}

		break;
	}

	if (entprops->GetEntPropEnt(flag, Prop_Send, "m_hOwnerEntity") != nullptr)
	{
		m_points[m_index].Term(); // invalidate the handle, the code above will skip it
		m_repathtimer.Invalidate();
		return Continue();
	}

	const Vector& origin = UtilHelpers::getEntityOrigin(flag);

	if (m_repathtimer.IsElapsed())
	{
		m_repathtimer.Start(1.0f);
		CTF2BotPathCost cost(bot);
		
		if (!m_nav.ComputePathToPosition(bot, origin, cost, 0.0f, true))
		{
			m_points[m_index].Term(); // invalidate the handle, the code above will skip it
			m_repathtimer.Invalidate();
			return Continue();
		}
	}

	m_nav.Update(bot);

	return Continue();
}
