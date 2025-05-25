#include <string_view>
#include <algorithm>
#include <extension.h>
#include <util/helpers.h>
#include <util/entprops.h>
#include <sdkports/sdk_ehandle.h>
#include <bot/tf2/tf2bot.h>
#include <mods/tf2/teamfortress2mod.h>
#include <mods/tf2/tf2lib.h>
#include <entities/tf2/tf_entities.h>
#include <bot/interfaces/path/chasenavigator.h>
#include <bot/tasks_shared/bot_shared_escort_entity.h>
#include <bot/tf2/tasks/scenario/tf2bot_map_ctf.h>
#include "tf2bot_rd_steal_enemy_points_task.h"

bool CTF2BotRDStealEnemyPointsTask::IsPossible(CTF2Bot* bot, CBaseEntity** flag)
{
	CBaseEntity* out = nullptr;

	UtilHelpers::ForEachEntityOfClassname("item_teamflag", [&bot, &out](int index, edict_t* edict, CBaseEntity* entity) {
		if (entity)
		{
			tfentities::HCaptureFlag cf(entity);

			if (!cf.IsDisabled() && cf.GetTFTeam() != bot->GetMyTFTeam() && (cf.IsHome() || cf.IsDropped()))
			{
				out = entity;
				return false;
			}
		}

		return true;
	});

	if (!out)
	{
		return false;
	}

	*flag = out;
	return true;
}

CTF2BotRDStealEnemyPointsTask::CTF2BotRDStealEnemyPointsTask(CBaseEntity* flag) :
	m_flag(flag)
{
	tfentities::HCaptureFlag cf(flag);
	m_homepos = cf.GetAbsOrigin();
	m_washome = cf.IsHome();
}

TaskResult<CTF2Bot> CTF2BotRDStealEnemyPointsTask::OnTaskUpdate(CTF2Bot* bot)
{
	CBaseEntity* ent = m_flag.Get();

	if (!ent)
	{
		return Done("No flag");
	}

	tfentities::HCaptureFlag flag(ent);

	if (flag.IsStolen())
	{
		CBaseEntity* owner = flag.GetOwnerEntity();

		if (owner == bot->GetEntity())
		{
			int logic = UtilHelpers::FindEntityByClassname(INVALID_EHANDLE_INDEX, "tf_logic_robot_destruction");

			if (m_washome && logic != INVALID_EHANDLE_INDEX)
			{
				int teampoints = 0;

				if (bot->GetMyTFTeam() == TeamFortress2::TFTeam::TFTeam_Red)
				{
					entprops->GetEntProp(logic, Prop_Send, "m_nBlueScore", teampoints);
				}
				else
				{
					entprops->GetEntProp(logic, Prop_Send, "m_nRedScore", teampoints);
				}

				// Keep collecting points until the flag is over 100 points or the enemy team doesn't have any points left
				if (teampoints > 0 || flag.GetPointValue() < 100)
				{
					if (m_repathtimer.IsElapsed())
					{
						m_repathtimer.Start(2.0f);
						CTF2BotPathCost cost(bot, SAFEST_ROUTE);
						m_nav.ComputePathToPosition(bot, m_homepos, cost);
					}

					m_nav.Update(bot);

					return Continue();
				}
			}

			return SwitchTo(new CTF2BotCTFDeliverFlagTask, "I have the reactor core, delivering it!");
		}
		else
		{
			int teamwork = std::clamp(bot->GetDifficultyProfile()->GetTeamwork(), 0, 50);

			if (CBaseBot::s_botrng.GetRandomInt<int>(1, 100) >= teamwork)
			{
				return SwitchTo(new CBotSharedEscortEntityTask<CTF2Bot, CTF2BotPathCost>(bot, owner, CBaseBot::s_botrng.GetRandomReal<float>(15.0f, 35.0f), 400.0f), 
					"Escorting reactor core carrier!");
			}
			else
			{
				return Done("Someone else got the flag before me.");
			}
		}
	}

	if (m_repathtimer.IsElapsed())
	{
		m_repathtimer.Start(2.0f);
		const Vector& pos = UtilHelpers::getEntityOrigin(ent);
		CTF2BotPathCost cost(bot, SAFEST_ROUTE);
		m_nav.ComputePathToPosition(bot, pos, cost);
	}
	
	m_nav.Update(bot);

	return Continue();
}
