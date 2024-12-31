#include <extension.h>
#include <util/entprops.h>
#include <util/helpers.h>
#include "nav.h"
#include "nav_mesh.h"
#include "nav_trace.h"

bool CTraceFilterWalkableEntities::ShouldHitEntity(int entity, CBaseEntity* pEntity, edict_t* pEdict, const int contentsMask)
{
	if (CTraceFilterNoNPCsOrPlayers::ShouldHitEntity(entity, pEntity, pEdict, contentsMask))
	{
		return !TheNavMesh->IsEntityWalkable(pEntity, m_flags);
	}

	return false;
}

bool CTraceFilterTransientAreas::ShouldHitEntity(int entity, CBaseEntity* pEntity, edict_t* pEdict, const int contentsMask)
{
	if (trace::CTraceFilterSimple::ShouldHitEntity(entity, pEntity, pEdict, contentsMask))
	{
		if (entity > 0 && entity <= gpGlobals->maxClients)
		{
			return false; // players doesn't block areas
		}

		if (entity == 0)
		{
			return true; // always hit worldspawn
		}

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
