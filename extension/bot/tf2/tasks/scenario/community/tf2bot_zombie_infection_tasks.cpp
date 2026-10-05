#include NAVBOT_PCH_FILE
#include <numeric>
#include <mods/tf2/tf2lib.h>
#include <mods/tf2/teamfortress2mod.h>
#include <mods/tf2/nav/tfnavmesh.h>
#include <bot/tf2/tf2bot.h>
#include <bot/interfaces/path/chasenavigator.h>
#include <bot/bot_shared_utils.h>
#include <bot/tf2/tasks/tf2bot_scenario_task.h>
#include <bot/tasks_shared/bot_shared_default_combat_tasks.h>
#include <bot/tasks_shared/bot_shared_go_to_position.h>
#include <bot/tasks_shared/bot_shared_roam.h>
#include <bot/tasks_shared/bot_shared_defend_spot.h>
#include <bot/tasks_shared/bot_shared_retreat_from_threat.h>
#include <bot/tasks_shared/bot_shared_seek_and_destroy_entity.h>
#include <bot/tasks_shared/bot_shared_patrol_uncleared_areas.h>
#include <bot/tasks_shared/bot_shared_clear_reported_enemy.h>
#include "tf2bot_zombie_infection_tasks.h"

namespace zi
{
	static bool IsPlayerRevealed(CBaseEntity* player)
	{
		bool result = false;
		entprops->GetEntPropBool(player, Prop_Send, "m_bGlowEnabled", result);
		return result;
	}

	class CTF2ZIBeaconBuildSpotCollector : public botsharedutils::IsReachableAreas<CTFNavArea, CTF2Bot>
	{
	public:
		CTF2ZIBeaconBuildSpotCollector(CTF2Bot* me) :
			botsharedutils::IsReachableAreas<CTFNavArea, CTF2Bot>(me, 8192.0f)
		{
			CollectEnemyPositions(me);
			m_buildArea = nullptr;
		}

		bool ShouldSearch(CTFNavArea* area) override
		{
			if (botsharedutils::IsReachableAreas<CTFNavArea, CTF2Bot>::ShouldSearch(area))
			{
				// Every nav area that enters this function should have a valid node, if not then we bug
				auto node = GetNodeForArea(area);

				// Always search areas near the bot
				if (node->GetTravelCostFromStart() >= 1024.0f)
				{
					for (auto& eyePos : m_enemyPos)
					{
						// Do not place beacons
						if (area->IsCenterVisible(eyePos, navgenparams->human_eye_height, MASK_VISIBLE))
						{
							return false;
						}
					}
				}

				return true;
			}

			return false;
		}

		bool ShouldCollect(CTFNavArea* area) override
		{
			// Don't build in small areas.
			if (area->IsSmallerThan(40.0f))
			{
				return false;
			}

			// Don't build in the zombie's spawnroom
			if (area->GetSpawnRoomTeam() == TeamFortress2::TFTeam::TFTeam_Blue)
			{
				return false;
			}

			if (area->HasTFPathAttributes(CTFNavArea::TFNavPathAttributes::TFNAV_PATH_NO_BLU_TEAM))
			{
				return false;
			}

			return botsharedutils::IsReachableAreas<CTFNavArea, CTF2Bot>::ShouldCollect(area);
		}

		void OnDone() override
		{
			botsharedutils::IsReachableAreas<CTFNavArea, CTF2Bot>::OnDone();
			SelectBuildArea();
		}

		CTFNavArea* GetSelectedBuildArea() const { return m_buildArea; }

	private:
		std::vector<Vector> m_enemyPos;
		trace::CTraceFilterNoNPCsOrPlayers m_filter;
		CTFNavArea* m_buildArea;

		void CollectEnemyPositions(CTF2Bot* me)
		{
			auto func = [this](const CKnownEntity* known) {
				if (known->IsPlayer())
				{
					this->m_enemyPos.emplace_back(known->GetPlayerInstance()->GetEyeOrigin());
				}
			};

			me->GetSensorInterface()->ForEveryKnownEnemy(func);
		}


		void SelectBuildArea()
		{
			std::vector<std::pair<CTFNavArea*, float>> areas;
			Vector origin = GetBot()->GetAbsOrigin();

			auto func = [&areas, &origin, this](CTFNavArea* area) {

				constexpr float weightBotDistance = 1.0f;
				constexpr float weightEnemyDistance = 1.5f;

				const Vector& areaPos = area->GetCenter();
				float distanceToBot = areaPos.DistTo(origin);
				float minDistanceToEnemy = std::numeric_limits<float>::max();

				// there should always be at least one enemy
				for (const Vector& pos : m_enemyPos)
				{
					float dist = areaPos.DistTo(pos);

					if (dist < minDistanceToEnemy)
					{
						minDistanceToEnemy = dist;
					}
				}

				float score = -((weightBotDistance * distanceToBot) + (weightEnemyDistance * minDistanceToEnemy));
				areas.emplace_back(area, score);
				return true;
			};

			ForEachCollectedNavArea(func);

			float best_score = std::numeric_limits<float>::lowest();

			for (auto& [area, score] : areas)
			{
				if (score > best_score)
				{
					best_score = score;
					m_buildArea = area;
				}
			}

			if (GetBot()->IsDebugging(BOTDEBUG_TASKS) && m_buildArea != nullptr)
			{
				m_buildArea->DrawFilled(255, 255, 0, 127, 10.0f);
				GetBot()->DebugPrintToConsole(255, 255, 0, "%s BEACON BUILD AREA #%i SCORE %g OUT OF %zu\n", 
					GetBot()->GetDebugIdentifier(), m_buildArea->GetID(), best_score, areas.size());
				NDebugOverlay::Text(m_buildArea->GetCenter(), "BEACON BUILD AREA", false, 10.0f);
			}
		}
	};
}

TaskResult<CTF2Bot> CTF2BotZIZombieRespawningTask::OnTaskUpdate(CTF2Bot* bot)
{
	bot->GetMovementInterface()->ClearStuckStatus("ZI: Spawn Selection State!");

	// don't bother with spawn selections for now, just take what the game gives to us
	if (!m_didJump)
	{
		m_didJump = true;
		bot->GetControlInterface()->PressJumpButton(0.5f);
	}

	int flags = bot->GetFlags();

	if ((flags & FL_NOTARGET) == 0)
	{
		return Done("No longer in spawn state!");
	}

	return Continue();
}

CTF2BotZIMonitorTask::CTF2BotZIMonitorTask()
{
	m_isUsingClassBehavior = false;
	m_isZombie = false;
	m_placedBeacon = false;
}

AITask<CTF2Bot>* CTF2BotZIMonitorTask::InitialNextTask(CTF2Bot* bot)
{
	if (bot->GetMyTFTeam() == TeamFortress2::TFTeam::TFTeam_Red)
	{
		AITask<CTF2Bot>* classBehavior = CTF2BotScenarioTask::SelectClassTask(bot);

		if (classBehavior)
		{
			m_isUsingClassBehavior = true;
			return classBehavior;
		}

		return new CTF2BotZISurvivorBehaviorTask;
	}

	m_isZombie = true;
	return new CTF2BotZIZombieBehaviorTask;
}

TaskResult<CTF2Bot> CTF2BotZIMonitorTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	return Continue();
}

TaskResult<CTF2Bot> CTF2BotZIMonitorTask::OnTaskUpdate(CTF2Bot* bot)
{
	if (m_isZombie)
	{
		int flags = bot->GetFlags();

		// FL_NOTARGET is set when a zombie is in the spawn selection state
		if ((flags & FL_NOTARGET) != 0)
		{
			return PauseFor(new CTF2BotZIZombieRespawningTask, "In spawn selection state!");
		}

		if (m_zombieThinkTimer.IsElapsed())
		{
			m_zombieThinkTimer.Start(0.5f);

			DetectRevealedPlayers(bot);

			if (m_zombieAbilityCooldown.IsElapsed())
			{
				if (ShouldUseAbility(bot))
				{
					m_zombieAbilityCooldown.Start(12.0f);
					bot->GetControlInterface()->PressSecondaryAttackButton(0.5f);
				}
			}

			if (AllowedToBuildBeacon(bot))
			{
				Vector spot;
				if (FindBeaconBuildSpot(bot, spot))
				{
					m_placedBeacon = true;
					return PauseFor(new CTF2BotZIBuildBeaconTask(spot), "Building spawn beacon!");
				}
			}
		}
	}
	else
	{
		if (bot->GetMyTFTeam() == TeamFortress2::TFTeam::TFTeam_Blue)
		{
			return SwitchTo(new CTF2BotZIMonitorTask, "I have become a zombie, restarting ZI behavior!");
		}
	}

	return Continue();
}

void CTF2BotZIMonitorTask::DetectRevealedPlayers(CTF2Bot* me) const
{
	CTF2BotSensor* sensor = me->GetSensorInterface();

	auto func = [&sensor](int client, edict_t* entity, SourceMod::IGamePlayer* player) {
		CBaseEntity* pEntity = gamehelpers->ReferenceToEntity(client);

		if (player->IsInGame() && pEntity)
		{
			if (tf2lib::GetEntityTFTeam(client) == TeamFortress2::TFTeam::TFTeam_Red)
			{
				bool revealed = false;
				entprops->GetEntPropBool(client, Prop_Send, "m_bGlowEnabled", revealed);

				if (revealed)
				{
					CKnownEntity* known = sensor->AddKnownEntity(pEntity);
					known->UpdatePosition();
				}
			}
		}
	};

	UtilHelpers::ForEachPlayer(func);
}

bool CTF2BotZIMonitorTask::ShouldUseAbility(CTF2Bot* me) const
{
	TeamFortress2::TFClassType myclass = me->GetMyClassType();
	const CKnownEntity* threat = me->GetSensorInterface()->GetPrimaryKnownThreat();

	switch (myclass)
	{
	case TeamFortress2::TFClass_Soldier:
		[[fallthrough]];
	case TeamFortress2::TFClass_Sniper:
		[[fallthrough]];
	case TeamFortress2::TFClass_Pyro:
		return (threat != nullptr && threat->IsVisibleNow());
	case TeamFortress2::TFClass_DemoMan:
		return (threat != nullptr && threat->IsVisibleNow() && threat->IsPlayer() && me->GetRangeTo(threat->GetLastKnownPosition()) <= 600.0f);
	case TeamFortress2::TFClass_Engineer:
		return false;
	case TeamFortress2::TFClass_Medic:
		return me->GetHealthPercentage() <= me->GetDifficultyProfile()->GetHealthLowThreshold();
	case TeamFortress2::TFClass_Spy:
		return (threat != nullptr && threat->IsVisibleNow() && me->GetRangeTo(threat->GetLastKnownPosition()) <= 900.0f);
	case TeamFortress2::TFClass_Heavy:
		return (threat != nullptr && threat->IsVisibleNow() && me->GetRangeTo(threat->GetLastKnownPosition()) <= 900.0f);
	default:
		break;
	}

	return false;
}

bool CTF2BotZIMonitorTask::AllowedToBuildBeacon(CTF2Bot* me) const
{
	// Only place one beacon per life.
	// The beacon is a base_boss entity, the beacon owner is not easily accessible without access to vscript.
	if (m_placedBeacon)
	{
		return false;
	}

	// Must be an engineer
	if (me->GetMyClassType() != TeamFortress2::TFClassType::TFClass_Engineer)
	{
		return false;
	}

	const CKnownEntity* threat = me->GetSensorInterface()->GetPrimaryKnownThreat();

	// Wait until the bot is aware of at least one enemy
	if (threat == nullptr)
	{
		return false;
	}

	// Only build beacons against enemy survivors
	if (!threat->IsPlayer())
	{
		return false;
	}

	// Don't build, just attack
	if (threat->IsVisibleNow())
	{
		return false;
	}

	return true;
}

bool CTF2BotZIMonitorTask::FindBeaconBuildSpot(CTF2Bot* me, Vector& spot) const
{
	constexpr float Z_OFFSET = 40.0f;
	CTeamFortress2Mod* mod = CTeamFortress2Mod::GetTF2Mod();
	auto& waypoints = mod->GetAllTeleExitWaypoints();

	// Priorize designed placed spots
	if (!waypoints.empty())
	{
		std::vector<CTFWaypoint*> selected;

		for (auto waypoint : waypoints)
		{
			if (waypoint->IsAvailableTo(me))
			{
				selected.push_back(waypoint);
			}
		}

		if (!selected.empty())
		{
			CTFWaypoint* waypoint = librandom::utils::GetRandomElementFromVector(selected);
			spot = waypoint->GetRandomPoint();
			spot.z += Z_OFFSET;
			return true;
		}
	}

	zi::CTF2ZIBeaconBuildSpotCollector collector(me);
	collector.Execute();

	CTFNavArea* area = collector.GetSelectedBuildArea();

	if (area)
	{
		spot = area->GetCenter();
		spot.z += Z_OFFSET;
		return true;
	}

	return false;
}

TaskResult<CTF2Bot> CTF2BotZISurvivorBehaviorTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	return Continue();
}

TaskResult<CTF2Bot> CTF2BotZISurvivorBehaviorTask::OnTaskUpdate(CTF2Bot* bot)
{
	CTF2BotSensor* sensor = bot->GetSensorInterface();

	if (sensor->GetVisibleEnemiesCount() > 2 && sensor->GetVisibleEnemiesCount() > sensor->GetVisibleAlliesCount())
	{
		const CKnownEntity* threat = sensor->GetPrimaryKnownThreat();

		if ((bot->GetAbsOrigin() - threat->GetLastKnownPosition()).IsLengthLessThan(512.0f))
		{
			return PauseFor(new CBotSharedRetreatFromThreatTask<CTF2Bot, CTF2BotPathCost>(bot, false), "Retreating!");
		}
	}

	// increase the camp/defend chance on ZI for suvivors
	int defendChance = CTeamFortress2Mod::GetTF2Mod()->GetTF2ModSettings()->GetDefendRate() * 3;
	defendChance = std::clamp(defendChance, 0, 75);

	if (CBaseBot::s_botrng.GetRandomChance(defendChance))
	{
		// Survivor bots will use a random defend waypoint and stay there
		CWaypoint* waypoint = botsharedutils::waypoints::GetRandomDefendWaypoint(bot, nullptr, -1.0f);

		if (waypoint)
		{
			return PauseFor(new CBotSharedDefendSpotTask<CTF2Bot, CTF2BotPathCost>(bot, waypoint, -1.0f, true), "Camping");
		}
	}

	// Roam if no defend waypoint is available
	return PauseFor(new CBotSharedRoamTask<CTF2Bot, CTF2BotPathCost>(bot, 10000.0f, false, -1.0f, false), "Roaming!");
}

TaskResult<CTF2Bot> CTF2BotZIZombieBehaviorTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	return Continue();
}

TaskResult<CTF2Bot> CTF2BotZIZombieBehaviorTask::OnTaskUpdate(CTF2Bot* bot)
{
	CTF2BotSensor* sensor = bot->GetSensorInterface();

	const CKnownEntity* threat = sensor->GetPrimaryKnownThreat();

	if (threat)
	{
		if (threat->IsPlayer())
		{
			bool glowing = false;
			entprops->GetEntPropBool(threat->GetEntity(), Prop_Send, "m_bGlowEnabled", glowing);

			if (glowing)
			{
				auto task = new CBotSharedSeekAndDestroyEntityTask<CTF2Bot, CTF2BotPathCost>(bot, threat->GetEntity());
				auto func = std::bind(zi::IsPlayerRevealed, std::placeholders::_1);
				task->SetValidatorFunction(func);
				return PauseFor(task, "Chasing releaved survivor!");
			}
		}

		if (threat->IsVisibleNow())
		{
			return PauseFor(new CBotSharedDefaultCombatBehaviorTask<CTF2Bot, CTF2BotPathCost>, "Attacking visible survivor!");
		}

		// if this is false, then the bot knowns about this enemy from the shared memory interface
		if (!threat->WasLastKnownPositionSeen())
		{
			CBaseEntity* target;

			if (CBotSharedClearReportedEnemyTask<CTF2Bot, CTF2BotPathCost>::IsPossible(bot, &target))
			{
				return PauseFor(new CBotSharedClearReportedEnemyTask<CTF2Bot, CTF2BotPathCost>(target), "Investigating reported enemy position!");
			}
		}
	}

	if (CBaseBot::s_botrng.GetRandomChance(90))
	{
		CNavArea* area;

		if (CBotSharedPatrolUnclearedAreasTask<CTF2Bot, CTF2BotPathCost>::IsPossible(bot, &area))
		{
			return PauseFor(new CBotSharedPatrolUnclearedAreasTask<CTF2Bot, CTF2BotPathCost>(area), "Patrolling!");
		}
	}

	return PauseFor(new CBotSharedRoamTask<CTF2Bot, CTF2BotPathCost>(bot, 10000.0f, true, -1.0f, true), "Roaming!");
}

TaskResult<CTF2Bot> CTF2BotZIBuildBeaconTask::OnTaskUpdate(CTF2Bot* bot)
{
	if (bot->GetRangeTo(m_buildSpot) <= 600.0f)
	{
		if (IsLOFClear(bot))
		{
			auto input = bot->GetControlInterface();

			input->AimAt(m_buildSpot, IPlayerController::LOOK_CRITICAL, 1.0f, "Looking at beacon build spot!");

			if (input->IsAimOnTarget())
			{
				input->PressSecondaryAttackButton(0.5f);
				return Done("Beacon built!");
			}
		}
	}

	if (m_nav.NeedsRepath())
	{
		m_nav.StartRepathTimer();
		CTF2BotPathCost cost(bot, RouteType::SAFEST_ROUTE);

		if (!m_nav.ComputePathToPosition(bot, m_buildSpot, cost))
		{
			if (m_counter.Increase())
			{
				return Done("No path!");
			}
		}
	}

	m_nav.Update(bot);
	return Continue();
}

bool CTF2BotZIBuildBeaconTask::IsLOFClear(CTF2Bot* bot) const
{
	trace::CTraceFilterSimple filter(bot->GetEntity(), COLLISION_GROUP_NONE);
	trace_t tr;
	trace::line(bot->GetEyeOrigin(), m_buildSpot, MASK_SOLID, &filter, tr);

	if (tr.fraction == 1.0f)
	{
		return true;
	}

	if (bot->IsDebugging(BOTDEBUG_TASKS))
	{
		bot->DebugPrintToConsole(255, 0, 0, "%s ZI BUILD BEACON LOF OBSTRUCTED %g %s %s \n", 
			bot->GetDebugIdentifier(), tr.fraction, UtilHelpers::textformat::FormatVector(tr.endpos), UtilHelpers::textformat::FormatEntity(tr.m_pEnt));
	}

	if (tr.m_pEnt)
	{
		// hit a beacon
		if (UtilHelpers::FClassnameIs(tr.m_pEnt, "base_boss"))
		{
			return true;
		}
	}

	return false;
}
