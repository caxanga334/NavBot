#include NAVBOT_PCH_FILE
#include <extension.h>
#include <util/librandom.h>
#include <util/entprops.h>
#include <mods/tf2/teamfortress2_shareddefs.h>
#include <mods/tf2/tf2lib.h>
#include <mods/tf2/teamfortress2mod.h>
#include <mods/tf2/nav/tfnavmesh.h>
#include <mods/tf2/nav/tfnavarea.h>
#include <mods/tf2/nav/tfnav_waypoint.h>
#include <entities/tf2/tf_entities.h>
#include <navmesh/nav_pathfind.h>
#include "tf2bot.h"
#include "tf2bot_utils.h"
#include <bot/bot_shared_utils.h>

#if defined(TF_DLL) && defined(EXT_DEBUG)
static ConVar cvar_debug_tf2botutils("sm_navbot_tf_debug_tf2bot_utils", "0", FCVAR_GAMEDLL | FCVAR_CHEAT, "Enable tf2botuilts debug messages.");
#endif // defined(TF_DLL) && defined(EXT_DEBUG)

class CTF2EngineerBuildableAreaCollector : public INavAreaCollector<CTFNavArea>
{
public:
	CTF2EngineerBuildableAreaCollector(CTF2Bot* engineer, CTFNavArea* startArea, const float maxRange = 4096.0f) :
		INavAreaCollector<CTFNavArea>(startArea, maxRange)
	{
		m_me = engineer;
		m_avoidSlopes = false;
		m_allowVisCheck = false;
		m_myteam = static_cast<int>(engineer->GetMyTFTeam());
	}

	bool ShouldSearch(CTFNavArea* area) override;
	bool ShouldCollect(CTFNavArea* area) override;
	void ShouldAvoidSlopes(bool avoid) { m_avoidSlopes = avoid; }
	void ShouldCheckVis(bool value) { m_allowVisCheck = value; }
	CTFNavArea* GetNearestAreaWithin(const float minDistFromStart);
private:
	CTF2Bot* m_me;
	bool m_avoidSlopes;
	bool m_allowVisCheck;
	int m_myteam;
};

bool CTF2EngineerBuildableAreaCollector::ShouldSearch(CTFNavArea* area)
{
	if (area->IsBlocked(m_myteam))
	{
		return false;
	}

	return true;
}

bool CTF2EngineerBuildableAreaCollector::ShouldCollect(CTFNavArea* area)
{
	if (!m_me->GetMovementInterface()->IsAreaTraversable(area))
	{
		return false;
	}

	if (!area->IsBuildable(m_me->GetMyTFTeam()))
	{
		return false;
	}

	if (m_avoidSlopes)
	{
		Vector point = area->GetCenter();
		point.z += 16.0f;
		Vector normal;

		if (trace::getgroundnormal(point, normal))
		{
			if (normal.z <= 0.98f)
			{
				return false;
			}
		}
	}

	if (m_allowVisCheck && !GetStartArea()->HasTFPathAttributes(CTFNavArea::TFNavPathAttributes::TFNAV_PATH_DYNAMIC_SPAWNROOM) && GetStartArea() != area)
	{
		Vector start = GetStartArea()->GetCenter();
		start.z += navgenparams->human_eye_height;
		Vector end = area->GetCenter();
		end.z += navgenparams->human_eye_height;

		CTraceFilterWorldAndPropsOnly filter;
		trace_t tr;
		trace::line(start, end, MASK_SHOT, &filter, tr);

		if (tr.DidHit())
		{
			return false;
		}
	}

	return true;
}

CTFNavArea* CTF2EngineerBuildableAreaCollector::GetNearestAreaWithin(const float minDistFromStart)
{
	auto& vec = GetCollectedAreas();

	if (vec.empty()) { return nullptr; }

	CTFNavArea* out = nullptr;
	float best = std::numeric_limits<float>::max();
	const Vector& start = GetStartArea()->GetCenter();

	for (CTFNavArea* area : vec)
	{
		if (area == GetStartArea()) { continue; }

		const float range = (area->GetCenter() - start).Length();

		if (minDistFromStart > 0.0f && range <= minDistFromStart) { continue; }

		auto node = GetNodeForArea(area);

		if (node->GetTravelCostFromStart() < best)
		{
			best = node->GetTravelCostFromStart();
			out = area;
		}
	}

	return out;
}

CTFWaypoint* tf2botutils::SelectWaypointForSentryGun(CTF2Bot* bot, const float maxRange, const Vector* searchCenter)
{
	std::vector<CTFWaypoint*> spots;
	auto& sentryWaypoints = CTeamFortress2Mod::GetTF2Mod()->GetAllSentryWaypoints();
	const Vector center = searchCenter ? *searchCenter : bot->GetAbsOrigin();

	for (auto waypoint : sentryWaypoints)
	{
		if (waypoint->IsEnabled() && waypoint->IsAvailableToTeam(static_cast<int>(bot->GetMyTFTeam())) && waypoint->CanBeUsedByBot(bot))
		{
			if (maxRange > 0.0f)
			{
				float range = (waypoint->GetOrigin() - center).Length();

				if (range > maxRange)
				{
					continue;
				}
			}

			spots.push_back(waypoint);
		}
	}

	if (spots.empty())
	{
		return nullptr;
	}

	return spots[CBaseBot::s_botrng.GetRandomInt<size_t>(0U, spots.size() - 1U)];
}

CTFNavArea* tf2botutils::FindRandomNavAreaToBuild(CTF2Bot* bot, const float maxRange, const Vector* searchCenter, const bool avoidSlopes, const bool checkVis)
{
	CTFNavArea* area = nullptr;

	if (searchCenter)
	{
		area = static_cast<CTFNavArea*>(TheNavMesh->GetNearestNavArea(*searchCenter, 1024.0f, false));
	}
	else
	{
		area = static_cast<CTFNavArea*>(bot->GetLastKnownNavArea());
	}

	if (!area)
	{
#if defined(TF_DLL) && defined(EXT_DEBUG)
		if (cvar_debug_tf2botutils.GetBool())
		{
			META_CONPRINTF("%s tf2botutils::FindRandomNavAreaToBuild NULL start area! maxRange: %g avoidSlopes %s \n", bot->GetDebugIdentifier(), maxRange, avoidSlopes ? "TRUE" : "FALSE");
		}
#endif // defined(TF_DLL) && defined(EXT_DEBUG)

		return nullptr;
	}

	CTF2EngineerBuildableAreaCollector collector(bot, area, maxRange);
	collector.ShouldAvoidSlopes(avoidSlopes);
	collector.ShouldCheckVis(checkVis);
	collector.Execute();

	if (collector.IsCollectedAreasEmpty())
	{
#if defined(TF_DLL) && defined(EXT_DEBUG)
		if (cvar_debug_tf2botutils.GetBool())
		{
			META_CONPRINTF("%s tf2botutils::FindRandomNavAreaToBuild collector is empty! maxRange: %g avoidSlopes %s \n", bot->GetDebugIdentifier(), maxRange, avoidSlopes ? "TRUE" : "FALSE");
		}
#endif // defined(TF_DLL) && defined(EXT_DEBUG)

		return nullptr;
	}

	CTFNavArea* result = collector.GetRandomCollectedArea();
	return result;
}

CTFWaypoint* tf2botutils::SelectWaypointForDispenser(CTF2Bot* bot, const float maxRange, const Vector* searchCenter, const CTFWaypoint* sentryWaypoint)
{
	std::vector<CTFWaypoint*> spots;
	const Vector center = searchCenter ? *searchCenter : bot->GetAbsOrigin();

	// if a sentry waypoint was given, search for any dispenser waypoints connected to it
	if (sentryWaypoint)
	{
		auto& connections = sentryWaypoint->GetConnections();

		for (auto& conn : connections)
		{
			CTFWaypoint* waypoint = static_cast<CTFWaypoint*>(conn.GetOther());

			if (waypoint->GetTFHint() == CTFWaypoint::TFHINT_DISPENSER && waypoint->CanBuildHere(bot))
			{
				if (maxRange > 0.0f)
				{
					float range = (waypoint->GetOrigin() - center).Length();

					if (range > maxRange)
					{
						continue;
					}
				}

				spots.push_back(waypoint);
			}
		}
	}

	if (spots.empty())
	{
		auto& dispenserWaypoints = CTeamFortress2Mod::GetTF2Mod()->GetAllDispenserWaypoints();

		for (auto waypoint : dispenserWaypoints)
		{
			if (waypoint->CanBuildHere(bot))
			{
				if (maxRange > 0.0f)
				{
					float range = (waypoint->GetOrigin() - center).Length();

					if (range > maxRange)
					{
						continue;
					}
				}

				spots.push_back(waypoint);
			}
		}

		if (spots.empty())
		{
			return nullptr;
		}
	}

	return spots[CBaseBot::s_botrng.GetRandomInt<size_t>(0U, spots.size() - 1U)];
}

CTFWaypoint* tf2botutils::SelectWaypointForTeleExit(CTF2Bot* bot, const float maxRange, const Vector* searchCenter, const CTFWaypoint* sentryWaypoint)
{
	std::vector<CTFWaypoint*> spots;
	const Vector center = searchCenter ? *searchCenter : bot->GetAbsOrigin();

	// if a sentry waypoint was given, search for any tele exit waypoints connected to it
	if (sentryWaypoint)
	{
		auto& connections = sentryWaypoint->GetConnections();

		for (auto& conn : connections)
		{
			CTFWaypoint* waypoint = static_cast<CTFWaypoint*>(conn.GetOther());

			if (waypoint->GetTFHint() == CTFWaypoint::TFHINT_TELE_EXIT && waypoint->CanBuildHere(bot))
			{
				if (maxRange > 0.0f)
				{
					float range = (waypoint->GetOrigin() - center).Length();

					if (range > maxRange)
					{
						continue;
					}
				}

				spots.push_back(waypoint);
			}
		}
	}

	if (spots.empty())
	{
		auto& teleExitWaypoints = CTeamFortress2Mod::GetTF2Mod()->GetAllTeleExitWaypoints();

		for (auto waypoint : teleExitWaypoints)
		{
			if (waypoint->CanBuildHere(bot))
			{
				if (maxRange > 0.0f)
				{
					float range = (waypoint->GetOrigin() - center).Length();

					if (range > maxRange)
					{
						continue;
					}
				}

				spots.push_back(waypoint);
			}
		}

		if (spots.empty())
		{
			return nullptr;
		}
	}

	return spots[CBaseBot::s_botrng.GetRandomInt<size_t>(0U, spots.size() - 1U)];
}

CTFWaypoint* tf2botutils::SelectWaypointForTeleEntrance(CTF2Bot* bot, const float maxRange, const Vector* searchCenter)
{
	std::vector<CTFWaypoint*> spots;
	auto& teleWaypoints = CTeamFortress2Mod::GetTF2Mod()->GetAllTeleEntranceWaypoints();
	const Vector center = searchCenter ? *searchCenter : bot->GetAbsOrigin();

	for (auto waypoint : teleWaypoints)
	{
		if (waypoint->IsEnabled() && waypoint->IsAvailableToTeam(static_cast<int>(bot->GetMyTFTeam())) && waypoint->CanBeUsedByBot(bot))
		{
			if (maxRange > 0.0f)
			{
				float range = (waypoint->GetOrigin() - center).Length();

				if (range > maxRange)
				{
					continue;
				}
			}

			spots.push_back(waypoint);
		}
	}

	if (spots.empty())
	{
		return nullptr;
	}

	return spots[CBaseBot::s_botrng.GetRandomInt<size_t>(0U, spots.size() - 1U)];
}

CTFNavArea* tf2botutils::FindTeleEntranceNavAreaFromSpawnRoom(CTF2Bot* bot, const Vector& spawnPointPos)
{
	botsharedutils::IsReachableAreas collector(bot, 5000.0f, false, true, false);
	CTFNavArea* start = static_cast<CTFNavArea*>(TheNavMesh->GetNearestNavArea(spawnPointPos, 600.0f, false, true, bot->GetCurrentTeamIndex()));

	if (start && start->HasTFPathAttributes(CTFNavArea::TFNavPathAttributes::TFNAV_PATH_DYNAMIC_SPAWNROOM))
	{
		collector.SetStartArea(start);
		collector.Execute();

		if (!collector.IsCollectedAreasEmpty())
		{
			CTFNavArea* best = nullptr;
			float smallest_cost = std::numeric_limits<float>::max();
			TeamFortress2::TFTeam myteam = bot->GetMyTFTeam();

			// Search the collected areas, select the nearest non spawn room area
			for (CNavArea* base : collector.GetCollectedAreas())
			{
				CTFNavArea* tfarea = static_cast<CTFNavArea*>(base);

				if (tfarea->IsBuildable(myteam)) // this already checks for the spawn room attribute
				{
					auto node = collector.GetNodeForArea(base);

					if (node->GetTravelCostFromStart() < smallest_cost)
					{
						smallest_cost = node->GetTravelCostFromStart();
						best = tfarea;
					}
				}
			}

			if (best)
			{
				return best;
			}
		}
	}

	return nullptr;
}

CTFNavArea* tf2botutils::FindAreaNearSentryGun(CTF2Bot* bot, CTFNavArea* sentryArea, const float maxRange, const float minRangeScale)
{
	CTF2EngineerBuildableAreaCollector collector(bot, sentryArea, maxRange);
	collector.Execute();

	if (collector.IsCollectedAreasEmpty()) { return nullptr; }

	CTFNavArea* area = collector.GetNearestAreaWithin(maxRange * minRangeScale);
	return area;
}

bool tf2botutils::GetSentrySearchStartPosition(CTF2Bot* bot, Vector& spot)
{
	TeamFortress2::GameModeType gm = CTeamFortress2Mod::GetTF2Mod()->GetCurrentGameMode();

	if (gm == TeamFortress2::GameModeType::GM_MVM)
	{
		const Vector& bombHatchPos = CTeamFortress2Mod::GetTF2Mod()->GetMvMBombHatchPosition();

		// between waves, setup near the frontline areas (bot spawn zones)

		if (entprops->GameRules_GetRoundState() == RoundState::RoundState_BetweenRounds)
		{
			CTFNavArea* frontlineArea = TheTFNavMesh()->GetRandomFrontLineArea();

			if (!frontlineArea)
			{
				return false; // the frontline attribute is required for MvM.
			}

			spot = frontlineArea->GetCenter();
			return true;
		}

		// wave is active, build near the objective (a bomb or a tank)

		CBaseEntity* tank = tf2lib::mvm::GetMostDangerousTank();
			
		// tank is active in the map
		if (tank)
		{
			CBaseEntity* flag = tf2lib::mvm::GetMostDangerousFlag(true);

			if (!flag)
			{
				spot = UtilHelpers::getEntityOrigin(tank);
				return true;
			}

			const Vector& tankPos = UtilHelpers::getEntityOrigin(tank);
			const Vector& flagPos = tf2lib::GetFlagPosition(flag);

			if ((tankPos - bombHatchPos).LengthSqr() < (flagPos - bombHatchPos).LengthSqr())
			{
				spot = tankPos;
			}
			else
			{
				spot = flagPos;
			}

			return true;
		}
		else
		{
			// no tank on the map
			CBaseEntity* flag = tf2lib::mvm::GetMostDangerousFlag(false);

			if (flag)
			{
				spot = UtilHelpers::getEntityOrigin(flag);
			}
			else
			{
				// no flag to defend, build near the frontlines
				CTFNavArea* frontlineArea = TheTFNavMesh()->GetRandomFrontLineArea();

				if (frontlineArea)
				{
					spot = frontlineArea->GetCenter();
				}
				else
				{
					spot = bot->GetAbsOrigin();
				}
			}

			return true;
		}
	}
	else if (gm == TeamFortress2::GameModeType::GM_CTF)
	{
		edict_t* flag = bot->GetFlagToDefend();
		spot = tf2lib::GetFlagPosition(flag->GetIServerEntity()->GetBaseEntity());
		return true;
	}
	else if (gm == TeamFortress2::GameModeType::GM_ADCP || gm == TeamFortress2::GameModeType::GM_CP || gm == TeamFortress2::GameModeType::GM_PL ||
		gm == TeamFortress2::GameModeType::GM_PL_RACE || gm == TeamFortress2::GameModeType::GM_KOTH)
	{
		std::vector<CBaseEntity*> points;
		points.reserve(MAX_CONTROL_POINTS);
		auto mod = CTeamFortress2Mod::GetTF2Mod();
		mod->CollectControlPointsToAttack(bot->GetMyTFTeam(), points);
		mod->CollectControlPointsToDefend(bot->GetMyTFTeam(), points);

		if (points.empty())
		{
			if (gm == TeamFortress2::GameModeType::GM_CP)
			{
				CBaseEntity* neutralPoint = mod->FindNeutralControlPoint();

				if (neutralPoint)
				{
					spot = UtilHelpers::getWorldSpaceCenter(neutralPoint);
					return true;
				}
			}

			spot = bot->GetAbsOrigin();
			return true;
		}

		CBaseEntity* pEntity = librandom::utils::GetRandomElementFromVector<CBaseEntity*>(points);

		spot = UtilHelpers::getWorldSpaceCenter(pEntity);

		return true;
	}
	else if (gm == TeamFortress2::GameModeType::GM_PD)
	{
		CBaseEntity* myleader = tf2lib::pd::GetTeamLeader(bot->GetMyTFTeam());

		if (myleader)
		{
			spot = UtilHelpers::getEntityOrigin(myleader);
			return true;
		}
	}

	spot = bot->GetAbsOrigin();
	return true;
}

float tf2botutils::GetSentrySearchMaxRange(bool isWaypointSearch)
{
	using namespace TeamFortress2;

	if (isWaypointSearch)
	{
		switch (CTeamFortress2Mod::GetTF2Mod()->GetCurrentGameMode())
		{
		case GameModeType::GM_MVM:
			return CTeamFortress2Mod::GetTF2Mod()->GetTF2ModSettings()->GetMvMSentryToBombRange();
		default:
			return -1.0f; // no limit
		}
	}
	
	switch (CTeamFortress2Mod::GetTF2Mod()->GetCurrentGameMode())
	{
	case GameModeType::GM_MVM:
		return CTeamFortress2Mod::GetTF2Mod()->GetTF2ModSettings()->GetMvMSentryToBombRange();
	default:
		return CTeamFortress2Mod::GetTF2Mod()->GetTF2ModSettings()->GetEngineerRandomNavAreaBuildRange();
	}
}

bool tf2botutils::FindSpotToBuildSentryGun(CTF2Bot* bot, CTFWaypoint** waypoint, CTFNavArea** area)
{
	Vector spot;

	if (tf2botutils::GetSentrySearchStartPosition(bot, spot))
	{
		*waypoint = tf2botutils::SelectWaypointForSentryGun(bot, tf2botutils::GetSentrySearchMaxRange(true), &spot);

		if (*waypoint != nullptr)
		{
			return true;
		}

		const bool checkVis = CTeamFortress2Mod::GetTF2Mod()->GetTF2ModSettings()->ShouldRandomNavAreaBuildCheckForVisiblity();
		*area = tf2botutils::FindRandomNavAreaToBuild(bot, tf2botutils::GetSentrySearchMaxRange(false), &spot, false, checkVis);

		if (*area != nullptr)
		{
			return true;
		}
	}

	return false;
}

bool tf2botutils::FindSpotToBuildDispenserBehindSentry(CTF2Bot* bot, Vector& spot, const float distance)
{
	CBaseEntity* sentryGun = bot->GetMySentryGun();

	if (!sentryGun)
	{
		return false;
	}

	const Vector& sentryPos = UtilHelpers::getEntityOrigin(sentryGun);
	const QAngle& sentryAngles = UtilHelpers::getEntityAngles(sentryGun);
	Vector forward;
	AngleVectors(sentryAngles, &forward);
	forward.NormalizeInPlace();
	spot = sentryPos + (forward * (distance * -1.0f));
	spot = trace::getground(spot);

	Vector mins(-16.0f, -16.0f, 0.0f);
	Vector maxs(16.0f, 16.0f, 72.0f);

	trace_t tr;
	trace::hull(spot, spot, mins, maxs, MASK_SOLID, bot->GetEntity(), COLLISION_GROUP_NONE, tr);

	if (!tr.startsolid && tr.fraction == 1.0f)
	{
		return true;
	}

	return false;
}

bool tf2botutils::FindSpotToBuildDispenser(CTF2Bot* bot, CTFWaypoint** waypoint, CTFNavArea** area, const CTFWaypoint* sentryGunWaypoint, const Vector* sentryGunPos)
{
	CBaseEntity* mysentry = bot->GetMySentryGun();

	Vector sentryPos;

	if (mysentry)
	{
		sentryPos = UtilHelpers::getEntityOrigin(mysentry);
	}
	else
	{
		if (!sentryGunPos)
		{
			return false; // no sentry and no position given
		}

		sentryPos = *sentryGunPos;
	}

	const float maxRange = CTeamFortress2Mod::GetTF2Mod()->GetTF2ModSettings()->GetEngineerNestDispenserRange();

	*waypoint = SelectWaypointForDispenser(bot, maxRange, &sentryPos, sentryGunWaypoint);

	if (*waypoint != nullptr)
	{
		return true;
	}

	CTFNavArea* sentryArea = static_cast<CTFNavArea*>(TheNavMesh->GetNearestNavArea(sentryPos, CPath::PATH_GOAL_MAX_DISTANCE_TO_AREA * 4.0f));

	if (sentryArea)
	{
		*area = FindAreaNearSentryGun(bot, sentryArea, maxRange);

		if (*area != nullptr)
		{
			return true;
		}
	}

	*area = FindRandomNavAreaToBuild(bot, maxRange, &sentryPos, false);

	if (*area != nullptr)
	{
		return true;
	}

	return false;
}

bool tf2botutils::FindSpotToBuildTeleEntrance(CTF2Bot* bot, CTFWaypoint** waypoint, CTFNavArea** area)
{
	*waypoint = tf2botutils::SelectWaypointForTeleEntrance(bot);

	if (*waypoint != nullptr)
	{
		return true;
	}

	const float maxRange = CTeamFortress2Mod::GetTF2Mod()->GetTF2ModSettings()->GetEntranceSpawnRange();

	CBaseEntity* spawnpoint = tf2lib::GetFirstValidSpawnPointForTeam(bot->GetMyTFTeam());

	if (!spawnpoint)
	{
		return false;
	}

	const Vector& searchStart = UtilHelpers::getEntityOrigin(spawnpoint);
	*area = tf2botutils::FindTeleEntranceNavAreaFromSpawnRoom(bot, searchStart);

	if (*area == nullptr)
	{
		*area = tf2botutils::FindRandomNavAreaToBuild(bot, maxRange, &searchStart, true);
	}

	if (*area != nullptr)
	{
		return true;
	}

	return false;
}

bool tf2botutils::FindSpotToBuildTeleExit(CTF2Bot* bot, CTFWaypoint** waypoint, CTFNavArea** area, const CTFWaypoint* sentryGunWaypoint, const Vector* sentryGunPos)
{
	CBaseEntity* mysentry = bot->GetMySentryGun();

	Vector sentryPos;

	if (mysentry)
	{
		sentryPos = UtilHelpers::getEntityOrigin(mysentry);
	}
	else
	{
		if (!sentryGunPos)
		{
			return false; // no sentry and no position given
		}

		sentryPos = *sentryGunPos;
	}

	const float maxRange = CTeamFortress2Mod::GetTF2Mod()->GetTF2ModSettings()->GetEngineerNestExitRange();

	*waypoint = tf2botutils::SelectWaypointForTeleExit(bot, maxRange, &sentryPos, sentryGunWaypoint);

	if (*waypoint != nullptr)
	{
		return true;
	}

	CTFNavArea* sentryArea = static_cast<CTFNavArea*>(TheNavMesh->GetNearestNavArea(sentryPos, CPath::PATH_GOAL_MAX_DISTANCE_TO_AREA * 4.0f));

	if (sentryArea)
	{
		*area = FindAreaNearSentryGun(bot, sentryArea, maxRange, 0.4f);

		if (*area != nullptr)
		{
			return true;
		}
	}

	*area = tf2botutils::FindRandomNavAreaToBuild(bot, maxRange, &sentryPos, true);

	if (*area != nullptr)
	{
		return true;
	}

	return false;
}

CBaseEntity* tf2botutils::MedicSelectBestPatientToHeal(CTF2Bot* bot, const float maxSearchRange)
{
	std::vector<std::pair<CBaseEntity*, int>> patients;
	patients.reserve(static_cast<size_t>(gpGlobals->maxClients));
	auto functor = [&bot, &maxSearchRange, &patients](int client, edict_t* entity, SourceMod::IGamePlayer* player) {
		// must be in-game, must not be the bot itself and must be alive
		if (player->IsInGame() && client != bot->GetIndex() && UtilHelpers::IsPlayerAlive(client))
		{
			if (bot->GetRangeTo(entity) > maxSearchRange)
			{
				return; // out of range
			}

			TeamFortress2::TFClassType theirclass = tf2lib::GetPlayerClassType(client);

			// handle enemy spies
			if (tf2lib::GetEntityTFTeam(client) != bot->GetMyTFTeam())
			{
				if (theirclass != TeamFortress2::TFClassType::TFClass_Spy)
				{
					return; // not a spy, don't heal
				}

				// player is from enemy team and is a spy.

				auto& spy = bot->GetSpyMonitorInterface()->GetKnownSpy(entity);

				if (spy.GetDetectionLevel() != CTF2BotSpyMonitor::SpyDetectionLevel::DETECTION_FOOLED)
				{
					return; // only heal spies that fooled me
				}
			}

			CBaseEntity* pEnt = entity->GetIServerEntity()->GetBaseEntity();

			// don't heal invisible players
			if (tf2lib::IsPlayerInvisible(pEnt))
			{
				return;
			}

			// all patients have a base score of 1000
			auto& pair = patients.emplace_back(pEnt, 1000);
			bool inCond = false; // in a condition that priorizes healing (fire/bleed)

			if (tf2lib::IsPlayerInCondition(client, TeamFortress2::TFCond::TFCond_Bleeding) ||
				tf2lib::IsPlayerInCondition(client, TeamFortress2::TFCond::TFCond_OnFire))
			{
				pair.second += 500; // priorize bleeding and burning patients
				inCond = true;
			}

			if (tf2lib::IsPlayerInCondition(client, TeamFortress2::TFCond::TFCond_Milked) ||
				tf2lib::IsPlayerInCondition(client, TeamFortress2::TFCond::TFCond_Jarated) ||
				tf2lib::IsPlayerInCondition(client, TeamFortress2::TFCond::TFCond_Gas))
			{
				pair.second += 100; // priorize removing these effects
			}

			float health = tf2lib::GetPlayerHealthPercentage(client);
			float score = RemapValClamped(health, 0.1f, 1.5f, 1300.0f, -800.0f);
			score = std::round(score);
			pair.second += static_cast<int>(score);

			if (theirclass == TeamFortress2::TFClass_Sniper && !inCond && health >= 0.99f)
			{
				pair.second = -1; // don't heal full health snipers unless they are bleeding or on fire
			}
			else if (theirclass == TeamFortress2::TFClass_Medic && health >= 0.5f)
			{
				pair.second -= 800; // prevents two medics getting stuck healing each other
			}
		}
	};

	// the functor is only called if player is not NULL.
	UtilHelpers::ForEachPlayer(functor);

	int bestscore = 0; // if a target ends up with a negative score, don't heal them
	CBaseEntity* ret = nullptr;

	for (auto& pair : patients)
	{
		if (pair.second > bestscore)
		{
			bestscore = pair.second;
			ret = pair.first;
		}
	}

	return ret;
}
