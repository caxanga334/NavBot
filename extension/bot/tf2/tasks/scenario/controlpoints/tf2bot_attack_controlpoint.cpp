#include <extension.h>
#include <util/entprops.h>
#include <mods/tf2/teamfortress2mod.h>
#include <mods/tf2/tf2lib.h>
#include <entities/tf2/tf_entities.h>
#include <bot/tf2/tf2bot.h>
#include <bot/tf2/tasks/tf2bot_attack.h>
#include "tf2bot_attack_controlpoint.h"

CTF2BotAttackControlPointTask::CTF2BotAttackControlPointTask(CBaseEntity* controlpoint)
{
	m_controlpoint = controlpoint;
}

TaskResult<CTF2Bot> CTF2BotAttackControlPointTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	CBaseEntity* pEntity = m_controlpoint.Get();

	if (pEntity == nullptr)
	{
		return Done("Given control point is NULL!");
	}

	tfentities::HTeamControlPoint cp(pEntity);

	if (cp.IsLocked())
	{
		return Done("Control point is locked!");
	}

	FindCaptureTrigger(pEntity);
	m_refreshPosTimer.Start(5.0f);

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotAttackControlPointTask::OnTaskUpdate(CTF2Bot* bot)
{
	CBaseEntity* pEntity = m_controlpoint.Get();

	if (pEntity == nullptr)
	{
		return Done("Given control point is NULL!");
	}

	if (m_refreshPosTimer.IsElapsed())
	{
		m_refreshPosTimer.Start(5.0f);
		FindCaptureTrigger(pEntity);
	}

	auto threat = bot->GetSensorInterface()->GetPrimaryKnownThreat(true);

	if (threat)
	{
		return PauseFor(new CTF2BotAttackTask(threat->GetEntity(), 2.0f, 20.0f), "Attacking threat!");
	}

	tfentities::HTeamControlPoint cp(pEntity);
	Vector pos = bot->WorldSpaceCenter();

	if (!trace::pointwithin(pEntity, pos))
	{
		CTF2BotPathCost cost(bot);
		m_nav.Update(bot, m_capturePos, cost);
	}

	std::vector<CBaseEntity*> Vecpoints;
	CTeamFortress2Mod::GetTF2Mod()->CollectControlPointsToAttack(bot->GetMyTFTeam(), Vecpoints);

	for (auto point : Vecpoints)
	{
		if (point == pEntity)
		{
			return Continue(); // current target point still is a valid attack target
		}
	}

	// current target is not listed on the control point attack list but the list is also not empty

	if (!Vecpoints.empty())
	{
		CBaseEntity* newPoint = librandom::utils::GetRandomElementFromVector<CBaseEntity*>(Vecpoints);
		return SwitchTo(new CTF2BotAttackControlPointTask(newPoint), "Attacking another control point!");
	}

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotAttackControlPointTask::OnTaskResume(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	if (m_controlpoint.Get() == nullptr)
	{
		return Done("NULL control point!");
	}

	return Continue();
}

void CTF2BotAttackControlPointTask::FindCaptureTrigger(CBaseEntity* controlpoint)
{
	tfentities::HTeamControlPoint entity(controlpoint);

	std::unique_ptr<char[]> targetname = std::make_unique<char[]>(128);
	entity.GetTargetName(targetname.get(), 128);
	CBaseEntity* trigger = nullptr;

	UtilHelpers::ForEachEntityOfClassname("trigger_capture_area", [&targetname, &controlpoint, &trigger](int index, edict_t* edict, CBaseEntity* entity) {

		if (entity != nullptr)
		{
			std::unique_ptr<char[]> cpname = std::make_unique<char[]>(128);
			size_t length = 0;
			entprops->GetEntPropString(index, Prop_Data, "m_iszCapPointName", cpname.get(), 128, length);

			if (length > 0)
			{
				int cpindex = UtilHelpers::FindEntityByTargetname(INVALID_EHANDLE_INDEX, cpname.get());

				if (cpindex == gamehelpers->EntityToBCompatRef(controlpoint))
				{
					trigger = entity;
					return false;
				}
			}

		}

		return true;
	});

	if (trigger)
	{
		Vector pos = UtilHelpers::getWorldSpaceCenter(trigger);

		if (!pos.IsZero(0.5f))
		{
			pos = trace::getground(pos);
			m_capturePos = pos;
		}
		else
		{
			m_capturePos = entity.GetAbsOrigin();
		}
	}
	else
	{
		m_capturePos = entity.GetAbsOrigin();
	}
}
