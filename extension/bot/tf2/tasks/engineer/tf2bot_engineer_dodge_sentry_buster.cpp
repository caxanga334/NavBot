#include <cstring>

#include <extension.h>
#include <util/helpers.h>
#include <mods/tf2/tf2lib.h>
#include <bot/tf2/tf2bot.h>
#include <util/librandom.h>
#include <util/entprops.h>
#include <entities/tf2/tf_entities.h>
#include <mods/tf2/teamfortress2mod.h>
#include <bot/tf2/tf2bot_utils.h>
#include <mods/tf2/nav/tfnavmesh.h>
#include <mods/tf2/nav/tfnavarea.h>
#include <mods/tf2/nav/tfnav_waypoint.h>
#include <navmesh/nav_pathfind.h>
#include <bot/bot_shared_utils.h>
#include <sdkports/sdk_ehandle.h>
#include "tf2bot_engineer_move_object.h"
#include "tf2bot_engineer_dodge_sentry_buster.h"

bool CTF2BotEngineerMvMDodgeSentryBusterTask::IsPossible(CTF2Bot* bot, CBaseEntity** sentryBuster)
{
	CBaseEntity* mySentry = bot->GetMySentryGun();

	if (!mySentry || tf2lib::IsBuildingSapped(mySentry))
	{
		return false;
	}

	CBaseEntity* buster = nullptr;
	auto func = [&bot, &buster](int client, edict_t* entity, SourceMod::IGamePlayer* player) -> void {
		if (!buster && player->IsInGame() && !player->GetPlayerInfo()->IsDead())
		{
			if (client == bot->GetIndex())
			{
				return;
			}

			int teamnum = 0;
			entprops->GetEntProp(client, Prop_Send, "m_iTeamNum", teamnum);

			if (teamnum != static_cast<int>(TeamFortress2::TFTeam::TFTeam_Blue))
			{
				return;
			}

			const char* model = STRING(entity->GetIServerEntity()->GetModelName());

			if (model && std::strcmp(model, "models/bots/demo/bot_sentry_buster.mdl") == 0)
			{
				buster = UtilHelpers::EdictToBaseEntity(entity);
			}
		}
	};

	UtilHelpers::ForEachPlayer(func);

	if (!buster)
	{
		return false;
	}

	const Vector& v1 = UtilHelpers::getEntityOrigin(mySentry);
	const Vector& v2 = UtilHelpers::getEntityOrigin(buster);

	if ((v1 - v2).Length() > SENTRY_BUSTER_DANGER_RADIUS)
	{
		return false;
	}

	*sentryBuster = buster;
	return true;
}

CTF2BotEngineerMvMDodgeSentryBusterTask::CTF2BotEngineerMvMDodgeSentryBusterTask(CBaseEntity* sentryBuster) :
	m_goal(0.0f, 0.0f, 0.0f)
{
	m_sentryBuster = sentryBuster;
	m_hasSentry = false;
	m_detonating = false;
	m_detonationRange = SENTRY_BUSTER_DEFAULT_DETONATION_RANGE;
}

TaskResult<CTF2Bot> CTF2BotEngineerMvMDodgeSentryBusterTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
// EP1/Original lacks ConVarRef
#if SOURCE_ENGINE == SE_TF2
	ConVarRef tf_bot_suicide_bomb_range("tf_bot_suicide_bomb_range");

	if (tf_bot_suicide_bomb_range.IsValid())
	{
		m_detonationRange = (tf_bot_suicide_bomb_range.GetFloat() / 3.0f);
	}
#endif

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotEngineerMvMDodgeSentryBusterTask::OnTaskUpdate(CTF2Bot* bot)
{
	CBaseEntity* buster = m_sentryBuster.Get();

	if (!buster || tf2lib::GetEntityTFTeam(buster) != TeamFortress2::TFTeam::TFTeam_Blue || !UtilHelpers::IsEntityAlive(buster))
	{
		if (bot->IsCarryingObject())
		{
			CBaseEntity* mySentry = bot->GetMySentryGun();
			CTFNavArea* area = nullptr;
			CTFWaypoint* wpt = nullptr;

			if (tf2botutils::FindSpotToBuildSentryGun(bot, &wpt, &area))
			{
				if (wpt)
				{
					return SwitchTo(new CTF2BotEngineerMoveObjectTask(mySentry, wpt), "Redeplying my sentry gun!");
				}
				else
				{
					return SwitchTo(new CTF2BotEngineerMoveObjectTask(mySentry, area->GetCenter()), "Redeplying my sentry gun!");
				}
			}
			else
			{
				return SwitchTo(new CTF2BotEngineerMoveObjectTask(mySentry, bot->GetAbsOrigin()), "Redeplying my sentry gun!");
			}
		}
		else
		{
			return Done("Sentry buster is gone!");
		}
	}

	if (!m_hasSentry)
	{
		CBaseEntity* mySentry = bot->GetMySentryGun();

		if (!mySentry)
		{
			return Done("No sentry!");
		}

		m_goal = UtilHelpers::getEntityOrigin(mySentry);

		if (bot->GetRangeTo(mySentry) <= 110.0f)
		{
			bot->GetControlInterface()->AimAt(UtilHelpers::getWorldSpaceCenter(mySentry), IPlayerController::LOOK_CRITICAL, 1.0f, "Looking at sentry to pick it!");
			
			if (bot->GetControlInterface()->IsAimOnTarget())
			{
				bot->GetControlInterface()->PressSecondaryAttackButton();
			}
		}

		if (bot->IsCarryingObject())
		{
			m_repathTimer.IsElapsed();
			m_hasSentry = true;
		}
	}
	else if (!m_detonating)
	{
		// the buster's m_takedamage is set to DAMAGE_NO when detonating
		int takedamage = -1;
		entprops->GetEntProp(m_sentryBuster.GetEntryIndex(), Prop_Data, "m_takedamage", takedamage);

		if (takedamage == DAMAGE_NO || tf2lib::IsPlayerInCondition(buster, TeamFortress2::TFCond::TFCond_Taunting) || bot->GetRangeTo(buster) <= m_detonationRange)
		{
			m_detonating = true;
			m_repathTimer.IsElapsed();
			botsharedutils::FindCoverCollector collector(UtilHelpers::getEntityOrigin(buster), SENTRY_BUSTER_MIN_COVER_RADIUS, false, false, SENTRY_BUSTER_MAX_COVER_RADIUS, bot);
			collector.Execute();

			if (collector.IsCollectedAreasEmpty())
			{
				m_goal = bot->GetHomePos();
			}
			else
			{
				m_goal = collector.GetRandomCollectedArea()->GetCenter();
			}
		}
		else
		{
			// move towards the buster
			m_goal = UtilHelpers::getEntityOrigin(buster);
		}
	}

	if (m_repathTimer.IsElapsed())
	{
		m_repathTimer.Start(1.0f);
		CTF2BotPathCost cost(bot);
		m_nav.ComputePathToPosition(bot, m_goal, cost, 0.0f, true);
	}

	m_nav.Update(bot);

	return Continue();
}
