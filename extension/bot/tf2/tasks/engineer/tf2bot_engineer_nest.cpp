#include <extension.h>
#include <util/librandom.h>
#include <entities/tf2/tf_entities.h>
#include <mods/tf2/teamfortress2mod.h>
#include <bot/tf2/tf2bot.h>
#include <mods/tf2/nav/tfnavarea.h>
#include <mods/tf2/nav/tfnav_waypoint.h>
#include <navmesh/nav_pathfind.h>
#include <sdkports/sdk_traces.h>
#include <bot/tf2/tasks/tf2bot_find_ammo_task.h>
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

inline static Vector GetSpotBehindSentry(CBaseEntity* sentry)
{
	tfentities::HBaseObject sentrygun(sentry);
	Vector origin = sentrygun.GetAbsOrigin();
	QAngle angle = sentrygun.GetAbsAngles();
	Vector forward;

	AngleVectors(angle, &forward);
	forward.NormalizeInPlace();

	Vector point = (origin - (forward * CTF2BotEngineerNestTask::behind_sentry_distance()));
	return trace::getground(point);
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
	m_sentryWaypoint = nullptr;
	m_boredTimer.Invalidate();
}

TaskResult<CTF2Bot> CTF2BotEngineerNestTask::OnTaskUpdate(CTF2Bot* bot)
{
	bot->FindMyBuildings();

	AITask<CTF2Bot>* nestTask = NestTask(bot);

	if (nestTask != nullptr)
	{
		return PauseFor(nestTask, "Taking care of my own nest!");
	}

	auto threat = bot->GetSensorInterface()->GetPrimaryKnownThreat(true);

	if (threat.get() != nullptr)
	{
		m_boredTimer.Invalidate();
	}
	else
	{
		// max my metal
		if (bot->GetAmmoOfIndex(TeamFortress2::TF_AMMO_METAL) < 200)
		{
			return PauseFor(new CTF2BotFindAmmoTask(200), "Need more metal to build!");
		}

		if (!m_boredTimer.HasStarted())
		{
			m_boredTimer.StartRandom(90.0f, 180.0f);
		}
		else if (m_boredTimer.IsElapsed())
		{
			// TO-DO: If we got here, then the bot has been sitting on a nest for over a minute without a visible enemy
		}
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

QueryAnswerType CTF2BotEngineerNestTask::IsReady(CBaseBot* me)
{
	CTF2Bot* tf2bot = static_cast<CTF2Bot*>(me);

	if (tf2bot->GetMySentryGun() != nullptr && tf2bot->GetMyDispenser() != nullptr &&
		tf2bot->GetMyTeleporterEntrance() != nullptr && tf2bot->GetMyTeleporterExit() != nullptr)
	{
		return ANSWER_YES;
	}

	return ANSWER_NO;
}

AITask<CTF2Bot>* CTF2BotEngineerNestTask::NestTask(CTF2Bot* me)
{
	Vector goal; // used if no waypoints are available
	CTFWaypoint* wpt = nullptr;

	if (me->GetMyTeleporterEntrance() == nullptr)
	{
		m_boredTimer.Invalidate();
		if (FindSpotToBuildTeleEntrance(me, &wpt, goal))
		{
			if (wpt != nullptr)
			{
				return new CTF2BotEngineerBuildObjectTask(CTF2BotEngineerBuildObjectTask::OBJECT_TELEPORTER_ENTRANCE, wpt);
			}
			else
			{
				return new CTF2BotEngineerBuildObjectTask(CTF2BotEngineerBuildObjectTask::OBJECT_TELEPORTER_ENTRANCE, goal);
			}
		}
	}
	else if (me->GetMySentryGun() == nullptr)
	{
		m_boredTimer.Invalidate();
		if (FindSpotToBuildSentryGun(me, &wpt, goal))
		{
			if (wpt != nullptr)
			{
				return new CTF2BotEngineerBuildObjectTask(CTF2BotEngineerBuildObjectTask::OBJECT_SENTRYGUN, wpt);
			}
			else
			{
				return new CTF2BotEngineerBuildObjectTask(CTF2BotEngineerBuildObjectTask::OBJECT_SENTRYGUN, goal);
			}
		}
	}
	else if (me->GetMyDispenser() == nullptr)
	{
		m_boredTimer.Invalidate();
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
		m_boredTimer.Invalidate();
		if (FindSpotToBuildTeleExit(me, &wpt, goal))
		{
			if (wpt != nullptr)
			{
				return new CTF2BotEngineerBuildObjectTask(CTF2BotEngineerBuildObjectTask::OBJECT_TELEPORTER_EXIT, wpt);
			}
			else
			{
				return new CTF2BotEngineerBuildObjectTask(CTF2BotEngineerBuildObjectTask::OBJECT_TELEPORTER_EXIT, goal);
			}
		}
	}

	CBaseEntity* entity = me->GetMySentryGun();

	if (entity != nullptr)
	{
		tfentities::HBaseObject sentrygun(entity);

		if (!sentrygun.IsAtMaxLevel())
		{
			return new CTF2BotEngineerUpgradeObjectTask(entity);
		}
		else if (sentrygun.GetHealthPercentage() < 1.0f || sentrygun.IsSapped())
		{
			return new CTF2BotEngineerRepairObjectTask(entity);
		}
	}

	entity = me->GetMyDispenser();

	if (entity != nullptr)
	{
		tfentities::HBaseObject dispenser(entity);

		if (!dispenser.IsAtMaxLevel())
		{
			return new CTF2BotEngineerUpgradeObjectTask(entity);
		}
		else if (dispenser.GetHealthPercentage() < 1.0f || dispenser.IsSapped())
		{
			return new CTF2BotEngineerRepairObjectTask(entity);
		}
	}

	CBaseEntity* exit = me->GetMyTeleporterExit();
	CBaseEntity* entrance = me->GetMyTeleporterEntrance();

	if (exit != nullptr && entrance != nullptr)
	{
		// only upgrade teleporters after both exit and entrance are setup.
		if (me->GetRangeToSqr(exit) < me->GetRangeToSqr(entrance))
		{
			tfentities::HBaseObject tele(exit);

			if (!tele.IsAtMaxLevel())
			{
				return new CTF2BotEngineerUpgradeObjectTask(exit);
			}
		}
		else
		{
			tfentities::HBaseObject tele(entrance);

			if (!tele.IsAtMaxLevel())
			{
				return new CTF2BotEngineerUpgradeObjectTask(entrance);
			}
		}
	}

	if (exit != nullptr)
	{
		tfentities::HBaseObject tele(exit);

		if (tele.GetHealthPercentage() < 1.0f || tele.IsSapped())
		{
			return new CTF2BotEngineerRepairObjectTask(exit);
		}
	}
	if (entrance != nullptr)
	{
		tfentities::HBaseObject tele(entrance);

		if (tele.GetHealthPercentage() < 1.0f || tele.IsSapped())
		{
			return new CTF2BotEngineerRepairObjectTask(entrance);
		}
	}

	return nullptr;
}

bool CTF2BotEngineerNestTask::FindSpotToBuildSentryGun(CTF2Bot* me, CTFWaypoint** out, Vector& pos)
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
		m_sentryWaypoint = nullptr;
		return GetRandomSentrySpot(me, &pos);
	}

	CTFWaypoint* waypoint = librandom::utils::GetRandomElementFromVector(spots);

	*out = waypoint;
	m_sentryWaypoint = waypoint;
	return true;
}

bool CTF2BotEngineerNestTask::FindSpotToBuildDispenser(CTF2Bot* me, CTFWaypoint** out)
{
	std::vector<CTFWaypoint*> spots;
	auto& dispenserWaypoints = CTeamFortress2Mod::GetTF2Mod()->GetAllDispenserWaypoints();
	CBaseEntity* mySentry = me->GetMySentryGun();

	if (mySentry == nullptr)
	{
		return false;
	}

	Vector origin = UtilHelpers::getWorldSpaceCenter(mySentry);

	if (m_sentryWaypoint != nullptr && m_sentryWaypoint->HasConnections())
	{
		auto& connections = m_sentryWaypoint->GetConnections();

		for (auto& conn : connections)
		{
			CTFWaypoint* waypoint = static_cast<CTFWaypoint*>(conn.GetOther());

			if (waypoint->IsEnabled() && waypoint->IsAvailableToTeam(static_cast<int>(me->GetMyTFTeam())) && waypoint->CanBeUsedByBot(me) &&
				waypoint->GetTFHint() == CTFWaypoint::TFHINT_DISPENSER)
			{
				spots.push_back(waypoint);
			}
		}
	}

	// no useable dispenser waypoint found connected to the sentry waypoint.
	if (spots.empty())
	{
		for (auto waypoint : dispenserWaypoints)
		{

			if (waypoint->IsEnabled() && waypoint->DistanceTo(origin) <= max_dispenser_to_sentry_range() &&
				waypoint->IsAvailableToTeam(static_cast<int>(me->GetMyTFTeam())) && waypoint->CanBeUsedByBot(me))
			{
				spots.push_back(waypoint);
			}
		}
	}

	if (spots.empty())
	{
		return false;
	}

	*out = librandom::utils::GetRandomElementFromVector(spots);
	return true;
}

bool CTF2BotEngineerNestTask::FindSpotToBuildTeleEntrance(CTF2Bot* me, CTFWaypoint** out, Vector& pos)
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
		return GetRandomEntranceSpot(me, &pos);
	}

	*out = librandom::utils::GetRandomElementFromVector(spots);
	return true;
}

bool CTF2BotEngineerNestTask::FindSpotToBuildTeleExit(CTF2Bot* me, CTFWaypoint** out, Vector& pos)
{
	std::vector<CTFWaypoint*> spots;
	auto& teleexitWaypoints = CTeamFortress2Mod::GetTF2Mod()->GetAllTeleExitWaypoints();

	if (m_sentryWaypoint != nullptr && m_sentryWaypoint->HasConnections())
	{
		auto& connections = m_sentryWaypoint->GetConnections();

		for (auto& conn : connections)
		{
			CTFWaypoint* waypoint = static_cast<CTFWaypoint*>(conn.GetOther());

			if (waypoint->IsEnabled() && waypoint->IsAvailableToTeam(static_cast<int>(me->GetMyTFTeam())) && waypoint->CanBeUsedByBot(me) &&
				waypoint->GetTFHint() == CTFWaypoint::TFHINT_TELE_EXIT)
			{
				spots.push_back(waypoint);
			}
		}
	}

	// no exit waypoints connected to my sentry waypoint
	if (spots.empty())
	{
		for (auto waypoint : teleexitWaypoints)
		{
			if (waypoint->IsEnabled() && waypoint->IsAvailableToTeam(static_cast<int>(me->GetMyTFTeam())) && waypoint->CanBeUsedByBot(me))
			{
				spots.push_back(waypoint);
			}
		}
	}

	if (spots.empty())
	{
		if (GetRandomExitSpot(me, &pos))
		{
			return true;
		}

		return false;
	}

	*out = librandom::utils::GetRandomElementFromVector(spots);
	return true;
}

bool CTF2BotEngineerNestTask::FindSpotToBuildDispenser(CTF2Bot* me, Vector& out)
{
	CBaseEntity* mysentry = me->GetMySentryGun();

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

bool CTF2BotEngineerNestTask::GetRandomSentrySpot(CTF2Bot* me, Vector* out)
{
	Vector buildPoint = GetSentryNestBuildPos(me);

	CNavArea* start = TheNavMesh->GetNearestNavArea(buildPoint, 1024.0f, false, true, me->GetCurrentTeamIndex());

	if (start == nullptr)
	{
		return false;
	}

	EngineerBuildableLocationCollector collector(me, static_cast<CTFNavArea*>(start));

	collector.SetTravelLimit(4096.0f);
	collector.Execute();

	if (collector.IsCollectedAreasEmpty())
	{
		return false;
	}

	auto& areas = collector.GetCollectedAreas();

	CTFNavArea* buildGoal = nullptr;

	buildGoal = areas[randomgen->GetRandomInt<size_t>(0, areas.size() - 1)];

	*out = buildGoal->GetRandomPoint();

	if (me->IsDebugging(BOTDEBUG_TASKS))
	{
		buildGoal->DrawFilled(0, 128, 64, 255, 5.0f, true);
	}

	return true;
}

Vector CTF2BotEngineerNestTask::GetSentryNestBuildPos(CTF2Bot* me)
{
	Vector build = me->GetAbsOrigin();
	auto mod = CTeamFortress2Mod::GetTF2Mod();

	auto gm = mod->GetCurrentGameMode();

	switch (gm)
	{
	case TeamFortress2::GameModeType::GM_TC:
		[[fallthrough]];
	case TeamFortress2::GameModeType::GM_KOTH:
		[[fallthrough]];
	case TeamFortress2::GameModeType::GM_CP:
		[[fallthrough]];
	case TeamFortress2::GameModeType::GM_ADCP:
	{
		std::vector<CBaseEntity*> points;
		points.reserve(MAX_CONTROL_POINTS);
		mod->CollectControlPointsToAttack(me->GetMyTFTeam(), points);
		mod->CollectControlPointsToDefend(me->GetMyTFTeam(), points);

		if (points.empty())
		{
			return build;
		}

		CBaseEntity* pEntity = librandom::utils::GetRandomElementFromVector<CBaseEntity*>(points);

		build = UtilHelpers::getWorldSpaceCenter(pEntity);

		return build;
	}
	case TeamFortress2::GameModeType::GM_CTF:
	{
		edict_t* flag = me->GetFlagToDefend();
		return UtilHelpers::getEntityOrigin(flag);
	}
	default:
		break;
	}

	return build;
}

bool CTF2BotEngineerNestTask::GetRandomEntranceSpot(CTF2Bot* me, Vector* out)
{
	Vector buildPoint = me->GetHomePos();

	CNavArea* start = TheNavMesh->GetNearestNavArea(buildPoint, 1024.0f, false, true, me->GetCurrentTeamIndex());

	if (start == nullptr)
	{
		return false;
	}

	EngineerBuildableLocationCollector collector(me, static_cast<CTFNavArea*>(start));

	collector.SetTravelLimit(1500.0f);
	collector.Execute();

	if (collector.IsCollectedAreasEmpty())
	{
		return false;
	}

	auto& areas = collector.GetCollectedAreas();

	CTFNavArea* buildGoal = nullptr;

	buildGoal = areas[randomgen->GetRandomInt<size_t>(0, areas.size() - 1)];

	*out = buildGoal->GetRandomPoint();

	if (me->IsDebugging(BOTDEBUG_TASKS))
	{
		buildGoal->DrawFilled(0, 128, 64, 255, 5.0f, true);
	}

	return true;
}

bool CTF2BotEngineerNestTask::GetRandomExitSpot(CTF2Bot* me, Vector* out)
{
	CBaseEntity* mysentry = me->GetMySentryGun();

	if (mysentry == nullptr)
	{
		// Must build sentry first
		return false;
	}

	Vector vecstart = UtilHelpers::getEntityOrigin(mysentry);

	CNavArea* start = TheNavMesh->GetNearestNavArea(vecstart, 256.0f, false, true, me->GetCurrentTeamIndex());

	if (start == nullptr)
	{
		return false;
	}

	EngineerBuildableLocationCollector collector(me, static_cast<CTFNavArea*>(start));

	collector.SetTravelLimit(random_exit_spot_travel_limit());
	collector.Execute();

	if (collector.IsCollectedAreasEmpty())
	{
		return false;
	}

	auto& areas = collector.GetCollectedAreas();

	CTFNavArea* buildGoal = nullptr;

	buildGoal = areas[randomgen->GetRandomInt<size_t>(0, areas.size() - 1)];

	*out = buildGoal->GetRandomPoint();

	if (me->IsDebugging(BOTDEBUG_TASKS))
	{
		buildGoal->DrawFilled(0, 128, 64, 255, 5.0f, true);
	}

	return true;
}

