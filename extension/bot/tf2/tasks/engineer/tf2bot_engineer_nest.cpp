#include <extension.h>
#include <util/librandom.h>
#include <entities/tf2/tf_entities.h>
#include <mods/tf2/teamfortress2mod.h>
#include <bot/tf2/tf2bot.h>
#include <mods/tf2/nav/tfnavarea.h>
#include <mods/tf2/nav/tfnav_waypoint.h>
#include <navmesh/nav_pathfind.h>
#include <sdkports/sdk_traces.h>
#include "tf2bot_engineer_build_object.h"
#include "tf2bot_engineer_repair_object.h"
#include "tf2bot_engineer_upgrade_object.h"
#include "tf2bot_engineer_nest.h"

class EngineerBuildableLocationCollector : public INavAreaCollector<CTFNavArea>
{
public:
	EngineerBuildableLocationCollector(CTF2Bot* engineer, CTFNavArea* startArea) :
		INavAreaCollector<CTFNavArea>(startArea, 4096.0f)
	{
		m_me = engineer;
	}

	bool ShouldCollect(CTFNavArea* area) override;
private:
	CTF2Bot* m_me;
};

inline static Vector GetSpotBehindSentry(edict_t* sentry)
{
	tfentities::HBaseObject sentrygun(sentry);
	Vector origin = sentrygun.GetAbsOrigin();
	QAngle angle = sentrygun.GetAbsAngles();
	Vector forward;

	AngleVectors(angle, &forward);
	forward.NormalizeInPlace();

	Vector point = (origin - (forward * CTF2BotEngineerNestTask::behind_sentry_distance()));
	return point;
}

bool EngineerBuildableLocationCollector::ShouldCollect(CTFNavArea* area)
{
	if (!area->IsBuildable())
	{
		return false;
	}

	return true;
}

CTF2BotEngineerNestTask::CTF2BotEngineerNestTask()
{
	m_goal.Init(0.0f, 0.0f, 0.0f);
}

TaskResult<CTF2Bot> CTF2BotEngineerNestTask::OnTaskUpdate(CTF2Bot* bot)
{
	bot->FindMyBuildings();

	AITask<CTF2Bot>* nextTask = NestTask(bot);

	if (nextTask != nullptr)
	{
		return PauseFor(nextTask, "Taking care of my own nest!");
	}

	/* TO-DO:
	* Check if buildings are still useful, if not, move them
	*/

	// At this point, the bot has fully upgraded everything, time to idle near the sentry

	m_goal = GetSpotBehindSentry(bot->GetMySentryGun());

	if (bot->GetRangeTo(m_goal) > 32.0f)
	{
		if (!m_nav.IsValid() || m_nav.GetAge() > 3.0f)
		{
			CTF2BotPathCost cost(bot);
			m_nav.ComputePathToPosition(bot, m_goal, cost);
		}

		m_nav.Update(bot);
	}

	return Continue();
}

bool CTF2BotEngineerNestTask::OnTaskPause(CTF2Bot* bot, AITask<CTF2Bot>* nextTask)
{
	m_nav.Invalidate();
	return true;
}

TaskResult<CTF2Bot> CTF2BotEngineerNestTask::OnTaskResume(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	return Continue();
}

TaskEventResponseResult<CTF2Bot> CTF2BotEngineerNestTask::OnMoveToFailure(CTF2Bot* bot, CPath* path, IEventListener::MovementFailureType reason)
{
	m_nav.Invalidate();
	CTF2BotPathCost cost(bot);
	m_nav.ComputePathToPosition(bot, m_goal, cost);

	return TryContinue();
}

TaskEventResponseResult<CTF2Bot> CTF2BotEngineerNestTask::OnMoveToSuccess(CTF2Bot* bot, CPath* path)
{
	return TryContinue();
}

AITask<CTF2Bot>* CTF2BotEngineerNestTask::NestTask(CTF2Bot* me)
{
	Vector goal;
	CTFWaypoint* wpt = nullptr;

	if (me->GetMyTeleporterEntrance() == nullptr)
	{
		if (FindSpotToBuildTeleEntrance(me, &wpt))
		{
			return new CTF2BotEngineerBuildObjectTask(CTF2BotEngineerBuildObjectTask::OBJECT_TELEPORTER_ENTRANCE, wpt);
		}
	}
	else if (me->GetMySentryGun() == nullptr)
	{
		if (FindSpotToBuildSentryGun(me, &wpt))
		{
			return new CTF2BotEngineerBuildObjectTask(CTF2BotEngineerBuildObjectTask::OBJECT_SENTRYGUN, wpt);
		}
	}
	else if (me->GetMyDispenser() == nullptr)
	{
		// Search for nearby waypoints
		if (FindSpotToBuildDispenser(me, &wpt))
		{
			return new CTF2BotEngineerBuildObjectTask(CTF2BotEngineerBuildObjectTask::OBJECT_DISPENSER, wpt);
		}

		// Try building behind the sentry
		if (FindSpotToBuildDispenser(me, goal))
		{
			return new CTF2BotEngineerBuildObjectTask(CTF2BotEngineerBuildObjectTask::OBJECT_DISPENSER, goal);
		}
	}
	else if (me->GetMyTeleporterExit() == nullptr)
	{
		if (FindSpotToBuildTeleExit(me, &wpt))
		{
			return new CTF2BotEngineerBuildObjectTask(CTF2BotEngineerBuildObjectTask::OBJECT_TELEPORTER_EXIT, wpt);
		}
	}

	if (me->GetMySentryGun() != nullptr)
	{
		tfentities::HBaseObject sentrygun(me->GetMySentryGun());

		if (!sentrygun.IsAtMaxLevel())
		{
			return new CTF2BotEngineerUpgradeObjectTask(gamehelpers->ReferenceToEntity(sentrygun.GetIndex()));
		}
	}

	if (me->GetMyDispenser() != nullptr)
	{
		tfentities::HBaseObject dispenser(me->GetMyDispenser());

		if (!dispenser.IsAtMaxLevel())
		{
			return new CTF2BotEngineerUpgradeObjectTask(gamehelpers->ReferenceToEntity(dispenser.GetIndex()));
		}
	}

	// For now we assume the exit is closer to the bot
	if (me->GetMyTeleporterExit() != nullptr)
	{
		tfentities::HBaseObject exit(me->GetMyTeleporterExit());

		if (!exit.IsAtMaxLevel())
		{
			return new CTF2BotEngineerUpgradeObjectTask(gamehelpers->ReferenceToEntity(exit.GetIndex()));
		}
	}

	return nullptr;
}

bool CTF2BotEngineerNestTask::FindSpotToBuildSentryGun(CTF2Bot* me, CTFWaypoint** out)
{
	std::vector<CTFWaypoint*> spots;
	auto& sentryWaypoints = CTeamFortress2Mod::GetTF2Mod()->GetAllSentryWaypoints();

	for (auto waypoint : sentryWaypoints)
	{
		if (waypoint->IsEnabled() && waypoint->IsAvailableToTeam(static_cast<int>(me->GetMyTFTeam())) && waypoint->CanBeUsedByBot(me))
		{
			spots.push_back(waypoint);
		}
	}

	if (spots.empty())
	{
		return false;
	}

	*out = librandom::utils::GetRandomElementFromVector(spots);
	return true;
}

bool CTF2BotEngineerNestTask::FindSpotToBuildDispenser(CTF2Bot* me, CTFWaypoint** out)
{
	std::vector<CTFWaypoint*> spots;
	auto& dispenserWaypoints = CTeamFortress2Mod::GetTF2Mod()->GetAllDispenserWaypoints();
	edict_t* mySentry = me->GetMySentryGun();

	if (mySentry == nullptr)
	{
		return false;
	}

	Vector origin = UtilHelpers::getWorldSpaceCenter(mySentry);

	for (auto waypoint : dispenserWaypoints)
	{

		if (waypoint->IsEnabled() && waypoint->DistanceTo(origin) <= max_dispenser_to_sentry_range() && 
			waypoint->IsAvailableToTeam(static_cast<int>(me->GetMyTFTeam())) && waypoint->CanBeUsedByBot(me))
		{
			spots.push_back(waypoint);
		}
	}

	if (spots.empty())
	{
		return false;
	}

	*out = librandom::utils::GetRandomElementFromVector(spots);
	return true;
}

bool CTF2BotEngineerNestTask::FindSpotToBuildTeleEntrance(CTF2Bot* me, CTFWaypoint** out)
{
	std::vector<CTFWaypoint*> spots;
	auto& teleentranceWaypoints = CTeamFortress2Mod::GetTF2Mod()->GetAllTeleEntranceWaypoints();

	for (auto waypoint : teleentranceWaypoints)
	{
		if (waypoint->IsEnabled() && waypoint->IsAvailableToTeam(static_cast<int>(me->GetMyTFTeam())) && waypoint->CanBeUsedByBot(me))
		{
			spots.push_back(waypoint);
		}
	}

	if (spots.empty())
	{
		return false;
	}

	*out = librandom::utils::GetRandomElementFromVector(spots);
	return true;
}

bool CTF2BotEngineerNestTask::FindSpotToBuildTeleExit(CTF2Bot* me, CTFWaypoint** out)
{
	std::vector<CTFWaypoint*> spots;
	auto& teleexitWaypoints = CTeamFortress2Mod::GetTF2Mod()->GetAllTeleExitWaypoints();

	for (auto waypoint : teleexitWaypoints)
	{
		if (waypoint->IsEnabled() && waypoint->IsAvailableToTeam(static_cast<int>(me->GetMyTFTeam())) && waypoint->CanBeUsedByBot(me))
		{
			spots.push_back(waypoint);
		}
	}

	if (spots.empty())
	{
		return false;
	}

	*out = librandom::utils::GetRandomElementFromVector(spots);
	return true;
}

bool CTF2BotEngineerNestTask::FindSpotToBuildDispenser(CTF2Bot* me, Vector& out)
{
	edict_t* mysentry = me->GetMySentryGun();

	if (mysentry == nullptr)
	{
		// Must build sentry first
		return false;
	}

	tfentities::HBaseObject sentrygun(mysentry);
	Vector origin = sentrygun.GetAbsOrigin();
	QAngle angle = sentrygun.GetAbsAngles();
	Vector forward;

	AngleVectors(angle, &forward);
	forward.NormalizeInPlace();

	static constexpr auto DISPENSER_OFFSET = 32.0f;
	Vector point = (origin - (forward * (behind_sentry_distance() + DISPENSER_OFFSET)));
	trace::CTraceFilterNoNPCsOrPlayers filter(me->GetEntity(), COLLISION_GROUP_NONE);
	Vector mins(-36.0f, -36.0f, 0.0f);
	Vector maxs(36.0f, 36.0f, 84.0f);
	trace_t tr;
	trace::hull(point, point, mins, maxs, MASK_SOLID, &filter, tr);
	Vector ground = trace::getground(point);

	if (!tr.DidHit() && fabsf(point.z - ground.z) < 10.0f)
	{
		if (me->IsDebugging(BOTDEBUG_TASKS))
		{
			debugoverlay->AddLineOverlay(point, point + Vector(0.0f, 0.0f, 64.0f), 0, 128, 0, true, 5.0f);
		}

		out = point;
		return true;
	}
	else if (me->IsDebugging(BOTDEBUG_TASKS))
	{
		debugoverlay->AddLineOverlay(point, point + Vector(0.0f, 0.0f, 64.0f), 200, 0, 0, true, 5.0f);
	}

	// Random nearby nav area if there is no space behind the sentry
	return GetRandomDispenserSpot(me, origin, out);
}

bool CTF2BotEngineerNestTask::GetRandomDispenserSpot(CTF2Bot* me, const Vector& Vecstart, Vector& out)
{
	CNavArea* start = TheNavMesh->GetNearestNavArea(Vecstart, max_dispenser_to_sentry_range(), false, true, me->GetCurrentTeamIndex());

	if (start == nullptr)
	{
		return false;
	}

	EngineerBuildableLocationCollector collector(me, static_cast<CTFNavArea*>(start));

	collector.SetTravelLimit(max_dispenser_to_sentry_range());
	collector.Execute();

	if (collector.IsCollectedAreasEmpty())
	{
		return false;
	}

	auto& areas = collector.GetCollectedAreas();

	CTFNavArea* buildGoal = nullptr;

	buildGoal = areas[randomgen->GetRandomInt<size_t>(0, areas.size() - 1)];

	out = buildGoal->GetRandomPoint();

	if (me->IsDebugging(BOTDEBUG_TASKS))
	{
		buildGoal->DrawFilled(0, 128, 64, 255, 5.0f, true);
	}

	return true;
}

