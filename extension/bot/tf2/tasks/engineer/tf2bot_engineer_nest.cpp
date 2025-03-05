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
	m_justMovedSentry = false;
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
		m_noThreatTimer.Invalidate();
	}
	else
	{
		// max my metal
		if (bot->GetAmmoOfIndex(TeamFortress2::TF_AMMO_METAL) < 200)
		{
			return PauseFor(new CTF2BotFindAmmoTask(200), "Need more metal to build!");
		}

		CBaseEntity* sentryGun = bot->GetMySentryGun();

		if (sentryGun && ShouldMoveSentryGun())
		{
			CTFWaypoint* waypoint = nullptr;
			CTFNavArea* area = nullptr;

			if (tf2botutils::FindSpotToBuildSentryGun(bot, &waypoint, &area))
			{
				m_justMovedSentry = true;
				if (waypoint)
				{
					return PauseFor(new CTF2BotEngineerMoveObjectTask(sentryGun, waypoint), "Moving my sentry gun!");
				}
				else
				{
					return PauseFor(new CTF2BotEngineerMoveObjectTask(sentryGun, area->GetCenter()), "Moving my sentry gun!");
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
			m_noThreatTimer.Start(0.1f);
			m_moveBuildingCheckTimer.Start(0.2f);
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

bool CTF2BotEngineerNestTask::ShouldMoveSentryGun()
{
	if (!m_noThreatTimer.HasStarted())
	{
		m_noThreatTimer.Start(CBaseBot::s_botrng.GetRandomReal<float>(90.0f, 180.0f));
		return false;
		
	}
	else if (m_noThreatTimer.IsElapsed())
	{
		// it has been a while since I saw an enemy, time to move my sentry gun.
		m_noThreatTimer.Start(CBaseBot::s_botrng.GetRandomReal<float>(90.0f, 180.0f)); // start the timer again so the bot doesn't enter a move loop
		return true;
	}

	return false;
}

AITask<CTF2Bot>* CTF2BotEngineerNestTask::NestTask(CTF2Bot* me)
{
	Vector goal; // used if no waypoints are available
	CTFWaypoint* wpt = nullptr;
	CTFNavArea* area = nullptr;

	if (me->GetMyTeleporterEntrance() == nullptr)
	{
		if (tf2botutils::FindSpotToBuildTeleEntrance(me, &wpt, &area))
		{
			if (wpt != nullptr)
			{
				return new CTF2BotEngineerBuildObjectTask(CTF2BotEngineerBuildObjectTask::OBJECT_TELEPORTER_ENTRANCE, wpt);
			}
			else
			{
				return new CTF2BotEngineerBuildObjectTask(CTF2BotEngineerBuildObjectTask::OBJECT_TELEPORTER_ENTRANCE, area->GetCenter());
			}
		}
	}
	else if (me->GetMySentryGun() == nullptr)
	{
		if (tf2botutils::FindSpotToBuildSentryGun(me, &wpt, &area))
		{
			if (wpt != nullptr)
			{
				m_sentryWaypoint = wpt;
				return new CTF2BotEngineerBuildObjectTask(CTF2BotEngineerBuildObjectTask::OBJECT_SENTRYGUN, wpt);
			}
			else
			{
				m_sentryWaypoint = nullptr;
				return new CTF2BotEngineerBuildObjectTask(CTF2BotEngineerBuildObjectTask::OBJECT_SENTRYGUN, area->GetCenter());
			}
		}
	}
	else if (me->GetMyDispenser() == nullptr)
	{
		bool canplacebehind = tf2botutils::FindSpotToBuildDispenserBehindSentry(me, goal);

		if (tf2botutils::FindSpotToBuildDispenser(me, &wpt, &area, m_sentryWaypoint))
		{
			if (wpt != nullptr) // place at waypoints if available
			{
				return new CTF2BotEngineerBuildObjectTask(CTF2BotEngineerBuildObjectTask::OBJECT_DISPENSER, wpt);
			}
			else if (canplacebehind) // place behind the sentry gun
			{
				return new CTF2BotEngineerBuildObjectTask(CTF2BotEngineerBuildObjectTask::OBJECT_DISPENSER, goal);
			}
			else // place in a random area near the sentry gun
			{
				return new CTF2BotEngineerBuildObjectTask(CTF2BotEngineerBuildObjectTask::OBJECT_DISPENSER, area->GetCenter());
			}
		}
	}
	else if (me->GetMyTeleporterExit() == nullptr)
	{
		if (tf2botutils::FindSpotToBuildTeleExit(me, &wpt, &area, m_sentryWaypoint))
		{
			if (wpt != nullptr)
			{
				return new CTF2BotEngineerBuildObjectTask(CTF2BotEngineerBuildObjectTask::OBJECT_TELEPORTER_EXIT, wpt);
			}
			else
			{
				return new CTF2BotEngineerBuildObjectTask(CTF2BotEngineerBuildObjectTask::OBJECT_TELEPORTER_EXIT, area->GetCenter());
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

AITask<CTF2Bot>* CTF2BotEngineerNestTask::MoveBuildingsIfNeeded(CTF2Bot* bot)
{
	if (!m_moveBuildingCheckTimer.IsElapsed())
	{
		return nullptr;
	}

	m_moveBuildingCheckTimer.Start(CBaseBot::s_botrng.GetRandomReal<float>(5.0f, 15.0f));

	CBaseEntity* sentry = bot->GetMySentryGun();
	CBaseEntity* dispenser = bot->GetMyDispenser();
	CBaseEntity* entrance = bot->GetMyTeleporterEntrance();
	CBaseEntity* exit = bot->GetMyTeleporterExit();

	// must have all 4 buildings
	if (sentry == nullptr || dispenser == nullptr || entrance == nullptr || exit == nullptr)
	{
		m_moveBuildingCheckTimer.Start(90.0f); // must build and upgrade, wait a longer time
		return nullptr;
	}

	constexpr auto DISTANCE_MARGIN = 200.0f;
	const CTF2ModSettings* settings = CTeamFortress2Mod::GetTF2Mod()->GetTF2ModSettings();
	const Vector& sentryPos = UtilHelpers::getEntityOrigin(sentry);
	const Vector& dispenserPos = UtilHelpers::getEntityOrigin(dispenser);
	CTFWaypoint* waypoint = nullptr;
	CTFNavArea* area = nullptr;

	// dispenser first
	float range = (sentryPos - dispenserPos).Length();

	if (m_justMovedSentry || range > (settings->GetEngineerNestDispenserRange() + DISTANCE_MARGIN))
	{
		m_justMovedSentry = false;
		Vector spot;

		// time to move
		if (tf2botutils::FindSpotToBuildDispenserBehindSentry(bot, spot))
		{
			return new CTF2BotEngineerMoveObjectTask(dispenser, spot);
		}
		else if (tf2botutils::FindSpotToBuildDispenser(bot, &waypoint, &area, m_sentryWaypoint))
		{
			if (waypoint)
			{
				return new CTF2BotEngineerMoveObjectTask(dispenser, waypoint);
			}
			else
			{
				return new CTF2BotEngineerMoveObjectTask(dispenser, area->GetCenter());
			}
		}
	}
	
	const Vector& exitPos = UtilHelpers::getEntityOrigin(exit);
	range = (sentryPos - exitPos).Length();

	if (range > (settings->GetEngineerNestExitRange() + DISTANCE_MARGIN))
	{
		if (tf2botutils::FindSpotToBuildTeleExit(bot, &waypoint, &area, m_sentryWaypoint))
		{
			if (waypoint)
			{
				return new CTF2BotEngineerMoveObjectTask(exit, waypoint);
			}
			else
			{
				return new CTF2BotEngineerMoveObjectTask(exit, area->GetCenter());
			}
		}
	}

	const Vector& entrancePos = UtilHelpers::getEntityOrigin(entrance);
	CBaseEntity* spawnPoint = tf2lib::GetFirstValidSpawnPointForTeam(bot->GetMyTFTeam());
	const Vector& spawnPos = UtilHelpers::getEntityOrigin(spawnPoint);
	range = (spawnPos - entrancePos).Length();

	if (range > (settings->GetEntranceSpawnRange() + DISTANCE_MARGIN))
	{
		if (tf2botutils::FindSpotToBuildTeleExit(bot, &waypoint, &area, m_sentryWaypoint))
		{
			if (waypoint)
			{
				return new CTF2BotEngineerMoveObjectTask(entrance, waypoint);
			}
			else
			{
				return new CTF2BotEngineerMoveObjectTask(entrance, area->GetCenter());
			}
		}
	}

	return nullptr;
}

