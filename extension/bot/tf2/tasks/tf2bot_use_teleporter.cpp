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

CTF2BotUseTeleporterTask::CTF2BotUseTeleporterTask(CBaseEntity* teleporter) :
	m_teleporter(teleporter)
{
}

bool CTF2BotUseTeleporterTask::IsPossible(CTF2Bot* bot, CBaseEntity** teleporter)
{
	CMeshNavigator* nav = bot->GetActiveNavigator();

	if (!nav || !nav->IsValid()) { return false; }

	if (bot->GetItem() != nullptr) { return false; }

	UtilHelpers::CEntityEnumerator enumerator;

	CBaseEntity* entrance = nullptr;
	CBaseEntity* exit = nullptr;

	auto func = [&entrance, &exit, &bot](int index, edict_t* edict, CBaseEntity* entity) {
		tfentities::HObjectTeleporter tele(entity);

		if (tele.IsSapped() || tele.GetPercentageConstructed() < 1.0f || tele.IsBeingCarried() || tele.IsPlacing() ||
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
		
		// too far
		if (bot->GetRangeTo(tele.WorldSpaceCenter()) > 1000.0f)
		{
			return true;
		}

		entrance = entity;
		exit = tf2lib::GetMatchingTeleporter(entity);
		return false; // stop on the first teleporter found
	};

	UtilHelpers::ForEachEntityOfClassname("obj_teleporter", func);

	Vector origin = bot->GetAbsOrigin();

	if (!entrance || !exit) { return false; }

	/*
	* Simple distance check for speed reasons.
	* The ideal solution would be comparing the path length between the bot and the teleporter exit.
	*/

	constexpr auto TELEPORTER_EXTRA_DIST = 300.0f; // artificial distance added to the teleporter distance to goal
	const Vector& goal = nav->GetPathDestination();
	const Vector& telepos = UtilHelpers::getEntityOrigin(exit);
	float distBotToGoal = (origin - goal).Length();

	if (distBotToGoal <= 512.0f)
	{
		return false; // already close to the goal
	}


	float distTPToGoal = (telepos - goal).Length() + TELEPORTER_EXTRA_DIST;

	if (distTPToGoal < distBotToGoal)
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

	if (teleporter.IsSapped() || teleporter.GetState() != TeamFortress2::TeleporterState::TELEPORTER_STATE_READY)
	{
		return Done("Teleporter is no longer available!");
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
