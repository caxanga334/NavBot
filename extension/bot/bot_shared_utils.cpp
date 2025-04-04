#include <extension.h>
#include <util/helpers.h>
#include "basebot.h"
#include "bot_shared_utils.h"

botsharedutils::AimSpotCollector::AimSpotCollector(CBaseBot* bot) :
	INavAreaCollector(bot->GetLastKnownNavArea(), bot->GetSensorInterface()->GetMaxVisionRange(), true, true, false), 
	m_vecOffset(0.0f, 0.0f, 32.0f)
{
	m_aimOrigin = bot->GetEyeOrigin();
	m_filter.SetPassEntity(bot->GetEntity());
}

bool botsharedutils::AimSpotCollector::ShouldCollect(CNavArea* area)
{
	bool visibleneighbors = true;

	trace_t tr;
	trace::line(m_aimOrigin, area->GetCenter() + m_vecOffset, MASK_VISIBLE, &m_filter, tr);

	if (tr.fraction < 1.0f)
	{
		return false; // center is not visible
	}

	std::vector<unsigned int> neighborsIDs;

	area->ForEachConnectedArea([&tr, this, &visibleneighbors, &neighborsIDs](CNavArea* connectedArea) {

		neighborsIDs.push_back(connectedArea->GetID());
		trace::line(m_aimOrigin, connectedArea->GetCenter() + m_vecOffset, MASK_VISIBLE, &m_filter, tr);

		// connected area can't be seen
		if (visibleneighbors && tr.fraction < 1.0f)
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
		m_aimSpots.push_back(area->GetCenter() + m_vecOffset);
	}
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
