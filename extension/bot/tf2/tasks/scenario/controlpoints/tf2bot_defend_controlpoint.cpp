#include <extension.h>
#include <util/entprops.h>
#include <sdkports/sdk_traces.h>
#include <mods/tf2/teamfortress2mod.h>
#include <mods/tf2/tf2lib.h>
#include <entities/tf2/tf_entities.h>
#include <mods/tf2/nav/tfnavmesh.h>
#include <mods/tf2/nav/tfnavarea.h>
#include <navmesh/nav_pathfind.h>
#include <bot/tf2/tf2bot.h>
#include "tf2bot_defend_controlpoint.h"

CTF2BotDefendControlPointTask::CTF2BotDefendControlPointTask(CBaseEntity* controlpoint) :
	m_capturePos(0.0f, 0.0f, 0.0f)
{
	m_controlpoint = controlpoint;
}

TaskResult<CTF2Bot> CTF2BotDefendControlPointTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
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

TaskResult<CTF2Bot> CTF2BotDefendControlPointTask::OnTaskUpdate(CTF2Bot* bot)
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

	tfentities::HTeamControlPoint controlpoint(pEntity);

	if (controlpoint.GetTFTeam() != bot->GetMyTFTeam())
	{
		return Done("I've failed to defend the control point!");
	}

	int index = controlpoint.GetPointIndex();
	float capPercent = CTeamFortress2Mod::GetTF2Mod()->GetTFObjectiveResource()->GetCapturePercentage(index);

	// If the objective resources is NULL we have bigger problems.
	if (capPercent < 0.001f || capPercent >= 0.99f)
	{
		// if the cap percent is 0.0, then the control point is not being capped, 0.99 means 1% progress towards capture

		return Done("Done defending!");
	}

	Vector pos = bot->WorldSpaceCenter();

	if (!trace::pointwithin(pEntity, pos))
	{
		CTF2BotPathCost cost(bot);
		m_nav.Update(bot, m_capturePos, cost);
	}

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotDefendControlPointTask::OnTaskResume(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	if (m_controlpoint.Get() == nullptr)
	{
		return Done("NULL control point!");
	}

	return Continue();
}

void CTF2BotDefendControlPointTask::FindCaptureTrigger(CBaseEntity* controlpoint)
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

class ControlPointGuardAreas : public INavAreaCollector<CTFNavArea>
{
public:
	ControlPointGuardAreas(const Vector& capturePos, int teamID)
	{
		m_capturePos = capturePos;
		m_myTeam = teamID;
		SetTravelLimit(1024.0f); // maximum distance to control point
	}

	// Checks if the given area should be searched
	bool ShouldSearch(CTFNavArea* area) override
	{
		if (area->IsBlocked(m_myTeam))
		{
			return false;
		}

		return true;
	}

	// Checks if the given area should be collected
	bool ShouldCollect(CTFNavArea* area) override
	{
		Vector start = area->GetCenter();
		start.z += (navgenparams->human_eye_height * 0.25f);

		trace_t tr;
		trace::line(start, m_capturePos, MASK_VISIBLE, &m_filter, tr);

		if (!tr.DidHit())
		{
			return true; // area has LOS to the capture point
		}

		return false;
	}

private:
	Vector m_capturePos;
	int m_myTeam;
	CTraceFilterWorldAndPropsOnly m_filter;
};

CTF2BotGuardControlPointTask::CTF2BotGuardControlPointTask(CBaseEntity* controlpoint) :
	m_lookTarget(0.0f, 0.0f, 0.0f)
{
	m_controlpoint = controlpoint;
	m_currentSpot = 0U;
}

TaskResult<CTF2Bot> CTF2BotGuardControlPointTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	CBaseEntity* pEntity = m_controlpoint.Get();

	if (pEntity == nullptr)
	{
		return Done("Given control point is NULL!");
	}

	BuildGuardSpots(bot);

	if (m_guardPos.empty())
	{
		return Done("Failed to find positions to defend the control point!");
	}

	tfentities::HTeamControlPoint cp(pEntity);

	if (cp.IsLocked())
	{
		return Done("Control point is locked!");
	}

	m_lookTarget = UtilHelpers::getWorldSpaceCenter(pEntity);

	m_moveTimer.StartRandom(5.0f, 10.0f);
	m_endTimer.StartRandom(30.f, 90.0f);
	m_currentSpot = randomgen->GetRandomInt<size_t>(0U, m_guardPos.size() - 1U);

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotGuardControlPointTask::OnTaskUpdate(CTF2Bot* bot)
{
	if (m_endTimer.IsElapsed())
	{
		return Done("Guard timer is up!");
	}

	CBaseEntity* pEntity = m_controlpoint.Get();

	if (pEntity == nullptr)
	{
		return Done("Given control point is NULL!");
	}

	if (m_moveTimer.IsElapsed())
	{
		m_currentSpot = randomgen->GetRandomInt<size_t>(0U, m_guardPos.size() - 1U);
		m_moveTimer.StartRandom(5.0f, 10.0f);
	}

	tfentities::HTeamControlPoint controlpoint(pEntity);

	if (controlpoint.GetTFTeam() != bot->GetMyTFTeam())
	{
		return Done("I've failed to guard the control point!");
	}

	int index = controlpoint.GetPointIndex();
	float capPercent = CTeamFortress2Mod::GetTF2Mod()->GetTFObjectiveResource()->GetCapturePercentage(index);

	// If the objective resources is NULL we have bigger problems.
	if (capPercent >= 0.01f)
	{
		return SwitchTo(new CTF2BotDefendControlPointTask(pEntity), "Point is getting capped, move to block!");
	}

	const Vector& moveGoal = m_guardPos[m_currentSpot];

	if (bot->GetRangeTo(moveGoal) > 24.0f)
	{
		CTF2BotPathCost cost(bot);
		m_nav.Update(bot, moveGoal, cost);
	}
	else
	{
		if (!bot->IsLookingTowards(m_lookTarget, 0.7f))
		{
			bot->GetControlInterface()->AimAt(m_lookTarget, IPlayerController::LOOK_INTERESTING, 0.5f, "Looking at control point!");
		}
	}

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotGuardControlPointTask::OnTaskResume(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	CBaseEntity* pEntity = m_controlpoint.Get();

	if (pEntity == nullptr)
	{
		return Done("Given control point is NULL!");
	}

	return Continue();
}

void CTF2BotGuardControlPointTask::BuildGuardSpots(CTF2Bot* bot)
{
	CBaseEntity* entity = m_controlpoint.Get();

	Vector center = UtilHelpers::getWorldSpaceCenter(entity);

	if (!center.IsZero(0.5f))
	{
		ControlPointGuardAreas collector(center, static_cast<int>(bot->GetMyTFTeam()));

		CTFNavArea* area = static_cast<CTFNavArea*>(TheNavMesh->GetNearestNavArea(center, 1024.0f));

		if (area == nullptr)
		{
			bot->DebugPrintToConsole(BOTDEBUG_TASKS, 255, 0, 0, "CTF2BotGuardControlPointTask::BuildGuardSpots -- control point nav area is NULL!\n");
			return;
		}

		collector.Reset();
		collector.SetStartArea(area);
		collector.Execute();

		if (!collector.IsCollectedAreasEmpty())
		{
			for (auto guardArea : collector.GetCollectedAreas())
			{
				auto& spot = m_guardPos.emplace_back();
				spot = guardArea->GetCenter();

				if (bot->IsDebugging(BOTDEBUG_TASKS))
				{
					guardArea->DrawFilled(255, 255, 0, 128, 5.0f);
				}
			}
		}
		else
		{
			bot->DebugPrintToConsole(BOTDEBUG_TASKS, 255, 0, 0, "CTF2BotGuardControlPointTask::BuildGuardSpots -- no guard areas!\n");
		}
	}
}
