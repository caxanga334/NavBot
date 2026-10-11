#include NAVBOT_PCH_FILE
#include <extension.h>
#include <util/helpers.h>
#include <util/entprops.h>
#include <entities/tf2/tf_entities.h>
#include <mods/tf2/teamfortress2mod.h>
#include <mods/tf2/tf2lib.h>
#include <bot/tf2/tf2bot.h>
#include <bot/interfaces/path/meshnavigator.h>
#include "tf2bot_use_teleporter.h"

#ifdef EXT_VPROF_ENABLED
#include <tier0/vprof.h>
#endif // EXT_VPROF_ENABLED

CTF2BotUseTeleporterTask::CTF2BotUseTeleporterTask(CBaseEntity* teleporter) :
	m_teleporter(teleporter)
{
}

bool CTF2BotUseTeleporterTask::IsPossible(CTF2Bot* bot, CBaseEntity** teleporter)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("CTF2BotUseTeleporterTask::IsPossible", "NavBot");
#endif // EXT_VPROF_ENABLED

	CMeshNavigator* nav = bot->GetActiveNavigator();

	if (!nav || !nav->IsValid()) { return false; }

	// Don't use teleporters while carrying the flag
	if (bot->GetItem() != nullptr) { return false; }

	CBaseEntity* entrance = nullptr;
	CBaseEntity* exit = nullptr;
	

	auto func = [&entrance, &exit, &bot](int index, edict_t* edict, CBaseEntity* entity) {
		tfentities::HObjectTeleporter tele(entity);

		// GetPercentageConstructed check was removed, on MvM or gamemodes with instant build, it's always 0.0 causing the teleporter to be skipped.
		// The ready state check is more than enough to determine if the teleporter can be used
		if (tele.IsSapped() || tele.IsBeingCarried() || tele.IsPlacing() ||
			tele.GetMode() != TeamFortress2::TFObjectMode::TFObjectMode_Entrance ||
			tele.GetState() != TeamFortress2::TeleporterState::TELEPORTER_STATE_READY ||
			tele.GetTFTeam() != bot->GetMyTFTeam())
		{
			return true;
		}

		// Scouts only uses level 3 teleporters
		if (bot->GetDifficultyProfile()->GetTeamwork() > 30 && bot->GetMyClassType() == TeamFortress2::TFClassType::TFClass_Scout && tele.GetLevel() < 3)
		{
			return true;
		}

		const Vector& entranceOrigin = UtilHelpers::getEntityOrigin(entity);
		
		// too far
		if (bot->GetRangeTo(entranceOrigin) > 1000.0f)
		{
			return true;
		}

		CNavArea* entranceArea = TheNavMesh->GetNearestNavArea(entranceOrigin, 256.0f, false, true, bot->GetCurrentTeamIndex());

		// Teleporter is outside the nav mesh
		if (entranceArea == nullptr)
		{
			return true;
		}

		entrance = entity;
		exit = tf2lib::GetMatchingTeleporter(entity);
		return false; // stop on the first teleporter found
	};

	UtilHelpers::ForEachEntityOfClassname("obj_teleporter", func);

	if (!entrance || !exit) { return false; }

	Vector origin = bot->GetAbsOrigin();
	const Vector& goal = nav->GetPathDestination();
	float distBotToGoal = (origin - goal).Length();

	if (distBotToGoal <= 512.0f)
	{
		return false; // already close to the goal
	}

	/*
	* Simple distance check for speed reasons.
	* The ideal solution would be comparing the path length between the bot and the teleporter exit.
	*/

	CNavArea* exitArea = TheNavMesh->GetNearestNavArea(UtilHelpers::getEntityOrigin(exit), 256.0f, false, true, bot->GetCurrentTeamIndex());

	// Exit must in in the nav mesh
	if (exitArea == nullptr)
	{
		return false;
	}

	const float pathDistance = nav->GetTravelDistance();
	CTF2BotPathCost cost(bot, RouteType::FASTEST_ROUTE);
	float teleToGoal = NavAreaTravelDistance(exitArea, nav->GetLastSegment()->area, cost);
	
	// If the travel distance is negative, no path to the goal exists from the teleporter exit
	if (teleToGoal < 0.0f)
	{
		return false;
	}
	
	teleToGoal += (bot->GetRangeTo(entrance) * 0.5f); // add half the distance to the entrance

	// if the travel distance from the tele exit to my goal is smaller than the current path distance, take the teleporter
	if (teleToGoal < pathDistance)
	{
		*teleporter = entrance;
		return true;
	}

	return false;
}

TaskResult<CTF2Bot> CTF2BotUseTeleporterTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	m_timeout.Start(20.0f);
	return Continue();
}

TaskResult<CTF2Bot> CTF2BotUseTeleporterTask::OnTaskUpdate(CTF2Bot* bot)
{
	if (m_timeout.IsElapsed())
	{
		return Done("Timeout timer elapsed!");
	}

	CBaseEntity* pEntity = m_teleporter.Get();

	if (!pEntity)
	{
		return Done("Teleporter is NULL!");
	}

	CBaseEntity* pExit = tf2lib::GetMatchingTeleporter(pEntity);

	if (!pExit)
	{
		return Done("No Teleporter Exit!");
	}

	CBaseEntity* pGround = bot->GetGroundEntity();

	if (pGround == pExit)
	{
		return Done("I've teleported!");
	}

	if (bot->GetRangeTo(pExit) <= 128.0f)
	{
		return Done("I've teleported!");
	}

	tfentities::HObjectTeleporter teleporter(pEntity);

	if (teleporter.IsSapped())
	{
		return Done("Teleporter is sapped!");
	}

	TeamFortress2::TeleporterState state = teleporter.GetState();

	if (state == TeamFortress2::TeleporterState::TELEPORTER_STATE_BUILDING || state == TeamFortress2::TeleporterState::TELEPORTER_STATE_IDLE)
	{
		return Done("Teleporter is no longer available!");
	}

	if (state != TeamFortress2::TeleporterState::TELEPORTER_STATE_READY)
	{
		// true if the teleporter is sending a player
		const bool isSending = state == TeamFortress2::TeleporterState::TELEPORTER_STATE_SENDING;

		if (pGround == pEntity && isSending)
		{
			// The teleporter is teleporting me, wait for the teleport.
			return Continue();
		}

		// For level 3 teleporters, bots wait for it to recharge, the timeout timer will prevent bots from getting perma stuck in this behavior.
		if (teleporter.GetLevel() > 2)
		{
			return Continue();
		}
	}

	if (pGround != pEntity)
	{
		if (m_nav.NeedsRepath())
		{
			m_nav.StartRepathTimer();
			CTF2BotPathCost cost(bot);
			m_nav.ComputePathToPosition(bot, UtilHelpers::getEntityOrigin(pEntity), cost, 0.0f, true);
		}

		m_nav.Update(bot);
	}

	return Continue();
}
