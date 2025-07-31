#include NAVBOT_PCH_FILE
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
#include <bot/bot_shared_utils.h>
#include <bot/tasks_shared/bot_shared_roam.h>
#include <bot/tasks_shared/bot_shared_go_to_position.h>
#include "tf2bot_engineer_build_object.h"
#include "tf2bot_engineer_repair_object.h"
#include "tf2bot_engineer_upgrade_object.h"
#include "tf2bot_engineer_move_object.h"
#include "tf2bot_engineer_nest.h"

#if SOURCE_ENGINE == SE_TF2
static ConVar sm_navbot_tf_engineer_vc_listen_range("sm_navbot_tf_engineer_vc_listen_range", "1500", FCVAR_GAMEDLL, "Maximum distance engineers will listen for teammates voice commands", true, 0.0f, true, 8192.0f);
#endif // SOURCE_ENGINE == SE_TF2


inline static Vector GetSpotBehindSentry(CBaseEntity* sentry)
{
	const Vector& origin = UtilHelpers::getEntityOrigin(sentry);
	const QAngle& angle = UtilHelpers::getEntityAngles(sentry);
	Vector forward;

	AngleVectors(angle, &forward);
	forward.NormalizeInPlace();

	Vector point = (origin - (forward * CTF2BotEngineerNestTask::behind_sentry_distance()));
	return trace::getground(point);
}

CTF2BotEngineerNestTask::CTF2BotEngineerNestTask() :
	m_goal(vec3_origin), m_sentryBuildPos(vec3_origin), m_myOrigin(vec3_origin)
{
	m_sentryWaypoint = nullptr;
	m_checkDispenser = false;
	m_findNewNestPos = true;
	m_sentryWasBuilt = false;
	m_checkEntrance = false;
	m_checkExit = false;
	m_mysentry = nullptr;
	m_mydispenser = nullptr;
	m_myentrance = nullptr;
	m_myexit = nullptr;
	m_sentryKills = 0;
	m_teleUses = 0;
	m_myteam = 0;
	m_maxHelpAllyRange = CTeamFortress2Mod::GetTF2Mod()->GetTF2ModSettings()->GetEngineerHelpAllyMaxRange();
}

TaskResult<CTF2Bot> CTF2BotEngineerNestTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	m_myteam = static_cast<int>(bot->GetMyTFTeam());
	bot->FindMyBuildings();
	CBaseEntity* mysentry = bot->GetMySentryGun();

	CTeamFortress2Mod* tf2mod = CTeamFortress2Mod::GetTF2Mod();

	for (auto wpt : tf2mod->GetAllSentryWaypoints())
	{
		if (wpt->IsCurrentUser(bot))
		{
			m_sentryWaypoint = wpt;
			m_sentryBuildPos = wpt->GetOrigin();
			break;
		}
	}

	if (!mysentry)
	{
		if (m_sentryWaypoint)
		{
			m_sentryWaypoint->StopUsing(bot);
			m_sentryWaypoint = nullptr;
		}

		m_sentryKills = 0;
		m_teleUses = 0;
	}
	else
	{
		m_findNewNestPos = false; // already have a sentry
		m_sentryWasBuilt = true;
		m_sentryBuildPos = UtilHelpers::getEntityOrigin(mysentry);
		m_sentryKills = tf2lib::GetSentryKills(mysentry) + tf2lib::GetSentryAssists(mysentry);
	}

	CBaseEntity* entrance = bot->GetMyTeleporterEntrance();

	if (entrance)
	{
		m_teleUses = tf2lib::GetTeleporterUses(entrance);
	}

	m_moveTimer.Start(tf2mod->GetTF2ModSettings()->GetEngineerMoveCheckInterval());

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotEngineerNestTask::OnTaskUpdate(CTF2Bot* bot)
{
	bot->GetMyBuildings(&m_mysentry, &m_mydispenser, &m_myentrance, &m_myexit);

	// Makes high team work recently spawned bots priorize helping allies setup their teleporters
	if (bot->GetTimeSinceLastSpawn() <= 5.0f && bot->GetDifficultyProfile()->GetTeamwork() > 50)
	{
		AITask<CTF2Bot>* helpAllyTask = GetHelpFriendlyEngineerTask(bot);

		if (helpAllyTask)
		{
			return PauseFor(helpAllyTask, "Helping another engineer!");
		}
	}

	AITask<CTF2Bot>* buildTask = GetBuildTask(bot);

	if (buildTask)
	{
		m_moveTimer.Start(CTeamFortress2Mod::GetTF2Mod()->GetTF2ModSettings()->GetEngineerMoveCheckInterval());
		return PauseFor(buildTask, "Taking care of my own buildings!");
	}

	if (bot->GetDifficultyProfile()->GetTeamwork() > 5)
	{
		AITask<CTF2Bot>* helpAllyTask = GetHelpFriendlyEngineerTask(bot);

		if (helpAllyTask)
		{
			if (m_moveTimer.IsElapsed())
			{
				m_moveTimer.StartRandom(10.0f, 20.0f);
			}

			return PauseFor(helpAllyTask, "Helping another engineer!");
		}
	}

	AITask<CTF2Bot>* moveTask = GetMoveBuildingsTask(bot);

	if (moveTask)
	{
		return PauseFor(moveTask, "Moving my builings!");
	}

	if (!m_mysentry)
	{
		// Nothing to do and without a sentry, probably out of metal and no ammo source nearby, just roam
		return PauseFor(new CBotSharedRoamTask<CTF2Bot, CTF2BotPathCost>(bot, 4096.0f, true), "Nothing to do, roaming!");
	}
	else
	{
		if (m_scanForEnemiesTimer.IsElapsed())
		{
			m_scanForEnemiesTimer.Start(0.5f);

			CBaseEntity* enemy = entprops->GetEntPropEnt(m_mysentry, Prop_Send, "m_hEnemy");

			if (enemy)
			{
				CKnownEntity* known = bot->GetSensorInterface()->AddKnownEntity(enemy);
				known->UpdatePosition();
				bot->GetControlInterface()->AimAt(UtilHelpers::getWorldSpaceCenter(enemy), IPlayerController::LOOK_ALERT, 1.0f, "Looking at my sentry current enemy!");
			}
		}

		const Vector& pos = UtilHelpers::getEntityOrigin(m_mysentry);

		if (bot->GetRangeTo(pos) > 260.0f)
		{
			return PauseFor(new CBotSharedGoToPositionTask<CTF2Bot, CTF2BotPathCost>(bot, pos, "GoBehindSentry", false, true, true, 200.0f), "Moving back to the nest!");
		}
	}

	// TODO: Do something other than idling near the sentry after everything has been setup

	return Continue();
}

bool CTF2BotEngineerNestTask::OnTaskPause(CTF2Bot* bot, AITask<CTF2Bot>* nextTask)
{
	m_nav.Invalidate();
	m_nav.ForceRepath();
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

TaskEventResponseResult<CTF2Bot> CTF2BotEngineerNestTask::OnRoundStateChanged(CTF2Bot* bot)
{
	if (CTeamFortress2Mod::GetTF2Mod()->GetCurrentGameMode() == TeamFortress2::GameModeType::GM_MVM)
	{
		m_roundStateTimer.Start(5.0f);
	}

	return TryContinue();
}

TaskEventResponseResult<CTF2Bot> CTF2BotEngineerNestTask::OnVoiceCommand(CTF2Bot* bot, CBaseEntity* subject, int command)
{
#if SOURCE_ENGINE == SE_TF2
	if (bot->GetDifficultyProfile()->GetGameAwareness() <= 15)
	{
		return TryContinue();
	}

	TeamFortress2::VoiceCommandsID vcmd = static_cast<TeamFortress2::VoiceCommandsID>(command);

	switch (vcmd)
	{
	case TeamFortress2::VC_PLACETPHERE:
		[[fallthrough]];
	case TeamFortress2::VC_PLACEDISPENSERHERE:
		[[fallthrough]];
	case TeamFortress2::VC_PLACESENTRYHERE:
		break;
	default:
		return TryContinue();
	}

	if (m_respondToVCTimer.IsElapsed() && subject && tf2lib::GetEntityTFTeam(subject) == bot->GetMyTFTeam() &&
		bot->GetRangeTo(subject) <= sm_navbot_tf_engineer_vc_listen_range.GetFloat())
	{
		if (bot->GetDifficultyProfile()->GetTeamwork() >= CBaseBot::s_botrng.GetRandomInt<int>(1, 100))
		{
			m_respondToVCTimer.StartRandom(30.0f, 50.0f);
			bot->SendVoiceCommand(TeamFortress2::VoiceCommandsID::VC_YES);
			const Vector& origin = UtilHelpers::getEntityOrigin(subject);

			switch (vcmd)
			{
			case TeamFortress2::VC_PLACETPHERE:
			{
				CBaseEntity* exit = bot->GetMyTeleporterExit();

				if (exit)
				{
					return TryPauseFor(new CTF2BotEngineerMoveObjectTask(exit, origin), PRIORITY_MEDIUM, "Teammate needs a teleporter here!");
				}

				return TryPauseFor(new CTF2BotEngineerBuildObjectTask(CTF2BotEngineerBuildObjectTask::eObjectType::OBJECT_TELEPORTER_EXIT, origin), PRIORITY_MEDIUM, "Teammate needs a teleporter here!");
			}
			case TeamFortress2::VC_PLACEDISPENSERHERE:
			{
				CBaseEntity* dispenser = bot->GetMyDispenser();

				if (dispenser)
				{
					return TryPauseFor(new CTF2BotEngineerMoveObjectTask(dispenser, origin), PRIORITY_MEDIUM, "Teammate needs a dispenser here!");
				}

				return TryPauseFor(new CTF2BotEngineerBuildObjectTask(CTF2BotEngineerBuildObjectTask::eObjectType::OBJECT_DISPENSER, origin), PRIORITY_MEDIUM, "Teammate needs a dispenser here!");
			}
			case TeamFortress2::VC_PLACESENTRYHERE:
			{
				CBaseEntity* sentry = bot->GetMySentryGun();

				if (sentry)
				{
					return TryPauseFor(new CTF2BotEngineerMoveObjectTask(sentry, origin), PRIORITY_MEDIUM, "Teammate needs a sentry here!");
				}

				return TryPauseFor(new CTF2BotEngineerBuildObjectTask(CTF2BotEngineerBuildObjectTask::eObjectType::OBJECT_SENTRYGUN, origin), PRIORITY_MEDIUM, "Teammate needs a sentry here!");
			}
			case TeamFortress2::VC_GOGOGO:
				[[fallthrough]];
			case TeamFortress2::VC_MOVEUP:
				[[fallthrough]];
			case TeamFortress2::VC_GOLEFT:
				[[fallthrough]];
			case TeamFortress2::VC_GORIGHT:
			{
				ForceMoveBuildings(2.0f);
				m_respondToVCTimer.Start(15.0f);
				break;
			}
			default:
				m_respondToVCTimer.Start(5.0f); // command that we ignore
				return TryContinue();
			}
		}
		else
		{
			bot->SendVoiceCommand(TeamFortress2::VoiceCommandsID::VC_NO);
		}
	}

#endif // SOURCE_ENGINE == SE_TF2

	return TryContinue();
}

TaskEventResponseResult<CTF2Bot> CTF2BotEngineerNestTask::OnControlPointCaptured(CTF2Bot* bot, CBaseEntity* point)
{
	ForceMoveBuildings(10.0f);

	return TryContinue();
}

TaskEventResponseResult<CTF2Bot> CTF2BotEngineerNestTask::OnControlPointLost(CTF2Bot* bot, CBaseEntity* point)
{
	ForceMoveBuildings(10.0f);

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

bool CTF2BotEngineerNestTask::operator()(int index, edict_t* edict, CBaseEntity* entity)
{
	if (entity)
	{
		tfentities::HBaseObject object(entity);

		if ((m_myOrigin - object.WorldSpaceCenter()).Length() > m_maxHelpAllyRange)
		{
			return true;
		}

		// must be from the same team
		if (object.GetTeam() != m_myteam || object.IsPlacing() || object.IsBeingCarried() || object.IsRedeploying())
		{
			return true;
		}

		if (object.IsSapped())
		{
			m_allyObjects.emplace_back(ALLY_ACTION_SAPPED, entity);
			return true;
		}

		if (object.GetPercentageConstructed() > 0.99f && object.GetHealthPercentage() < 0.9f)
		{
			m_allyObjects.emplace_back(ALLY_ACTION_REPAIR, entity);
			return true;
		}

		if (!object.IsAtMaxLevel())
		{
			m_allyObjects.emplace_back(ALLY_ACTION_UPGRADE, entity);
			return true;
		}
	}

	return true;
}

AITask<CTF2Bot>* CTF2BotEngineerNestTask::GetBuildTask(CTF2Bot* me)
{
	constexpr int MIN_METAL_TO_BUILD = 140;

	if (me->GetAmmoOfIndex(TeamFortress2::TF_AMMO_METAL) < MIN_METAL_TO_BUILD)
	{
		CBaseEntity* ammoSource = nullptr;
		if (CTF2BotFindAmmoTask::IsPossible(me, &ammoSource))
		{
			return new CTF2BotFindAmmoTask(ammoSource, MIN_METAL_TO_BUILD);
		}
		else
		{
			if (me->IsDebugging(BOTDEBUG_ERRORS))
			{
				const Vector origin = me->GetAbsOrigin();

				me->DebugPrintToConsole(255, 0, 0, "%s ERROR: CTF2BotEngineerNestTask::GetBuildTask bot needs more metal to build but CTF2BotFindAmmoTask::IsPossible returned false! Team: %i Position: %g %g %g", me->GetDebugIdentifier(), me->GetCurrentTeamIndex(), origin.x, origin.y, origin.z);
			}

			return nullptr; // need more metal but find ammo failed
		}
	}

	CTFWaypoint* buildwpt = nullptr;
	CTFNavArea* buildarea = nullptr;
	CTeamFortress2Mod* tf2mod = CTeamFortress2Mod::GetTF2Mod();
	auto role = tf2mod->GetTeamRole(me->GetMyTFTeam());

	if (!m_sentryWasBuilt && m_mysentry)
	{
		m_sentryWasBuilt = true; // remember that we built a sentry
	}

	// No entrance, build it first
	if (!m_myentrance)
	{
		m_teleUses = 0; // reset this

		if (tf2botutils::FindSpotToBuildTeleEntrance(me, &buildwpt, &buildarea))
		{
			if (buildwpt)
			{
				return new CTF2BotEngineerBuildObjectTask(CTF2BotEngineerBuildObjectTask::OBJECT_TELEPORTER_ENTRANCE, buildwpt);
			}
			else
			{
				return new CTF2BotEngineerBuildObjectTask(CTF2BotEngineerBuildObjectTask::OBJECT_TELEPORTER_ENTRANCE, buildarea->GetRandomPoint());
			}
		}
	}
	else
	{
		// entrance is built

		if (role == TeamFortress2::TeamRoles::TEAM_ROLE_ATTACKERS && !m_myexit && CBaseBot::s_botrng.GetRandomChance(35))
		{
			// bots on attack roles have a random chance to priorize upgrading their teleporter entrance before building anything else
			tfentities::HBaseObject object(m_myentrance);

			if (!object.IsAtMaxLevel())
			{
				return new CTF2BotEngineerUpgradeObjectTask(m_myentrance);
			}
		}

		if (!m_mysentry) // no sentry gun
		{
			m_sentryKills = 0; // reset this

			// sentry was destroyed, find a new location
			if (m_sentryWasBuilt)
			{
				m_sentryWasBuilt = false;
				m_findNewNestPos = true;
			}

			// time to find a new spot to build
			if (m_findNewNestPos)
			{
				if (m_sentryWaypoint)
				{
					m_sentryWaypoint->StopUsing(me);
					m_sentryWaypoint = nullptr;
				}

				if (tf2botutils::FindSpotToBuildSentryGun(me, &buildwpt, &buildarea))
				{
					m_findNewNestPos = false;

					if (buildwpt)
					{
						m_sentryWaypoint = buildwpt;
						m_sentryBuildPos = buildwpt->GetOrigin();
						m_sentryWaypoint->Use(me, 90.0f); // lock the waypoint
					}
					else
					{
						m_sentryWaypoint = nullptr;
						m_sentryBuildPos = buildarea->GetRandomPoint();
					}
				}
				else
				{
					if (me->IsDebugging(BOTDEBUG_ERRORS))
					{
						const Vector origin = me->GetAbsOrigin();

						me->DebugPrintToConsole(255, 0, 0, "%s ERROR: FindSpotToBuildSentryGun failed! Team: %i Position: %g %g %g", me->GetDebugIdentifier(),
							me->GetCurrentTeamIndex(), origin.x, origin.y, origin.z);
					}

					return nullptr;
				}
			}

			// Priorize the teleporter exit
			if (!m_myexit)
			{
				if (tf2botutils::FindSpotToBuildTeleExit(me, &buildwpt, &buildarea, m_sentryWaypoint, &m_sentryBuildPos))
				{
					if (buildwpt)
					{
						return new CTF2BotEngineerBuildObjectTask(CTF2BotEngineerBuildObjectTask::OBJECT_TELEPORTER_EXIT, buildwpt);
					}
					else
					{
						return new CTF2BotEngineerBuildObjectTask(CTF2BotEngineerBuildObjectTask::OBJECT_TELEPORTER_EXIT, buildarea->GetRandomPoint());
					}
				}
				else // failed to find a spot to build a tele exit, just build the sentry
				{
					if (m_sentryWaypoint)
					{
						return new CTF2BotEngineerBuildObjectTask(CTF2BotEngineerBuildObjectTask::OBJECT_SENTRYGUN, m_sentryWaypoint);
					}
					else
					{
						return new CTF2BotEngineerBuildObjectTask(CTF2BotEngineerBuildObjectTask::OBJECT_SENTRYGUN, m_sentryBuildPos);
					}
				}
			}
			else
			{
				// Random chance to build the dispenser before the sentry gun
				if (!m_mydispenser && CBaseBot::s_botrng.GetRandomChance(35) && tf2botutils::FindSpotToBuildDispenser(me, &buildwpt, &buildarea, m_sentryWaypoint, &m_sentryBuildPos))
				{
					if (buildwpt)
					{
						return new CTF2BotEngineerBuildObjectTask(CTF2BotEngineerBuildObjectTask::OBJECT_DISPENSER, buildwpt);
					}
					else
					{
						return new CTF2BotEngineerBuildObjectTask(CTF2BotEngineerBuildObjectTask::OBJECT_DISPENSER, buildarea->GetRandomPoint());
					}
				}

				if (m_sentryWaypoint)
				{
					return new CTF2BotEngineerBuildObjectTask(CTF2BotEngineerBuildObjectTask::OBJECT_SENTRYGUN, m_sentryWaypoint);
				}
				else
				{
					return new CTF2BotEngineerBuildObjectTask(CTF2BotEngineerBuildObjectTask::OBJECT_SENTRYGUN, m_sentryBuildPos);
				}
			}
		}
		else
		{
			// sentry gun is built

			// try dispenser
			if (!m_mydispenser)
			{
				if (tf2botutils::FindSpotToBuildDispenser(me, &buildwpt, &buildarea, m_sentryWaypoint, &m_sentryBuildPos))
				{
					if (buildwpt)
					{
						return new CTF2BotEngineerBuildObjectTask(CTF2BotEngineerBuildObjectTask::OBJECT_DISPENSER, buildwpt);
					}
					else
					{
						return new CTF2BotEngineerBuildObjectTask(CTF2BotEngineerBuildObjectTask::OBJECT_DISPENSER, buildarea->GetRandomPoint());
					}
				}
				else if (m_mysentry)
				{
					Vector buildPos;
					if (tf2botutils::FindSpotToBuildDispenserBehindSentry(me, buildPos))
					{
						return new CTF2BotEngineerBuildObjectTask(CTF2BotEngineerBuildObjectTask::OBJECT_DISPENSER, buildPos);
					}
				}
			}
		}

		// build an exit if we don't have one already
		if (!m_myexit && tf2botutils::FindSpotToBuildTeleExit(me, &buildwpt, &buildarea, m_sentryWaypoint, &m_sentryBuildPos))
		{
			if (buildwpt)
			{
				return new CTF2BotEngineerBuildObjectTask(CTF2BotEngineerBuildObjectTask::OBJECT_TELEPORTER_EXIT, buildwpt);
			}
			else
			{
				return new CTF2BotEngineerBuildObjectTask(CTF2BotEngineerBuildObjectTask::OBJECT_TELEPORTER_EXIT, buildarea->GetRandomPoint());
			}
		}
	}

	// nothing to build
	// time to upgrade stuff
	// repair is handled in the main engineer behavior

	if (m_mysentry)
	{
		tfentities::HBaseObject object(m_mysentry);

		if (!object.IsAtMaxLevel())
		{
			return new CTF2BotEngineerUpgradeObjectTask(m_mysentry);
		}
	}

	if (m_mydispenser)
	{
		tfentities::HBaseObject object(m_mydispenser);

		if (!object.IsAtMaxLevel())
		{
			return new CTF2BotEngineerUpgradeObjectTask(m_mydispenser);
		}
	}

	if (m_myexit && m_myentrance)
	{
		tfentities::HBaseObject object(m_myexit);

		if (!object.IsAtMaxLevel())
		{
			if ((me->GetRangeTo(m_myentrance) + 600.0f) < me->GetRangeTo(m_myexit))
			{
				return new CTF2BotEngineerUpgradeObjectTask(m_myentrance);
			}
			else
			{
				return new CTF2BotEngineerUpgradeObjectTask(m_myexit);
			}
		}
	}

	return nullptr;
}

AITask<CTF2Bot>* CTF2BotEngineerNestTask::GetMoveBuildingsTask(CTF2Bot* me)
{
	const CTF2ModSettings* settings = CTeamFortress2Mod::GetTF2Mod()->GetTF2ModSettings();
	CTFWaypoint* buildwpt = nullptr;
	CTFNavArea* buildarea = nullptr;

	if (m_moveTimer.IsElapsed())
	{
		const CKnownEntity* threat = me->GetSensorInterface()->GetPrimaryKnownThreat(false);

		// Don't move buildings if I see or recently saw an enemy
		if (threat && (threat->IsVisibleNow() || threat->GetTimeSinceLastInfo() <= 10.0f))
		{
			m_moveTimer.Start(settings->GetEngineerMoveCheckInterval() * 0.25f);
			return nullptr;
		}

		// If waypoint trust settings is enabled, assume waypoints are always in a good spot and don't bother moving it unless the current waypoint is not avaiable anymore
		if (m_sentryWaypoint && settings->ShouldEngineersTrustWaypoints())
		{
			if (m_sentryWaypoint->IsEnabled() && m_sentryWaypoint->IsAvailableToTeam(static_cast<int>(me->GetMyTFTeam())))
			{
				m_moveTimer.Start(settings->GetEngineerMoveCheckInterval() * 0.25f);
				return nullptr;
			}
		}

		m_moveTimer.Start(settings->GetEngineerMoveCheckInterval()); 

		if (m_mysentry)
		{
			int currentKills = tf2lib::GetSentryKills(m_mysentry) + tf2lib::GetSentryAssists(m_mysentry);

			// todo settings
			if (currentKills - settings->GetEngineerSentryKillAssistsThreshold() <= m_sentryKills)
			{
				if (me->IsDebugging(BOTDEBUG_TASKS))
				{
					me->DebugPrintToConsole(0, 230, 200, "%s MOVING SENTRY! CURRENT KILLS %i STORED KILLS %i\n", me->GetDebugIdentifier(), currentKills, m_sentryKills);
				}

				if (tf2botutils::FindSpotToBuildSentryGun(me, &buildwpt, &buildarea))
				{
					// give some time to reach the new location
					m_moveTimer.Start(settings->GetEngineerMoveCheckInterval() + MOVE_BUILDING_TIMER_EXTRA_TIME);
					m_sentryKills = currentKills;
					m_checkDispenser = true;
					m_checkEntrance = true;
					m_checkExit = true;
					m_checkOthersTimer.Start(30.0f);

					if (buildwpt)
					{
						if (m_sentryWaypoint)
						{
							m_sentryWaypoint->StopUsing(me);
						}

						buildwpt->Use(me, 90.0f);
						m_sentryWaypoint = buildwpt;
						m_sentryBuildPos = buildwpt->GetOrigin();
						
						return new CTF2BotEngineerMoveObjectTask(m_mysentry, buildwpt, true);
					}
					else
					{
						if (m_sentryWaypoint)
						{
							m_sentryWaypoint->StopUsing(me);
							m_sentryWaypoint = nullptr;
						}

						m_sentryBuildPos = buildarea->GetRandomPoint();
						return new CTF2BotEngineerMoveObjectTask(m_mysentry, m_sentryBuildPos, true);
					}
				}
			}
			else
			{
				if (me->IsDebugging(BOTDEBUG_TASKS))
				{
					me->DebugPrintToConsole(255, 120, 0, "%s NOT MOVING SENTRY! CURRENT KILLS %i STORED KILLS %i\n", me->GetDebugIdentifier(), currentKills, m_sentryKills);
				}

				m_sentryKills = currentKills;
			}
		}
	}

	if (m_checkOthersTimer.HasStarted() && m_checkOthersTimer.IsElapsed())
	{
		m_checkOthersTimer.Invalidate();
		m_checkDispenser = true;
		m_checkEntrance = true;
		m_checkExit = true;
	}

	if (m_checkDispenser)
	{
		m_checkDispenser = false;

		if (m_mydispenser && m_mysentry)
		{
			const float range = (UtilHelpers::getEntityOrigin(m_mydispenser) - UtilHelpers::getEntityOrigin(m_mysentry)).Length();

			if (range >= settings->GetEngineerNestDispenserRange())
			{
				if (me->IsDebugging(BOTDEBUG_TASKS))
				{
					me->DebugPrintToConsole(0, 230, 200, "%s MOVING DISPENSER! RANGE TO SENTRY (%g) GREATER THAN LIMIT (%g)\n",
						me->GetDebugIdentifier(), range, settings->GetEngineerNestDispenserRange());
				}

				if (tf2botutils::FindSpotToBuildDispenser(me, &buildwpt, &buildarea, m_sentryWaypoint, &m_sentryBuildPos))
				{
					if (buildwpt)
					{
						return new CTF2BotEngineerMoveObjectTask(m_mydispenser, buildwpt, true);
					}
					else
					{
						return new CTF2BotEngineerMoveObjectTask(m_mydispenser, buildarea->GetRandomPoint(), true);
					}
				}
				else
				{
					Vector buildPos;
					if (tf2botutils::FindSpotToBuildDispenserBehindSentry(me, buildPos))
					{
						return new CTF2BotEngineerMoveObjectTask(m_mydispenser, buildPos, true);
					}
				}
			}
			else
			{
				if (me->IsDebugging(BOTDEBUG_TASKS))
				{
					me->DebugPrintToConsole(255, 120, 0, "%s NOT MOVING DISPENSER! RANGE TO SENTRY (%g) LESS THAN LIMIT (%g)\n",
						me->GetDebugIdentifier(), range, settings->GetEngineerNestDispenserRange());
				}
			}
		}
	}

	if (m_checkEntrance || m_checkExit)
	{
		// both exists
		if (m_myentrance && m_myexit)
		{
			int uses = tf2lib::GetTeleporterUses(m_myentrance);

			if (uses - settings->GetEngineerTeleporterUsesThreshold() > m_teleUses)
			{
				if (me->IsDebugging(BOTDEBUG_TASKS))
				{
					me->DebugPrintToConsole(255, 120, 0, "%s NOT MOVING TELEPORTERS! CURRENT USES %i STORED USES %i\n", me->GetDebugIdentifier(), uses, m_teleUses);
				}

				// teleporter still being used, skip
				m_checkEntrance = false;
				m_checkExit = false;
			}

			m_teleUses = uses;
		}
	}

	if (m_checkEntrance)
	{
		m_checkEntrance = false;

		if (m_myentrance)
		{
			CBaseEntity* spawn = tf2lib::GetNearestValidSpawnPointForTeam(me->GetMyTFTeam(), UtilHelpers::getEntityOrigin(m_myentrance));

			if (spawn)
			{
				const Vector& spawnPos = UtilHelpers::getEntityOrigin(spawn);
				const float range = (spawnPos - UtilHelpers::getEntityOrigin(m_myentrance)).Length();

				if (range >= settings->GetEntranceSpawnRange())
				{
					if (me->IsDebugging(BOTDEBUG_TASKS))
					{
						me->DebugPrintToConsole(0, 230, 200, "%s MOVING TELE ENTRANCE! DISTANCE TO SPAWN <%g %g %g> : %g\n",
							me->GetDebugIdentifier(), spawnPos.x, spawnPos.y, spawnPos.z, range);
					}

					if (tf2botutils::FindSpotToBuildTeleEntrance(me, &buildwpt, &buildarea))
					{
						if (buildwpt)
						{
							return new CTF2BotEngineerMoveObjectTask(m_myentrance, buildwpt, true);
						}
						else
						{
							return new CTF2BotEngineerMoveObjectTask(m_myentrance, buildarea->GetRandomPoint(), true);
						}
					}
				}
			}
		}
	}

	if (m_checkExit)
	{
		if (m_mysentry && m_myexit)
		{
			const float range = (UtilHelpers::getEntityOrigin(m_myexit) - UtilHelpers::getEntityOrigin(m_mysentry)).Length();

			if (range >= settings->GetEngineerNestExitRange())
			{
				if (me->IsDebugging(BOTDEBUG_TASKS))
				{
					me->DebugPrintToConsole(0, 230, 200, "%s MOVING TELE EXIT! RANGE TO SENTRY (%g) GREATER THAN LIMIT (%g)\n",
						me->GetDebugIdentifier(), range, settings->GetEngineerNestExitRange());
				}

				if (tf2botutils::FindSpotToBuildTeleExit(me, &buildwpt, &buildarea, m_sentryWaypoint, &m_sentryBuildPos))
				{
					if (buildwpt)
					{
						return new CTF2BotEngineerMoveObjectTask(m_myexit, buildwpt, true);
					}
					else
					{
						return new CTF2BotEngineerMoveObjectTask(m_myexit, buildarea->GetRandomPoint(), true);
					}
				}
			}
		}
	}

	return nullptr;
}

AITask<CTF2Bot>* CTF2BotEngineerNestTask::GetHelpFriendlyEngineerTask(CTF2Bot* me)
{
	using namespace std::literals::string_view_literals;

	constexpr std::array classnames = {
		"obj_sentrygun"sv,
		"obj_dispenser"sv,
		"obj_teleporter"sv,
	};

	m_allyObjects.clear();
	m_myOrigin = me->GetAbsOrigin();

	for (auto& clname : classnames)
	{
		UtilHelpers::ForEachEntityOfClassname(clname.data(), *this);
	}

	if (m_allyObjects.empty())
	{
		return nullptr;
	}

	for (auto& pair : m_allyObjects)
	{
		// Priorize sapped builings
		if (pair.first == ALLY_ACTION_SAPPED)
		{
			return new CTF2BotEngineerRepairObjectTask(pair.second);
		}
	}

	auto& pair = m_allyObjects[randomgen->GetRandomInt<std::size_t>(0U, m_allyObjects.size() - 1U)];

	if (pair.first == ALLY_ACTION_REPAIR)
	{
		return new CTF2BotEngineerRepairObjectTask(pair.second);
	}

	return new CTF2BotEngineerUpgradeObjectTask(pair.second);
}
