#include NAVBOT_PCH_FILE
#include <extension.h>
#include <util/helpers.h>
#include <util/prediction.h>
#include <util/entprops.h>
#include <util/gamedata_const.h>
#include <navmesh/nav_mesh.h>
#include <navmesh/nav_area.h>
#include <navmesh/nav_waypoint.h>
#include "basebot.h"
#include "bot_shared_utils.h"

botsharedutils::AimSpotCollector::AimSpotCollector(CBaseBot* bot) :
	INavAreaCollector(bot->GetLastKnownNavArea(), bot->GetSensorInterface()->GetMaxVisionRange(), true, true, false, true), 
	m_vecOffset1(0.0f, 0.0f, 32.0f), m_vecOffset2(0.0f, 0.0f, 60.0f)
{
	m_aimOrigin = bot->GetEyeOrigin();
	m_filter.SetPassEntity(bot->GetEntity());
}

bool botsharedutils::AimSpotCollector::ShouldCollect(CNavArea* area)
{
	bool visibleneighbors = true;

	if (BlocksLOS(area->GetCenter() + m_vecOffset1) && BlocksLOS(area->GetCenter() + m_vecOffset2))
	{
		return false; // center is not visible
	}

	std::vector<unsigned int> neighborsIDs;
	auto functor = [this, &visibleneighbors, &neighborsIDs](CNavArea* connectedArea) {
		neighborsIDs.push_back(connectedArea->GetID());

		bool nolos = (this->BlocksLOS(connectedArea->GetCenter() + m_vecOffset1) && this->BlocksLOS(connectedArea->GetCenter() + m_vecOffset2));

		// connected area can't be seen
		if (visibleneighbors && nolos)
		{
			visibleneighbors = false;
		}
	};

	area->ForEachConnectedArea(functor);

	for (unsigned int& id : neighborsIDs)
	{
		if (m_checkedAreas.count(id) > 0)
		{
			return false; // already check, skip
		}
	}

	if (!visibleneighbors)
	{
		for (unsigned int& id : neighborsIDs)
		{
			m_checkedAreas.insert(id);
		}

		return true; // this area contains connected areas that aren't visible, an enemy may arrive from it
	}

	return false;
}

void botsharedutils::AimSpotCollector::OnDone()
{
	for (CNavArea* area : GetCollectedAreas())
	{
		m_aimSpots.push_back(area->GetCenter() + m_vecOffset1);
	}
}

bool botsharedutils::AimSpotCollector::BlocksLOS(Vector endpos)
{
	trace_t tr;
	trace::line(m_aimOrigin, endpos, MASK_VISIBLE, &m_filter, tr);
	return tr.fraction < 1.0f;
}

botsharedutils::TraceFilterVisibleSimple::TraceFilterVisibleSimple() :
	trace::CTraceFilterSimple(nullptr, COLLISION_GROUP_NONE)
{
}

bool botsharedutils::TraceFilterVisibleSimple::ShouldHitEntity(IHandleEntity* pHandleEntity, int contentsMask)
{
	if (trace::CTraceFilterSimple::ShouldHitEntity(pHandleEntity, contentsMask))
	{
		CBaseEntity* pEntity = trace::EntityFromEntityHandle(pHandleEntity);

		if (!pEntity)
		{
			return true; // static prop
		}

		return EntityBlocksLOS(pEntity);
	}

	return false;
}

bool botsharedutils::TraceFilterVisibleSimple::EntityBlocksLOS(CBaseEntity* entity)
{
	int entindex = UtilHelpers::IndexOfEntity(entity);

	if (entindex == 0) { return true; } // worldspawn, always blocks

	if (entindex >= 1 && entindex <= gpGlobals->maxClients)
	{
		return false; // player, don't block LOS
	}

	if (UtilHelpers::FClassnameIs(entity, "prop_phys*"))
	{
		return false; // assume physics props can be pushed and doesn't block LOS
	}

	if (UtilHelpers::FClassnameIs(entity, "obj_*"))
	{
		return false; // Buildings don't block LOS
	}

	return true;
}

botsharedutils::RandomDefendSpotCollector::RandomDefendSpotCollector(const Vector& spot, CBaseBot* bot) :
	m_vecOffset(0.0f, 0.0f, 36.0f)
{
	m_bot = bot;
	m_spot = spot;
	m_filter.SetPassEntity(bot->GetEntity());

	SetSearchElevators(false);
	SetTravelLimit(bot->GetSensorInterface()->GetMaxVisionRange());

	CNavArea* area = TheNavMesh->GetNearestNavArea(spot, 512.0f, true, true);
	SetStartArea(area);
}

bool botsharedutils::RandomDefendSpotCollector::ShouldSearch(CNavArea* area)
{
	// Don't search into blocked areas for my team
	return !area->IsBlocked(m_bot->GetCurrentTeamIndex());
}

bool botsharedutils::RandomDefendSpotCollector::ShouldCollect(CNavArea* area)
{
	if (area == GetStartArea())
	{
		return false;
	}

	if (!m_bot->GetMovementInterface()->IsAreaTraversable(area))
	{
		return false;
	}

	trace_t tr;
	trace::line(m_spot, area->GetCenter() + m_vecOffset, MASK_VISIBLE, &m_filter, tr);

	if (tr.fraction < 1.0f)
	{
		return false; // area can't be seen from the position we're going to defend
	}

	return true;
}

botsharedutils::RandomSnipingSpotCollector::RandomSnipingSpotCollector(const Vector& spot, CBaseBot* bot, const float maxRange) :
	m_vecOffset1(0.0f, 0.0f, 0.0f), m_vecOffset2(0.0f, 0.0f, 0.0f), m_spot(spot)
{
	m_bot = bot;
	m_snipeArea = nullptr;
	m_filter.SetPassEntity(bot->GetEntity());

	float halfvisrange = (bot->GetSensorInterface()->GetMaxVisionRange() * 0.5f);
	m_minRange = std::clamp(halfvisrange, 256.0f, 512.0f);

	SetSearchElevators(false);
	SetTravelLimit(std::clamp(bot->GetSensorInterface()->GetMaxVisionRange(), 0.0f, maxRange));

	CNavArea* area = TheNavMesh->GetNavArea(spot);
	SetStartArea(area);
}

bool botsharedutils::RandomSnipingSpotCollector::ShouldSearch(CNavArea* area)
{
	return !area->IsBlocked(m_bot->GetCurrentTeamIndex());
}

bool botsharedutils::RandomSnipingSpotCollector::ShouldCollect(CNavArea* area)
{
	if (m_bot->GetRangeTo(area->GetCenter()) < m_minRange)
	{
		return false; // too close
	}

	if (!m_bot->GetMovementInterface()->IsAreaTraversable(area))
	{
		return false;
	}

	bool nolos = (BlockedLOS(area->GetCenter() + m_vecOffset1) && BlockedLOS(area->GetCenter() + m_vecOffset2));

	if (nolos) { return false; }

	return true;
}

void botsharedutils::RandomSnipingSpotCollector::OnDone()
{
	// Further filter the collected areas.
	
	constexpr float HIGH_GROUND_LIMIT = 450.0f;
	float lastZ = -999999.0f;
	float lastRange = 999999.0f;
	CNavArea* candidate = nullptr;

	for (CNavArea* area : GetCollectedAreas())
	{
		float z = std::abs((area->GetCenter().z - m_spot.z));
		float range = (area->GetCenter() - m_spot).Length();

		if (range < lastRange && z < HIGH_GROUND_LIMIT && z > lastZ)
		{
			lastZ = z;
			lastRange = range;
			candidate = area;
		}
	}

	m_snipeArea = candidate;
}

bool botsharedutils::RandomSnipingSpotCollector::BlockedLOS(Vector endPos)
{
	trace_t tr;
	trace::line(m_spot, endPos, MASK_VISIBLE, &m_filter, tr);
	return tr.fraction < 1.0f;
}

botsharedutils::FindCoverCollector::FindCoverCollector(const Vector& fromSpot, const float radius, const bool checkLOS, const bool checkCorners, const float maxSearchRange, CBaseBot* bot) :
	m_vecOffset(0.0f, 0.0f, 32.0f)
{
	m_coverFrom = fromSpot;
	m_minDistance = radius;
	m_checkLOS = checkLOS;
	m_checkCorners = checkCorners;
	m_bot = bot;
	m_myPos = bot->GetAbsOrigin();
	m_dir = (m_myPos - m_coverFrom);
	m_dir.NormalizeInPlace();
	m_cos = UtilHelpers::GetForwardViewCone(120.0f);
	SetStartArea(bot->GetLastKnownNavArea());
	SetTravelLimit(maxSearchRange);
	SetSearchElevators(false); // too slow for cover
}

botsharedutils::FindCoverCollector::FindCoverCollector(const Vector& fromSpot, const float radius, const bool checkLOS, const bool checkCorners, const float maxSearchRange, const Vector& origin) :
	m_vecOffset(0.0f, 0.0f, 32.0f)
{
	m_coverFrom = fromSpot;
	m_minDistance = radius;
	m_checkLOS = checkLOS;
	m_checkCorners = checkCorners;
	m_bot = nullptr;
	m_myPos = origin;
	m_dir = (m_myPos - m_coverFrom);
	m_dir.NormalizeInPlace();
	m_cos = UtilHelpers::GetForwardViewCone(120.0f);
	SetStartArea(TheNavMesh->GetNavArea(origin));
	SetTravelLimit(maxSearchRange);
	SetSearchElevators(false); // too slow for cover
}

bool botsharedutils::FindCoverCollector::ShouldSearch(CNavArea* area)
{
	if (m_bot)
	{
		return !area->IsBlocked(m_bot->GetCurrentTeamIndex());
	}

	return true;
}

bool botsharedutils::FindCoverCollector::ShouldCollect(CNavArea* area)
{
	if (m_bot)
	{
		if (!m_bot->GetMovementInterface()->IsAreaTraversable(area))
		{
			return false;
		}
	}

	float range = (m_coverFrom - area->GetCenter()).Length();

	if (range <= m_minDistance)
	{
		return false;
	}

	float range2 = (m_myPos - area->GetCenter()).Length();

	// area is closer to the cover from spot
	if (range < range2)
	{
		return false;
	}

	// skip areas that would make us move towards the threat!
	if (UtilHelpers::PointWithinViewAngle(m_myPos, area->GetCenter(), m_dir, m_cos))
	{
		return false;
	}
	
	if (m_checkLOS)
	{
		trace_t tr;

		trace::line(m_coverFrom, area->GetCenter() + m_vecOffset, MASK_VISIBLE, tr);

		if (!tr.DidHit())
		{
			return false; // has LOS to center
		}

		if (m_checkCorners)
		{
			for (int corner = 0; corner < static_cast<int>(NavCornerType::NUM_CORNERS); corner++)
			{
				trace::line(m_coverFrom, area->GetCorner(static_cast<NavCornerType>(corner)) + m_vecOffset, MASK_VISIBLE, tr);

				if (!tr.DidHit())
				{
					return false; // has LOS to corner
				}
			}
		}
	}

	return true;
}

float botsharedutils::weapons::GetMaxAttackRangeForCurrentlyHeldWeapon(CBaseBot* bot)
{
	const CBotWeapon* weapon = bot->GetInventoryInterface()->GetActiveBotWeapon();

	if (!weapon)
	{
		return -999999.0f;
	}

	float range = 0.0f;

	if (bot->GetControlInterface()->GetLastUsedAttackType() == IPlayerController::AttackType::ATTACK_NONE ||
		bot->GetControlInterface()->GetLastUsedAttackType() == IPlayerController::AttackType::ATTACK_PRIMARY)
	{
		range = weapon->GetWeaponInfo()->GetAttackInfo(WeaponInfo::AttackFunctionType::PRIMARY_ATTACK).GetMaxRange();
	}
	else
	{
		range = weapon->GetWeaponInfo()->GetAttackInfo(WeaponInfo::AttackFunctionType::SECONDARY_ATTACK).GetMaxRange();
	}

	return range;
}

void botsharedutils::aiming::SelectDesiredAimSpotForTarget(CBaseBot* bot, CBaseEntity* target)
{
	const Vector& maxs = reinterpret_cast<IServerEntity*>(target)->GetCollideable()->OBBMaxs();
	Vector center = UtilHelpers::getWorldSpaceCenter(target);
	const Vector& origin = UtilHelpers::getEntityOrigin(target);
	Vector head = origin + maxs;
	head.z -= 2.0f;

	const bool headisclear = bot->IsLineOfFireClear(head);

	if (bot->GetDifficultyProfile()->IsAllowedToHeadshot() && headisclear)
	{
		bot->GetControlInterface()->SetDesiredAimSpot(IDecisionQuery::DesiredAimSpot::AIMSPOT_HEAD);
	}
	else if (bot->IsLineOfFireClear(center))
	{
		bot->GetControlInterface()->SetDesiredAimSpot(IDecisionQuery::DesiredAimSpot::AIMSPOT_CENTER);
	}
	else if (bot->IsLineOfFireClear(origin))
	{
		bot->GetControlInterface()->SetDesiredAimSpot(IDecisionQuery::DesiredAimSpot::AIMSPOT_ABSORIGIN);
	}
	else if (headisclear) // this is a fallback for when the bot skips the head due to not being allowed to headshot but LOF is blocked for center and origin.
	{
		bot->GetControlInterface()->SetDesiredAimSpot(IDecisionQuery::DesiredAimSpot::AIMSPOT_CENTER);
	}
}

botsharedutils::RandomDestinationCollector::RandomDestinationCollector(CBaseBot* bot, const float travellimit) :
	INavAreaCollector<CNavArea>(bot->GetLastKnownNavArea(), travellimit)
{
	m_bot = bot;
}

botsharedutils::RandomDestinationCollector::RandomDestinationCollector(CNavArea* start, const float travellimit) :
	INavAreaCollector<CNavArea>(start, travellimit)
{
	m_bot = nullptr;
}

bool botsharedutils::RandomDestinationCollector::ShouldSearch(CNavArea* area)
{
	if (m_bot)
	{
		return m_bot->GetMovementInterface()->IsAreaTraversable(area);
	}

	return !area->IsBlocked(NAV_TEAM_ANY);
}

botsharedutils::IsReachableAreas::IsReachableAreas(CBaseBot* bot, const float limit, const bool searchLadders, const bool searchLinks, const bool searchElevators) :
	INavAreaCollector<CNavArea>(bot->GetLastKnownNavArea(), limit, searchLadders, searchLinks, searchElevators, false)
{
	m_bot = bot;
}

bool botsharedutils::IsReachableAreas::ShouldSearch(CNavArea* area)
{
	return !area->IsBlocked(m_bot->GetCurrentTeamIndex());
}

const bool botsharedutils::IsReachableAreas::IsReachable(CNavArea* area, float* cost) const
{
	auto node = GetNodeForArea(area);

	if (node)
	{
		if (cost)
		{
			*cost = node->GetTravelCostFromStart();
		}

		return true;
	}

	return false;
}

botsharedutils::SpreadDangerToNearbyAreas::SpreadDangerToNearbyAreas(CNavArea* start, int teamid, const float travellimit, const float danger, const float maxdanger) :
	INavAreaCollector<CNavArea>(start, travellimit)
{
	m_danger = danger;
	m_limit = maxdanger;
	m_teamid = teamid;
}

bool botsharedutils::SpreadDangerToNearbyAreas::ShouldSearch(CNavArea* area)
{
	return !area->IsBlocked(m_teamid);
}

bool botsharedutils::SpreadDangerToNearbyAreas::ShouldCollect(CNavArea* area)
{
	return true;
}

void botsharedutils::SpreadDangerToNearbyAreas::OnDone()
{
	for (CNavArea* area : GetCollectedAreas())
	{
		float danger = area->GetDanger(m_teamid) + m_danger;

		if (danger > m_limit)
		{
			danger = m_limit - area->GetDanger(m_teamid);

			if (danger <= 0.0f)
			{
				continue;
			}

			area->IncreaseDanger(m_teamid, danger);
			continue;
		}

		area->IncreaseDanger(m_teamid, m_danger);
	}
}

botsharedutils::SelectRetreatArea::SelectRetreatArea(CBaseBot* bot, RetreatAreaPreference preference, const float minDistFromThreat, const float maxRetreatDist) :
	INavAreaCollector(bot->GetLastKnownNavArea(), maxRetreatDist, true, true, false, false)
{
	/* search ladders and off-mesh links but skip elevators (not ideal for retreating) and incoming connections (may not be reachable) */

	m_bot = bot;
	m_mindistance = minDistFromThreat;
	m_preference = preference;
	m_retreatArea = nullptr;
}

bool botsharedutils::SelectRetreatArea::ShouldSearch(CNavArea* area)
{
	return !area->IsBlocked(m_bot->GetCurrentTeamIndex());
}

bool botsharedutils::SelectRetreatArea::ShouldCollect(CNavArea* area)
{
	const float myDist = m_bot->GetRangeTo(area->GetCenter());
	bool collect = true;

	auto functor = [this, &area, myDist, &collect](const CKnownEntity* known) {

		if (!collect) { return; } // skip calculations if I'm not collecting it already

		const float dist = (area->GetCenter() - known->GetLastKnownPosition()).Length();

		// if the distance between a threat is smaller than the given limit OR it's closer to the enemy than to me, don't collect it
		if (dist <= m_mindistance || dist < myDist)
		{
			collect = false;
		}
	};

	m_bot->GetSensorInterface()->ForEveryKnownEnemy(functor);

	return collect;
}

void botsharedutils::SelectRetreatArea::OnDone()
{
	m_retreatArea = nullptr;

	if (IsCollectedAreasEmpty())
	{
		return; // no place to retreat
	}

	switch (m_preference)
	{
	case RetreatAreaPreference::NEAREST:
	{
		CNavArea* nearestArea = nullptr;
		float smallest_cost = std::numeric_limits<float>::max();

		for (auto area : GetCollectedAreas())
		{
			auto node = GetNodeForArea(area);
			// node should never be NULL for collected areas

			if (node->GetTravelCostFromStart() < smallest_cost)
			{
				nearestArea = area;
				smallest_cost = node->GetTravelCostFromStart();
			}
		}

		m_retreatArea = nearestArea;

		break;
	}
	case RetreatAreaPreference::FURTHEST:
	{
		CNavArea* nearestArea = nullptr;
		float highest_cost = -1.0f;

		for (auto area : GetCollectedAreas())
		{
			auto node = GetNodeForArea(area);
			// node should never be NULL for collected areas

			if (node->GetTravelCostFromStart() > highest_cost)
			{
				nearestArea = area;
				highest_cost = node->GetTravelCostFromStart();
			}
		}

		break;
	}
	default:
	{
		m_retreatArea = librandom::utils::GetRandomElementFromVector<CNavArea*>(GetCollectedAreas());
		return;
	}
	}
}

botsharedutils::CollectPatrolAreas::CollectPatrolAreas(CBaseBot* bot, const Vector& start, float minDistanceFromStart, const float maxSearchDistance) :
	INavAreaCollector<CNavArea>(nullptr, maxSearchDistance, true, true, false, false),
	m_bot(bot), m_vStart(start), m_vViewOffset(0.0f, 0.0f, 48.0f), m_minDistance(minDistanceFromStart)
{
	CNavArea* startArea = TheNavMesh->GetNearestNavArea(start, CPath::PATH_GOAL_MAX_DISTANCE_TO_AREA * 4.0f, true, true, bot->GetCurrentTeamIndex());
	SetStartArea(startArea);
}

bool botsharedutils::CollectPatrolAreas::ShouldSearch(CNavArea* area)
{
	return !area->IsBlocked(m_bot->GetCurrentTeamIndex());
}

bool botsharedutils::CollectPatrolAreas::ShouldCollect(CNavArea* area)
{
	const float range = (area->GetCenter() - m_vStart).Length();

	if (range < m_minDistance)
	{
		return false;
	}

	Vector start = m_vStart + m_vViewOffset;
	Vector end = area->GetCenter() + m_vViewOffset;

	trace_t tr;
	trace::line(start, end, MASK_BLOCKLOS, tr);

	if (tr.fraction >= 1.0f)
	{
		return false; // visible from start pos
	}



	for (const auto& vec : m_points)
	{
		const float r2 = (end - vec).Length();

		if (r2 > MAX_RANGE_FOR_VIS_CHECKS)
		{
			continue;
		}

		trace::line(vec, end, MASK_BLOCKLOS, tr);

		if (tr.fraction >= 1.0f)
		{
			return false; // current area is visible from a collected patrol spot
		}
	}

	m_points.push_back(std::move(end));
	return true;
}

CWaypoint* botsharedutils::waypoints::GetRandomRoamWaypoint(CBaseBot* bot, const float maxRange)
{
	std::vector<CWaypoint*> wpts;

	auto func = [&bot, &maxRange, &wpts](CWaypoint* waypoint) {
		if (waypoint->HasFlags(CWaypoint::BaseFlags::BASEFLAGS_ROAM) && waypoint->IsEnabled() && waypoint->IsAvailableToTeam(bot->GetCurrentTeamIndex()) && waypoint->CanBeUsedByBot(bot))
		{
			if (maxRange > 0.0f)
			{
				const float range = (bot->GetAbsOrigin() - waypoint->GetOrigin()).Length();

				if (range > maxRange)
				{
					return true;
				}
			}

			wpts.push_back(waypoint);
		}

		return true;
	};

	TheNavMesh->ForEveryWaypoint<CWaypoint, decltype(func)>(func);

	if (wpts.empty())
	{
		return nullptr;
	}

	if (wpts.size() == 1U)
	{
		return wpts[0];
	}

	return wpts[randomgen->GetRandomInt<std::size_t>(0U, wpts.size() - 1U)];
}

CWaypoint* botsharedutils::waypoints::GetRandomDefendWaypoint(CBaseBot* bot, const Vector* defendSpot, const float maxRange)
{
	std::vector<CWaypoint*> wpts;
	Vector origin = defendSpot != nullptr ? *defendSpot : bot->GetAbsOrigin();

	auto func = [&bot, &maxRange, &wpts, origin](CWaypoint* waypoint) {
		if (waypoint->HasFlags(CWaypoint::BaseFlags::BASEFLAGS_DEFEND) && waypoint->IsEnabled() && waypoint->IsAvailableToTeam(bot->GetCurrentTeamIndex()) && waypoint->CanBeUsedByBot(bot))
		{
			if (maxRange > 0.0f)
			{
				const float range = (origin - waypoint->GetOrigin()).Length();

				if (range > maxRange)
				{
					return true;
				}
			}

			wpts.push_back(waypoint);
		}

		return true;
		};

	TheNavMesh->ForEveryWaypoint<CWaypoint, decltype(func)>(func);

	if (wpts.empty())
	{
		return nullptr;
	}

	if (wpts.size() == 1U)
	{
		return wpts[0];
	}

	return wpts[randomgen->GetRandomInt<std::size_t>(0U, wpts.size() - 1U)];
}

CWaypoint* botsharedutils::waypoints::GetRandomSniperWaypoint(CBaseBot* bot, const Vector* defendSpot, const float maxRange)
{
	std::vector<CWaypoint*> wpts;
	Vector origin = defendSpot != nullptr ? *defendSpot : bot->GetAbsOrigin();

	auto func = [&bot, &maxRange, &wpts, origin](CWaypoint* waypoint) {
		if (waypoint->HasFlags(CWaypoint::BaseFlags::BASEFLAGS_SNIPER) && waypoint->IsEnabled() && waypoint->IsAvailableToTeam(bot->GetCurrentTeamIndex()) && waypoint->CanBeUsedByBot(bot))
		{
			if (maxRange > 0.0f)
			{
				const float range = (origin - waypoint->GetOrigin()).Length();

				if (range > maxRange)
				{
					return true;
				}
			}

			wpts.push_back(waypoint);
		}

		return true;
		};

	TheNavMesh->ForEveryWaypoint<CWaypoint, decltype(func)>(func);

	if (wpts.empty())
	{
		return nullptr;
	}

	if (wpts.size() == 1U)
	{
		return wpts[0];
	}

	return wpts[randomgen->GetRandomInt<std::size_t>(0U, wpts.size() - 1U)];
}

const CKnownEntity* botsharedutils::threat::DefaultThreatSelection(CBaseBot* bot, const CKnownEntity* first, const CKnownEntity* second)
{
	if (first != nullptr && second == nullptr)
	{
		return first;
	}

	if (first == nullptr && second != nullptr)
	{
		return second;
	}

	if (first == second)
	{
		return first;
	}

	const CBotWeapon* weapon = bot->GetInventoryInterface()->GetActiveBotWeapon();
	bool isMelee = (weapon != nullptr && weapon->GetWeaponInfo()->GetAttackInfo(WeaponInfo::AttackFunctionType::PRIMARY_ATTACK).IsMelee());

	// Melee always target the nearest threat
	if (isMelee)
	{
		return botsharedutils::threat::SelectNearestThreat(bot, first, second);
	}
	
	if (IsImmediateThreat(bot, first))
	{
		return first;
	}

	if (IsImmediateThreat(bot, second))
	{
		return second;
	}

	return botsharedutils::threat::SelectNearestThreat(bot, first, second);
}

const CKnownEntity* botsharedutils::threat::SelectNearestThreat(CBaseBot* bot, const CKnownEntity* first, const CKnownEntity* second)
{
	if (bot->GetRangeToSqr(first->GetLastKnownPosition()) > bot->GetRangeToSqr(second->GetLastKnownPosition()) && second->IsVisibleNow())
	{
		return second;
	}

	return first;
}

bool botsharedutils::threat::IsImmediateThreat(CBaseBot* bot, const CKnownEntity* threat)
{
	if (!threat->IsVisibleNow())
	{
		return false;
	}

	constexpr auto very_close_range = 512.0f;

	Vector to = (bot->GetAbsOrigin() - threat->GetLastKnownPosition());
	float range = to.NormalizeInPlace();

	if (range <= very_close_range)
	{
		return true;
	}

	return false;
}
