#include NAVBOT_PCH_FILE
#include <extension.h>
#include <util/entprops.h>
#include <util/helpers.h>
#include "nav.h"
#include "nav_mesh.h"
#include "nav_trace.h"

bool CTraceFilterWalkableEntities::ShouldHitEntity(IHandleEntity* pHandleEntity, int contentsMask)
{
	if (trace::CTraceFilterNoNPCsOrPlayers::ShouldHitEntity(pHandleEntity, contentsMask))
	{
		CBaseEntity* pEntity = trace::EntityFromEntityHandle(pHandleEntity);

		return !(TheNavMesh->IsEntityWalkable(pEntity, m_flags));
	}

	return false;
}

bool CTraceFilterTransientAreas::ShouldHitEntity(IHandleEntity* pHandleEntity, int contentsMask)
{
	if (trace::CTraceFilterNoNPCsOrPlayers::ShouldHitEntity(pHandleEntity, contentsMask))
	{
		CBaseEntity* pEntity = trace::EntityFromEntityHandle(pHandleEntity);

		if (gamehelpers->EntityToBCompatRef(pEntity) == 0)
		{
			return true; // always hit worldspawn
		}

		if (TheNavMesh->IsEntitySolidForTransientAreas(pEntity))
		{
			return true;
		}
	}

	return false;
}
