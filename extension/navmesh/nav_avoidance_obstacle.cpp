#include NAVBOT_PCH_FILE
#include "nav_mesh.h"
#include "nav_avoidance_obstacle.h"

CNavEntityAvoidanceObstacle::CNavEntityAvoidanceObstacle(CBaseEntity* entity)
{
	m_key = reinterpret_cast<std::uintptr_t>(entity);
	m_entity.Set(entity);
	UpdateEntityData(entity);

	// ignore small props
	if (m_height <= navgenparams->step_height)
	{
		// invalidate the HANDLE, the nav mesh will delete this instance.
		m_entity.Term();
		return;
	}

	m_oldPos = UtilHelpers::getWorldSpaceCenter(entity);

	std::vector<CNavArea*> areas;
	TheNavMesh->CollectAreasOverlappingExtent(m_bounds, areas);

	for (CNavArea* area : areas)
	{
		TheNavMesh->OnAvoidanceObstacleEnteredArea(area);
	}
}

CNavEntityAvoidanceObstacle::~CNavEntityAvoidanceObstacle()
{
	TheNavMesh->UnregisterEntityAvoidanceObstacle(m_key);
}

bool CNavEntityAvoidanceObstacle::IsEnabled() const
{
	return true;
}

bool CNavEntityAvoidanceObstacle::IsValid() const
{
	return m_entity.Get() != nullptr;
}

void CNavEntityAvoidanceObstacle::Update()
{
	CBaseEntity* entity = m_entity.Get();
	UpdateEntityData(entity);
	Vector newPos = UtilHelpers::getWorldSpaceCenter(entity);

	if ((newPos - m_oldPos).IsLengthGreaterThan(navgenparams->generation_step_size))
	{
		m_oldPos = newPos;
		
		// entity moved, update nav areas
		std::vector<CNavArea*> areas;
		TheNavMesh->CollectAreasOverlappingExtent(m_bounds, areas);

		for (CNavArea* area : areas)
		{
			TheNavMesh->OnAvoidanceObstacleEnteredArea(area);
		}
	}
}

bool CNavEntityAvoidanceObstacle::IsObstructing(const CNavArea* area) const
{
	Extent areaExtent;
	area->GetExtent(&areaExtent);
	return m_bounds.IsOverlapping(areaExtent);
}

float CNavEntityAvoidanceObstacle::GetObstructionHeight() const
{
	return m_height;
}

const Extent& CNavEntityAvoidanceObstacle::GetObstructionExtent() const
{
	return m_bounds;
}

CBaseEntity* CNavEntityAvoidanceObstacle::GetLinkedEntity() const
{
	return m_entity.Get();
}

void CNavEntityAvoidanceObstacle::UpdateEntityData(CBaseEntity* entity)
{
	m_bounds.Init(entity);
	// m_bounds.AddXYOffset(navgenparams->half_human_width);
	m_bounds.AddZOffset(navgenparams->step_height);
	Vector mins, maxs;
	entprops->GetEntPropVector(entity, Prop_Data, "m_vecMins", mins);
	entprops->GetEntPropVector(entity, Prop_Data, "m_vecMaxs", maxs);
	m_height = std::abs(mins.z) + std::abs(maxs.z);
}
