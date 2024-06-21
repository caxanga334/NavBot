#include <limits>

#include <extension.h>
#include <util/helpers.h>
#include <util/librandom.h>
#include <bot/tf2/tf2bot.h>
#include <mods/tf2/tf2lib.h>
#include "tf2bot_medic_retreat_task.h"

#undef max
#undef min
#undef clamp

TaskResult<CTF2Bot> CTF2BotMedicRetreatTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	m_goal = GetRetreatPosition(bot);

	CTF2BotPathCost cost(bot);
	if (!m_nav.ComputePathToPosition(bot, m_goal, cost, 0.0f, true))
	{
		return Done("Failed to find a path to home position!");
	}

	m_repathtimer.Start(randomgen->GetRandomReal<float>(1.0f, 2.0f));
	return Continue();
}

TaskResult<CTF2Bot> CTF2BotMedicRetreatTask::OnTaskUpdate(CTF2Bot* bot)
{
	if (m_repathtimer.IsElapsed())
	{
		m_repathtimer.Start(randomgen->GetRandomReal<float>(1.0f, 2.0f));
		CTF2BotPathCost cost(bot);
		m_nav.ComputePathToPosition(bot, m_goal, cost, 0.0f, true);
	}

	if (bot->GetRangeTo(m_goal) >= home_range())
	{
		m_nav.Update(bot);
	}
	else
	{
		return Done("Home reached!");
	}

	bool visibleteammate = false;

	bot->GetSensorInterface()->ForEveryKnownEntity([&bot, &visibleteammate](const CKnownEntity* known) {
		if (!visibleteammate && !known->IsObsolete() && known->IsPlayer() && known->IsVisibleNow())
		{
			if (tf2lib::GetEntityTFTeam(known->GetIndex()) == bot->GetMyTFTeam())
			{
				visibleteammate = true;
			}
		}
	});

	if (visibleteammate)
	{
		return Done("Found teammate!");
	}

	return Continue();
}

// Retreat to the nearest alive teammate or to my spawn point if none is found
Vector CTF2BotMedicRetreatTask::GetRetreatPosition(CTF2Bot* me) const
{
	CBaseEntity* nearestTeammate = nullptr;
	float t = std::numeric_limits<float>::max();
	auto myteam = me->GetMyTFTeam();
	Vector origin = me->GetAbsOrigin();
	Vector goal;

	UtilHelpers::ForEachPlayer([&t, &nearestTeammate, &myteam, &origin, &goal](int client, edict_t* entity, SourceMod::IGamePlayer* player) {
		if (player->IsInGame() && UtilHelpers::IsPlayerAlive(client) && tf2lib::GetEntityTFTeam(client) == myteam)
		{
			CBaseExtPlayer them(entity);
			float d = (them.GetAbsOrigin() - origin).Length();

			if (d < t)
			{
				t = d;
				nearestTeammate = them.GetEntity();
				goal = them.GetAbsOrigin();
			}
		}
	});

	if (nearestTeammate != nullptr)
	{
		return goal;
	}

	return me->GetHomePos();
}
