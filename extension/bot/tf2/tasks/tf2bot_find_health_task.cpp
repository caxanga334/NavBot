#include <limits>

#include <extension.h>
#include <util/helpers.h>
#include <util/entprops.h>
#include <util/librandom.h>
#include <entities/tf2/tf_entities.h>
#include <sdkports/debugoverlay_shared.h>
#include <mods/tf2/tf2lib.h>
#include "bot/tf2/tf2bot.h"
#include "tf2bot_find_health_task.h"

TaskResult<CTF2Bot> CTF2BotFindHealthTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	m_type = FindSource(bot);

	if (m_type == HealthSource::NONE)
	{
		return Done("Failed to find a health source!");
	}

	CTF2BotPathCost cost(bot);
	if (!m_nav.ComputePathToPosition(bot, m_sourcepos, cost))
	{
		return Done("Failed to build a path to the Health source!");
	}

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotFindHealthTask::OnTaskUpdate(CTF2Bot* bot)
{
	if (!IsSourceStillValid(bot))
		return Done("Health Source is invalid!");

	if (m_reached && m_failsafetimer.IsElapsed())
	{
		return Done("Health collected!");
	}

	if (bot->GetHealthPercentage() >= 0.98f)
	{
		return Done("I'm at full health!");
	}

	UpdateSourcePosition();

	// if the bot is this close to the dispenser, stop moving
	static constexpr auto DISPENSER_TOUCH_RANGE = 64.0f;

	if (m_type == HealthSource::DISPENSER && bot->GetRangeTo(m_sourcepos) < DISPENSER_TOUCH_RANGE)
	{
		return Continue();
	}

	CTF2BotPathCost cost(bot);
	m_nav.Update(bot, m_sourcepos, cost);

	return Continue();
}

CTF2BotFindHealthTask::HealthSource CTF2BotFindHealthTask::FindSource(CTF2Bot* me)
{
	Vector origin = me->GetAbsOrigin();
	float smallest = std::numeric_limits<float>::max();
	edict_t* best = nullptr;
	auto myteam = me->GetMyTFTeam();
	HealthSource source = HealthSource::NONE;

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
		HealthSource currentsource = HealthSource::NONE;

		if (strncasecmp(classname, "func_regenerate", 15) == 0)
		{
			if (!evaluateresupply(edict))
				continue;

			distance_mul = 0.6f; // prefer resupply lockers
			currentsource = HealthSource::RESUPPLY;
		}
		else if (strncasecmp(classname, "obj_dispenser", 13) == 0)
		{
			if (!evaluatedispenser(edict))
				continue;

			distance_mul = 0.75f; // prefer dispensers over health kits
			currentsource = HealthSource::DISPENSER;
		}
		else if (strncasecmp(classname, "item_healthkit", 14) == 0)
		{
			if (!evaluateammopack(edict))
				continue;

			currentsource = HealthSource::HEALTHPACK;
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

		if (source == HealthSource::RESUPPLY)
		{
			m_sourcepos = UtilHelpers::GetGroundPositionFromCenter(best);
		}
		else
		{
			m_sourcepos = UtilHelpers::getWorldSpaceCenter(best);
		}

		if (me->IsDebugging(BOTDEBUG_TASKS))
		{
			me->DebugPrintToConsole(BOTDEBUG_TASKS, 153, 156, 255, "%s Found Health Source Source <%i> at %3.2f, %3.2f, %3.2f \n", me->GetDebugIdentifier(), static_cast<int>(source),
				m_sourcepos.x, m_sourcepos.y, m_sourcepos.z);

			NDebugOverlay::Text(m_sourcepos, "Health Source!", false, 10.0f);
			NDebugOverlay::Sphere(m_sourcepos, 32.0f, 153, 156, 255, true, 10.0f);
		}

		return source;
	}

	return HealthSource::NONE;
}

void CTF2BotFindHealthTask::UpdateSourcePosition()
{
	// the other ammo sources shouldn't move
	if (m_type == HealthSource::DISPENSER)
	{
		auto edict = gamehelpers->GetHandleEntity(m_sourceentity);
		m_sourcepos = UtilHelpers::getWorldSpaceCenter(edict);
	}
}

bool CTF2BotFindHealthTask::IsSourceStillValid(CTF2Bot* me)
{
	auto edict = gamehelpers->GetHandleEntity(m_sourceentity);

	if (!edict)
		return false; // source entity no longer exists

	switch (m_type)
	{
	case CTF2BotFindHealthTask::HealthSource::HEALTHPACK:
	{
		tfentities::HTFBaseEntity entity(edict);

		if (entity.IsEffectActive(EF_NODRAW) || entity.IsDisabled())
			return false;

		break;
	}
	case CTF2BotFindHealthTask::HealthSource::RESUPPLY:
	{
		tfentities::HFuncRegenerate regen(edict);

		if (regen.IsDisabled())
			return false;

		if (regen.GetTFTeam() != TeamFortress2::TFTeam_Unassigned && regen.GetTFTeam() != me->GetMyTFTeam())
			return false;

		break;
	}
	case CTF2BotFindHealthTask::HealthSource::DISPENSER:
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