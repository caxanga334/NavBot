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

class CTF2EngineerBuildableAreaCollector : public INavAreaCollector<CTFNavArea>
{
public:
	CTF2EngineerBuildableAreaCollector(CTF2Bot* engineer, CTFNavArea* startArea, const float maxRange = 4096.0f) :
		INavAreaCollector<CTFNavArea>(startArea, maxRange)
	{
		m_me = engineer;
		m_avoidSlopes = false;
	}

	bool ShouldCollect(CTFNavArea* area) override;
	void ShouldAvoidSlopes(bool avoid) { m_avoidSlopes = avoid; }
private:
	CTF2Bot* m_me;
	bool m_avoidSlopes;
};

bool CTF2EngineerBuildableAreaCollector::ShouldCollect(CTFNavArea* area)
{
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

	return true;
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

CTFNavArea* tf2botutils::FindRandomNavAreaToBuild(CTF2Bot* bot, const float maxRange, const Vector* searchCenter, const bool avoidSlopes)
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
		return nullptr;
	}

	CTF2EngineerBuildableAreaCollector collector(bot, area, maxRange);
	collector.ShouldAvoidSlopes(avoidSlopes);
	collector.Execute();

	if (collector.IsCollectedAreasEmpty())
	{
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
		else
		{
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
				else
				{
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
			spot = bot->GetAbsOrigin();
			return true;
		}

		CBaseEntity* pEntity = librandom::utils::GetRandomElementFromVector<CBaseEntity*>(points);

		spot = UtilHelpers::getWorldSpaceCenter(pEntity);

		return true;
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
			return 1500.0f;
		default:
			return -1.0f; // no limit
		}
	}
	
	switch (CTeamFortress2Mod::GetTF2Mod()->GetCurrentGameMode())
	{
	case GameModeType::GM_MVM:
		return 1500.0f;
	default:
		return 4096.0f; // don't search the entire map when searching nav areas
	}
}

