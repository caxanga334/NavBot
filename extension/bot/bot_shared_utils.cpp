#include <extension.h>
#include <util/helpers.h>
#include "basebot.h"
#include "bot_shared_utils.h"

botsharedutils::AimSpotCollector::AimSpotCollector(CBaseBot* bot) :
	INavAreaCollector(bot->GetLastKnownNavArea(), bot->GetSensorInterface()->GetMaxVisionRange(), true, true, false), 
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

	area->ForEachConnectedArea([this, &visibleneighbors, &neighborsIDs](CNavArea* connectedArea) {
		neighborsIDs.push_back(connectedArea->GetID());

		bool nolos = (this->BlocksLOS(connectedArea->GetCenter() + m_vecOffset1) && this->BlocksLOS(connectedArea->GetCenter() + m_vecOffset2));

		// connected area can't be seen
		if (visibleneighbors && nolos)
		{
			visibleneighbors = false;
		}
	});

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

	CNavArea* area = TheNavMesh->GetNavArea(spot);
	SetStartArea(area);
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
	std::shared_ptr<CBotWeapon> weapon = bot->GetInventoryInterface()->GetActiveBotWeapon();

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
