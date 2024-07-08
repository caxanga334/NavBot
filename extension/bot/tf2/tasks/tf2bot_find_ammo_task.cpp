#include <limits>

#include <extension.h>
#include <util/helpers.h>
#include <util/entprops.h>
#include <util/librandom.h>
#include <entities/tf2/tf_entities.h>
#include <sdkports/debugoverlay_shared.h>
#include <mods/tf2/tf2lib.h>
#include "bot/tf2/tf2bot.h"
#include "tf2bot_find_ammo_task.h"

CTF2BotFindAmmoTask::CTF2BotFindAmmoTask() : m_sourcepos(0.0f, 0.0f, 0.0f),
m_type(AmmoSource::NONE),
m_reached(false),
m_metalLimit(200)
{
}

CTF2BotFindAmmoTask::CTF2BotFindAmmoTask(int maxmetal) : m_sourcepos(0.0f, 0.0f, 0.0f),
m_type(AmmoSource::NONE),
m_reached(false)
{
	m_metalLimit = maxmetal;
}

TaskResult<CTF2Bot> CTF2BotFindAmmoTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	m_type = FindSource(bot);

	if (m_type == AmmoSource::NONE)
	{
		return Done("Failed to find an ammo source!");
	}

	CTF2BotPathCost cost(bot);
	if (!m_nav.ComputePathToPosition(bot, m_sourcepos, cost))
	{
		return Done("Failed to build a path to the ammo source!");
	}

	m_repathtimer.Start(0.5f);

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotFindAmmoTask::OnTaskUpdate(CTF2Bot* bot)
{
	if (!IsSourceStillValid(bot))
		return Done("Ammo Source is invalid!");

	if (m_reached && m_failsafetimer.IsElapsed())
	{
		return Done("Ammo collected!");
	}

	if (bot->GetMyClassType() == TeamFortress2::TFClass_Engineer && bot->GetAmmoOfIndex(TeamFortress2::TF_AMMO_METAL) >= m_metalLimit)
	{
		return Done("Ammo collected!");
	}

	UpdateSourcePosition();

	if (m_repathtimer.IsElapsed())
	{
		m_repathtimer.Start(0.5f);

		CTF2BotPathCost cost(bot);
		if (!m_nav.ComputePathToPosition(bot, m_sourcepos, cost))
		{
			return Done("Failed to build a path to the ammo source!");
		}
	}

	// if the bot is this close to the dispenser, stop moving
	static constexpr auto DISPENSER_TOUCH_RANGE = 64.0f;

	if (m_type == AmmoSource::DISPENSER && bot->GetRangeTo(m_sourcepos) < DISPENSER_TOUCH_RANGE)
	{
		return Continue();
	}

	m_nav.Update(bot);

	return Continue();
}

TaskEventResponseResult<CTF2Bot> CTF2BotFindAmmoTask::OnMoveToFailure(CTF2Bot* bot, CPath* path, IEventListener::MovementFailureType reason)
{
	bot->GetMovementInterface()->ClearStuckStatus("Repath!");
	m_repathtimer.Start(0.5f);

	CTF2BotPathCost cost(bot);
	if (!m_nav.ComputePathToPosition(bot, m_sourcepos, cost))
	{
		return TryDone(PRIORITY_HIGH, "Failed to build a path to the ammo source!");
	}

	return TryContinue();
}

TaskEventResponseResult<CTF2Bot> CTF2BotFindAmmoTask::OnMoveToSuccess(CTF2Bot* bot, CPath* path)
{
	if (!m_reached)
	{
		m_reached = true;

		switch (m_type)
		{
		case CTF2BotFindAmmoTask::AmmoSource::AMMOPACK:
		case CTF2BotFindAmmoTask::AmmoSource::RESUPPLY:
			m_failsafetimer.Start(1.5f);
			break;
		default:
			m_failsafetimer.Start(10.0f);
			break;
		}
	}

	return TryContinue();
}

CTF2BotFindAmmoTask::AmmoSource CTF2BotFindAmmoTask::FindSource(CTF2Bot* me)
{
	Vector origin = me->GetAbsOrigin();
	float smallest = std::numeric_limits<float>::max();
	edict_t* best = nullptr;
	auto myteam = me->GetMyTFTeam();
	AmmoSource source = AmmoSource::NONE;

	auto evaluateammopack = [&myteam](edict_t* ammopack) -> bool {
		tfentities::HTFBaseEntity entity(ammopack);

		if (entity.IsEffectActive(EF_NODRAW))
			return false;

		if (entity.IsDisabled())
			return false;

		if (entity.GetTFTeam() != TeamFortress2::TFTeam_Unassigned && entity.GetTFTeam() != myteam)
			return false;

		return true;
	};

	auto evaluateresupply = [&myteam](edict_t* resupply) -> bool {
		tfentities::HFuncRegenerate regen(resupply);

		if (regen.IsDisabled())
			return false;

		if (regen.GetTFTeam() != TeamFortress2::TFTeam_Unassigned && regen.GetTFTeam() != myteam)
			return false;

		return true;
	};

	auto evaluatedispenser = [&myteam](edict_t* dispenser) -> bool {
		tfentities::HBaseObject object(dispenser);

		if (object.IsSapped() || object.IsDisabled() || object.IsPlacing() || object.IsBuilding() || object.IsBeingCarried() || object.IsRedeploying())
			return false;

		if (object.GetTFTeam() != myteam)
			return false;

		return true;
	};

	for (int i = gpGlobals->maxClients + 1; i < gpGlobals->maxEntities; i++)
	{
		edict_t* edict = gamehelpers->EdictOfIndex(i);

		if (!edict)
			continue;

		if (edict->IsFree())
			continue;

		auto classname = gamehelpers->GetEntityClassname(edict);

		if (classname == nullptr || classname[0] == 0)
			continue;

		float distance_mul = 1.0f;
		AmmoSource currentsource = AmmoSource::NONE;

		if (strncasecmp(classname, "func_regenerate", 15) == 0)
		{
			if (!evaluateresupply(edict))
				continue;

			distance_mul = 0.6f; // prefer resupply lockers
			currentsource = AmmoSource::RESUPPLY;
		}
		else if (strncasecmp(classname, "obj_dispenser", 13) == 0)
		{
			if (!evaluatedispenser(edict))
				continue;

			distance_mul = 0.75f; // prefer dispensers over ammopacks
			currentsource = AmmoSource::DISPENSER;
		}
		else if (strncasecmp(classname, "item_ammopack", 13) == 0)
		{
			if (!evaluateammopack(edict))
				continue;

			currentsource = AmmoSource::AMMOPACK;
		}
		else
		{
			continue;
		}

		Vector center = UtilHelpers::getWorldSpaceCenter(edict);
		float distance = ((center - origin).Length());

		if (distance > max_distance())
			continue;

		// apply preference
		distance = distance * distance_mul;

		if (distance < smallest)
		{
			smallest = distance;
			best = edict;
			source = currentsource;
		}
	}

	if (best)
	{
		m_sourceentity.Set(best->GetIServerEntity());

		if (source == AmmoSource::RESUPPLY)
		{
			m_sourcepos = GetResupplyPosition(best);
		}
		else
		{
			m_sourcepos = UtilHelpers::getWorldSpaceCenter(best);
		}

		if (me->IsDebugging(BOTDEBUG_TASKS))
		{
			me->DebugPrintToConsole(BOTDEBUG_TASKS, 153, 156, 255, "%s Found Ammo Source <%i> at %3.2f, %3.2f, %3.2f\n", me->GetDebugIdentifier(), static_cast<int>(source),
				m_sourcepos.x, m_sourcepos.y, m_sourcepos.z);

			NDebugOverlay::Text(m_sourcepos, "Ammo Source!", false, 10.0f);
			NDebugOverlay::Sphere(m_sourcepos, 32.0f, 153, 156, 255, true, 10.0f);
		}

		return source;
	}

	return AmmoSource::NONE;
}

void CTF2BotFindAmmoTask::UpdateSourcePosition()
{
	// the other ammo sources shouldn't move
	if (m_type == AmmoSource::DISPENSER)
	{
		auto edict = gamehelpers->GetHandleEntity(m_sourceentity);
		m_sourcepos = UtilHelpers::getWorldSpaceCenter(edict);
	}
}

bool CTF2BotFindAmmoTask::IsSourceStillValid(CTF2Bot* me)
{
	auto edict = gamehelpers->GetHandleEntity(m_sourceentity);

	if (!edict)
		return false; // source entity no longer exists

	switch (m_type)
	{
	case CTF2BotFindAmmoTask::AmmoSource::AMMOPACK:
	{
		tfentities::HTFBaseEntity entity(edict);

		if (entity.IsEffectActive(EF_NODRAW))
			return false;

		if (entity.IsDisabled())
			return false;

		break;
	}
	case CTF2BotFindAmmoTask::AmmoSource::RESUPPLY:
	{

		tfentities::HFuncRegenerate regen(edict);

		if (regen.IsDisabled())
			return false;

		if (regen.GetTFTeam() != TeamFortress2::TFTeam_Unassigned && regen.GetTFTeam() != me->GetMyTFTeam())
			return false;

		break;
	}
	case CTF2BotFindAmmoTask::AmmoSource::DISPENSER:
	{
		tfentities::HBaseObject object(edict);

		if (object.IsSapped() || object.IsDisabled() || object.IsPlacing() || object.IsBuilding() || object.IsBeingCarried() || object.IsRedeploying())
			return false;

		if (object.GetTFTeam() != me->GetMyTFTeam())
			return false;

		break;
	}
	default:
		break;
	}

	return true;
}

Vector CTF2BotFindAmmoTask::GetResupplyPosition(edict_t* resupply)
{
	Vector center = UtilHelpers::getWorldSpaceCenter(resupply);
	auto area = TheNavMesh->GetNearestNavArea(center, 1024.0f, false, false);

	if (area == nullptr)
	{
		return UtilHelpers::GetGroundPositionFromCenter(resupply);
	}

	Vector point;

	area->GetClosestPointOnArea(center, &point);
	point.z = area->GetZ(point.x, point.y);
	return point;
}
