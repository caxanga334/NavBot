#include <extension.h>
#include <util/librandom.h>
#include <util/entprops.h>
#include <entities/tf2/tf_entities.h>
#include <mods/tf2/teamfortress2mod.h>
#include <mods/tf2/tf2lib.h>
#include <bot/tf2/tf2bot.h>
#include <bot/tf2/tf2bot_utils.h>
#include <mods/tf2/nav/tfnavmesh.h>
#include <mods/tf2/nav/tfnavarea.h>
#include <mods/tf2/nav/tfnav_waypoint.h>
#include <navmesh/nav_pathfind.h>
#include <sdkports/sdk_traces.h>
#include <bot/tf2/tasks/tf2bot_find_ammo_task.h>
#include "tf2bot_engineer_build_object.h"
#include "tf2bot_engineer_repair_object.h"
#include "tf2bot_engineer_upgrade_object.h"
#include "tf2bot_engineer_move_object.h"
#include "tf2bot_engineer_nest.h"

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
			Vector goal; // used if no waypoints are available
			CTFWaypoint* wpt = nullptr;

			if (FindSpotToBuildSentryGun(bot, &wpt, goal))
			{
				m_boredTimer.Invalidate();
				m_moveBuildingCheckTimer.Invalidate();
				if (wpt != nullptr)
				{
					return PauseFor(new CTF2BotEngineerMoveObjectTask(bot->GetMySentryGun(), wpt), "Moving my sentry gun!");
				}
				else
				{
					return PauseFor(new CTF2BotEngineerMoveObjectTask(bot->GetMySentryGun(), goal), "Moving my sentry gun!");
				}
			}
		}
	}

	auto moveTask = MoveBuildingsIfNeeded(bot);

	if (moveTask != nullptr)
	{
		return PauseFor(moveTask, "Moving my buildings!");
	}

	// At this point, the bot has fully upgraded everything, time to idle near the sentry

	m_goal = GetSpotBehindSentry(bot->GetMySentryGun());

	if (bot->GetRangeTo(m_goal) > 48.0f)
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
	if (pastTask)
	{
		CTF2BotEngineerMoveObjectTask* moveTask = dynamic_cast<CTF2BotEngineerMoveObjectTask*>(pastTask);

		if (moveTask != nullptr)
		{
			m_moveBuildingCheckTimer.StartRandom(4.0f, 10.0f);
		}
	}

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

TaskEventResponseResult<CTF2Bot> CTF2BotEngineerNestTask::OnRoundStateChanged(CTF2Bot* bot)
{
	if (CTeamFortress2Mod::GetTF2Mod()->GetCurrentGameMode() == TeamFortress2::GameModeType::GM_MVM)
	{
		if (entprops->GameRules_GetRoundState() == RoundState_BetweenRounds)
		{
			m_boredTimer.Start(0.5f);
			m_moveBuildingCheckTimer.Start(5.0f);
		}
	}

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
		m_moveBuildingCheckTimer.StartRandom(60.0, 95.0f);
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
		m_moveBuildingCheckTimer.StartRandom(60.0, 95.0f);
		if (FindSpotToBuildSentryGun(me, &wpt, goal))
		{
			if (wpt != nullptr)
			{
				m_sentryWaypoint = wpt;
				return new CTF2BotEngineerBuildObjectTask(CTF2BotEngineerBuildObjectTask::OBJECT_SENTRYGUN, wpt);
			}
			else
			{
				m_sentryWaypoint = nullptr;
				return new CTF2BotEngineerBuildObjectTask(CTF2BotEngineerBuildObjectTask::OBJECT_SENTRYGUN, goal);
			}
		}
	}
	else if (me->GetMyDispenser() == nullptr)
	{
		m_boredTimer.Invalidate();
		m_moveBuildingCheckTimer.StartRandom(60.0, 95.0f);
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
		m_moveBuildingCheckTimer.StartRandom(60.0, 95.0f);
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

		if (m_sentryEnemyScanTimer.IsElapsed())
		{
			m_sentryEnemyScanTimer.Start(1.0f);
			int enemy = INVALID_EHANDLE_INDEX;

			if (entprops->GetEntPropEnt(sentrygun.GetIndex(), Prop_Send, "m_hEnemy", enemy))
			{
				if (enemy != INVALID_EHANDLE_INDEX)
				{
					CBaseEntity* pEnemy = gamehelpers->ReferenceToEntity(enemy);

					if (pEnemy)
					{
						auto known = me->GetSensorInterface()->AddKnownEntity(pEnemy);
						known->UpdatePosition();

						me->GetControlInterface()->AimAt(known->GetLastKnownPosition(), IPlayerController::LOOK_ALERT, 2.0f, "Looking at my sentry's current enemy!");
					}
				}
			}
		}

		if (!sentrygun.IsAtMaxLevel())
		{
			return new CTF2BotEngineerUpgradeObjectTask(entity);
		}
		else if (sentrygun.GetHealthPercentage() < 1.0f || sentrygun.IsSapped())
		{
			return new CTF2BotEngineerRepairObjectTask(entity);
		}
		else
		{
			int shells = 999;
			entprops->GetEntProp(sentrygun.GetIndex(), Prop_Send, "m_iAmmoShells", shells);
			int rockets = 999;
			entprops->GetEntProp(sentrygun.GetIndex(), Prop_Send, "m_iAmmoRockets", rockets);

			if (shells <= 50 || rockets <= 5)
			{
				// sentry needs ammo
				return new CTF2BotEngineerRepairObjectTask(entity);
			}
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
	Vector spot;

	if (tf2botutils::GetSentrySearchStartPosition(me, spot))
	{
		*out = tf2botutils::SelectWaypointForSentryGun(me, tf2botutils::GetSentrySearchMaxRange(true), &spot);

		if (*out != nullptr)
		{
			return true;
		}

		CTFNavArea* area = tf2botutils::FindRandomNavAreaToBuild(me, tf2botutils::GetSentrySearchMaxRange(false), &spot, false);

		if (!area)
		{
			return false;
		}

		pos = area->GetCenter();
		return true;
	}

	return false;
}

bool CTF2BotEngineerNestTask::FindSpotToBuildDispenser(CTF2Bot* me, CTFWaypoint** out) const
{
	CBaseEntity* mySentry = me->GetMySentryGun();

	if (mySentry == nullptr)
	{
		return false;
	}

	Vector origin = UtilHelpers::getWorldSpaceCenter(mySentry);
	*out = tf2botutils::SelectWaypointForDispenser(me, -1.0f, &origin, m_sentryWaypoint);

	return (*out != nullptr);
}

bool CTF2BotEngineerNestTask::FindSpotToBuildTeleEntrance(CTF2Bot* me, CTFWaypoint** out, Vector& pos)
{
	*out = tf2botutils::SelectWaypointForTeleEntrance(me);

	if (*out != nullptr)
	{
		return true;
	}

	// bot spawned recently, probably inside a spawnroom, use their position as the search start.
	if (me->GetTimeSinceLastSpawn() < 1.0f)
	{
		CTFNavArea* area = tf2botutils::FindRandomNavAreaToBuild(me, 2048.0f, nullptr, true);

		if (area)
		{
			pos = area->GetCenter();
			return true;
		}
	}
	else
	{
		// use an active spawnpoint as a base for our search
		CBaseEntity* spawnpoint = tf2lib::GetFirstValidSpawnPointForTeam(me->GetMyTFTeam());

		if (spawnpoint)
		{
			const Vector& searchStart = UtilHelpers::getEntityOrigin(spawnpoint);
			CTFNavArea* area = tf2botutils::FindRandomNavAreaToBuild(me, 2048.0f, &searchStart, true);

			if (area)
			{
				pos = area->GetCenter();
				return true;
			}
		}
	}

	return false;
}

bool CTF2BotEngineerNestTask::FindSpotToBuildTeleExit(CTF2Bot* me, CTFWaypoint** out, Vector& pos)
{
	CBaseEntity* mysentry = me->GetMySentryGun();

	if (mysentry == nullptr)
	{
		// Must build sentry first
		return false;
	}

	const Vector& sentryPos = UtilHelpers::getEntityOrigin(mysentry);

	*out = tf2botutils::SelectWaypointForTeleExit(me, 2048.0f, &sentryPos, m_sentryWaypoint);

	if (*out != nullptr)
	{
		return true;
	}

	CTFNavArea* area = tf2botutils::FindRandomNavAreaToBuild(me, 1500.0f, &sentryPos, true);

	if (area)
	{
		pos = area->GetCenter();
		return true;
	}

	return false;
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
	CTFNavArea* area = tf2botutils::FindRandomNavAreaToBuild(me, 1024.0f, &origin, false);

	if (area)
	{
		out = area->GetCenter();
		return true;
	}
	
	return false;
}

AITask<CTF2Bot>* CTF2BotEngineerNestTask::MoveBuildingsIfNeeded(CTF2Bot* bot)
{
	if (m_moveBuildingCheckTimer.IsElapsed())
	{
		m_moveBuildingCheckTimer.StartRandom(2.0f, 5.0f);

		CBaseEntity* entrance = bot->GetMyTeleporterEntrance();

		if (entrance)
		{
			Vector pos = UtilHelpers::getEntityOrigin(entrance);
			Vector home;
			CBaseEntity* spawn = tf2lib::GetFirstValidSpawnPointForTeam(bot->GetMyTFTeam());

			if (spawn)
			{
				home = UtilHelpers::getEntityOrigin(spawn);
			}
			else
			{
				home = bot->GetHomePos();
			}

			float range = (home - pos).Length();

			if (range > 1200.0f)
			{
				Vector goal; // used if no waypoints are available
				CTFWaypoint* wpt = nullptr;

				if (FindSpotToBuildTeleEntrance(bot, &wpt, goal))
				{
					if (wpt != nullptr)
					{
						return new CTF2BotEngineerMoveObjectTask(entrance, wpt);
					}
					else
					{
						return new CTF2BotEngineerMoveObjectTask(entrance, goal);
					}
				}
			}
		}

		CBaseEntity* sentrygun = bot->GetMySentryGun();
		CBaseEntity* dispenser = bot->GetMyDispenser();

		if (dispenser && sentrygun)
		{
			const Vector& vSentry = UtilHelpers::getEntityOrigin(sentrygun);
			const Vector& vDispenser = UtilHelpers::getEntityOrigin(dispenser);
			float range = (vSentry - vDispenser).Length();

			if (range > max_dispenser_to_sentry_range() + 512.0f)
			{
				Vector goal; // used if no waypoints are available
				CTFWaypoint* wpt = nullptr;

				if (FindSpotToBuildDispenser(bot, &wpt))
				{
					return new CTF2BotEngineerMoveObjectTask(dispenser, wpt);
				}
				else if (FindSpotToBuildDispenser(bot, goal))
				{
					return new CTF2BotEngineerMoveObjectTask(dispenser, goal);
				}
			}
		}

		CBaseEntity* exit = bot->GetMyTeleporterExit();

		if (exit && sentrygun)
		{
			const Vector& vSentry = UtilHelpers::getEntityOrigin(sentrygun);
			const Vector& vExit = UtilHelpers::getEntityOrigin(exit);
			float range = (vSentry - vExit).Length();

			if (range > random_exit_spot_travel_limit() + 512.0f)
			{
				Vector goal; // used if no waypoints are available
				CTFWaypoint* wpt = nullptr;

				if (FindSpotToBuildTeleExit(bot, &wpt, goal))
				{
					if (wpt != nullptr)
					{
						return new CTF2BotEngineerMoveObjectTask(exit, wpt);
					}
					else
					{
						return new CTF2BotEngineerMoveObjectTask(exit, goal);
					}
				}
			}
		}
	}

	return nullptr;
}

