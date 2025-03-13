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

		// TO-DO: Allow derived nav mesh classes to customize this
		if (UtilHelpers::FClassnameIs(pEntity, "func_brush") ||
			UtilHelpers::FClassnameIs(pEntity, "func_door*") ||
			UtilHelpers::FClassnameIs(pEntity, "prop_dynamic*") ||
			UtilHelpers::FClassnameIs(pEntity, "prop_static") ||
			UtilHelpers::FClassnameIs(pEntity, "func_wall_toggle") ||
			UtilHelpers::FClassnameIs(pEntity, "func_tracktrain"))
		{
			return true;
		}
	}

	return false;
}
