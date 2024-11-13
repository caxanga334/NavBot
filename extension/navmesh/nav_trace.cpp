#include <extension.h>
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
