#include NAVBOT_PCH_FILE
#include <extension.h>
#include <util/librandom.h>
#include <util/helpers.h>
#include <entities/tf2/tf_entities.h>
#include <mods/tf2/tf2lib.h>
#include <mods/tf2/nav/tfnav_waypoint.h>
#include <bot/tf2/tf2bot.h>
#include <bot/tf2/tasks/tf2bot_find_ammo_task.h>
#include "tf2bot_engineer_speedup_object.h"
#include "tf2bot_engineer_build_object.h"

CTF2BotEngineerBuildObjectTask::CTF2BotEngineerBuildObjectTask(eObjectType type, const CTFWaypoint* waypoint)
{
	m_type = type;
	m_waypoint = waypoint;
	m_goal = waypoint->GetRandomPoint();
	m_reachedGoal = false;
	m_trydir = randomgen->GetRandomInt<int>(0, 3); // randomize initial value
	m_hasangles = false;

	auto angles = waypoint->GetRandomAngle();

	if (angles.has_value())
	{
		m_lookangles = angles.value();
		m_hasangles = true;
	}
	else
	{
		m_lookangles = QAngle{ 0.0f, 0.0f, 0.0f };
	}

	switch (type)
	{
	case CTF2BotEngineerBuildObjectTask::OBJECT_SENTRYGUN:
		m_taskname.assign("BuildObject<SentryGun>");
		break;
	case CTF2BotEngineerBuildObjectTask::OBJECT_DISPENSER:
		m_taskname.assign("BuildObject<Dispenser>");
		break;
	case CTF2BotEngineerBuildObjectTask::OBJECT_TELEPORTER_ENTRANCE:
		m_taskname.assign("BuildObject<TeleEntrance>");
		break;
	case CTF2BotEngineerBuildObjectTask::OBJECT_TELEPORTER_EXIT:
		m_taskname.assign("BuildObject<TeleExit>");
		break;
	default:
		m_taskname.assign("BuildObject<unknown>");
		break;
	}
}

CTF2BotEngineerBuildObjectTask::CTF2BotEngineerBuildObjectTask(eObjectType type, const Vector& goal) :
	m_lookangles(0.0f, 0.0f, 0.0f)
{
	m_type = type;
	m_waypoint = nullptr;
	m_goal = goal;
	m_reachedGoal = false;
	m_trydir = randomgen->GetRandomInt<int>(0, 3); // randomize initial value
	m_hasangles = false;

	switch (type)
	{
	case CTF2BotEngineerBuildObjectTask::OBJECT_SENTRYGUN:
		m_taskname.assign("BuildObject<SentryGun>");
		break;
	case CTF2BotEngineerBuildObjectTask::OBJECT_DISPENSER:
		m_taskname.assign("BuildObject<Dispenser>");
		break;
	case CTF2BotEngineerBuildObjectTask::OBJECT_TELEPORTER_ENTRANCE:
		m_taskname.assign("BuildObject<TeleEntrance>");
		break;
	case CTF2BotEngineerBuildObjectTask::OBJECT_TELEPORTER_EXIT:
		m_taskname.assign("BuildObject<TeleExit>");
		break;
	default:
		m_taskname.assign("BuildObject<unknown>");
		break;
	}
}

TaskResult<CTF2Bot> CTF2BotEngineerBuildObjectTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	CTF2BotPathCost cost(bot, SAFEST_ROUTE);
	if (!m_nav.ComputePathToPosition(bot, m_goal, cost))
	{
		return Done("Failed to build path to build location.");
	}

	if (m_hasangles)
	{
		trace::CTraceFilterNoNPCsOrPlayers filter(bot->GetEntity(), COLLISION_GROUP_PLAYER_MOVEMENT);
		trace_t tr;
		Vector mins = bot->GetMins();
		Vector maxs = bot->GetMaxs();

		trace::hull(m_goal, m_goal, mins, maxs, MASK_PLAYERSOLID, &filter, tr);

		if (tr.fraction < 1.0f || tr.startsolid)
		{
			// disable high precision placement, bot will get stuck
			m_hasangles = false;
		}
	}

	m_nav.StartRepathTimer();

	// hack for mvm
	bot->SelectWeapon(bot->GetWeaponOfSlot(TeamFortress2::TFWeaponSlot::TFWeaponSlot_Primary));

	if (m_waypoint != nullptr)
	{
		m_waypoint->Use(bot, 30.0f);
	}

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotEngineerBuildObjectTask::OnTaskUpdate(CTF2Bot* bot)
{
	switch (m_type)
	{
	case CTF2BotEngineerBuildObjectTask::OBJECT_SENTRYGUN:
	{
		if (bot->GetMySentryGun() != nullptr)
		{
			return Done("Object built!");
		}

		break;
	}
	case CTF2BotEngineerBuildObjectTask::OBJECT_DISPENSER:
	{
		if (bot->GetMyDispenser() != nullptr)
		{
			return Done("Object built!");
		}

		break;
	}
	case CTF2BotEngineerBuildObjectTask::OBJECT_TELEPORTER_ENTRANCE:
	{
		if (bot->GetMyTeleporterEntrance() != nullptr)
		{
			return Done("Object built!");
		}

		break;
	}
	case CTF2BotEngineerBuildObjectTask::OBJECT_TELEPORTER_EXIT:
	{
		if (bot->GetMyTeleporterExit() != nullptr)
		{
			return Done("Object built!");
		}

		break;
	}
	default:
		return Done("Unknown object type!");
	}

	if (bot->GetAmmoOfIndex(TeamFortress2::TF_AMMO_METAL) < 150)
	{
		CBaseEntity* source = nullptr;

		if (CTF2BotFindAmmoTask::IsPossible(bot, &source))
		{
			return PauseFor(new CTF2BotFindAmmoTask(source, 150), "Need more metal to build!");
		}
	}

	if (m_reachedGoal)
	{
		if (m_giveupTimer.IsElapsed())
		{
			return Done("Giving up after many failed build attempts!");
		}

		// move randomly while trying to place
		if (m_strafeTimer.IsElapsed())
		{
			m_strafeTimer.Start(randomgen->GetRandomReal<float>(3.0f, 4.0f));

			if (randomgen->GetRandomInt<int>(0, 4) > 2)
			{
				bot->GetControlInterface()->PressMoveLeftButton(1.8f);
			}
			else
			{
				bot->GetControlInterface()->PressMoveRightButton(1.8f);
			}

			Vector forward, right;
			Vector origin = bot->GetEyeOrigin();
			bot->EyeVectors(&forward, &right, nullptr);
			forward.NormalizeInPlace();
			right.NormalizeInPlace();

			switch (m_trydir)
			{
			case 0:
				origin = origin + forward * 300.0f;
				m_trydir++;
				break;
			case 1:
				origin = origin + forward * -300.0f;
				m_trydir++;
				break;
			case 2:
				origin = origin + right * 300.0f;
				m_trydir++;
				break;
			default:
				origin = origin + right * -300.0f;
				m_trydir = 0; // reset
				break;
			}

			bot->GetControlInterface()->AimAt(origin, IPlayerController::LOOK_PRIORITY, 0.2f, "Looking at build place.");
		}

		// crouch while placing
		bot->GetControlInterface()->PressCrouchButton();

		CBaseEntity* weapon = bot->GetActiveWeapon();

		if (weapon != nullptr)
		{
			auto classname = gamehelpers->GetEntityClassname(weapon);

			if (classname != nullptr && classname[0] != '\0' && strcasecmp(classname, "tf_weapon_builder") == 0)
			{
				bot->GetControlInterface()->PressAttackButton();
			}
			else
			{
				switch (m_type)
				{
				case CTF2BotEngineerBuildObjectTask::OBJECT_SENTRYGUN:
					bot->BeginBuilding(TeamFortress2::TFObject_Sentry, TeamFortress2::TFObjectMode_None);
					break;
				case CTF2BotEngineerBuildObjectTask::OBJECT_DISPENSER:
					bot->BeginBuilding(TeamFortress2::TFObject_Dispenser, TeamFortress2::TFObjectMode_None);
					break;
				case CTF2BotEngineerBuildObjectTask::OBJECT_TELEPORTER_ENTRANCE:
					bot->BeginBuilding(TeamFortress2::TFObject_Teleporter, TeamFortress2::TFObjectMode_Entrance);
					break;
				case CTF2BotEngineerBuildObjectTask::OBJECT_TELEPORTER_EXIT:
					bot->BeginBuilding(TeamFortress2::TFObject_Teleporter, TeamFortress2::TFObjectMode_Exit);
					break;
				default:
					break;
				}
			}
		}
	}
	else
	{
		if (m_nav.NeedsRepath())
		{
			m_nav.StartRepathTimer();

			CTF2BotPathCost cost(bot, SAFEST_ROUTE);
			if (!m_nav.ComputePathToPosition(bot, m_goal, cost))
			{
				return Done("Failed to build path to build location.");
			}
		}

		m_nav.Update(bot);

		// RED bots on MvM can build without limits, this is a workround to make sure the bot has the correct weapon
		bot->SelectWeapon(bot->GetWeaponOfSlot(TeamFortress2::TFWeaponSlot_Primary));
	}


	return Continue();
}

void CTF2BotEngineerBuildObjectTask::OnTaskEnd(CTF2Bot* bot, AITask<CTF2Bot>* nextTask)
{
	bot->GetInventoryInterface()->SelectBestWeapon();
	bot->FindMyBuildings(); // update buildings
}

TaskEventResponseResult<CTF2Bot> CTF2BotEngineerBuildObjectTask::OnMoveToFailure(CTF2Bot* bot, CPath* path, IEventListener::MovementFailureType reason)
{
	CTF2BotPathCost cost(bot, SAFEST_ROUTE);
	if (!m_nav.ComputePathToPosition(bot, m_goal, cost))
	{
		return TryDone(PRIORITY_HIGH, "Failed to build path to build location.");
	}

	return TryContinue();
}

TaskEventResponseResult<CTF2Bot> CTF2BotEngineerBuildObjectTask::OnMoveToSuccess(CTF2Bot* bot, CPath* path)
{
	m_reachedGoal = true;
	m_giveupTimer.Start(15.0f);

	// high precision building placement
	if (m_hasangles)
	{
		Vector forward;
		AngleVectors(m_lookangles, &forward);
		forward.NormalizeInPlace();
		Vector lookpoint = m_goal + (forward * 256.0f);
		bot->GetControlInterface()->AimAt(lookpoint, IPlayerController::LOOK_CRITICAL, 2.0f, "Looking at high precision building placement location.");
		bot->Teleport(m_goal); // cheat
		bot->SnapEyeAngles(m_lookangles);
		m_strafeTimer.Start(4.0f);
		bot->GetMovementInterface()->DoCounterStrafe(0.2f);
	}
	else
	{
		m_strafeTimer.Start(2.0f);
		bot->GetMovementInterface()->DoCounterStrafe(0.2f);
	}

	return TryContinue();
}
