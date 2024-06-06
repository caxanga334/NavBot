/*
 * EntityUtils.h
 *
 *  Created on: Apr 15, 2017
 */

#ifndef UTILS_VALVE_NAVMESH_UTIL_ENTITYUTILS_H_
#define UTILS_VALVE_NAVMESH_UTIL_ENTITYUTILS_H_

#include <utllinkedlist.h>

class CBaseEntity;
class IHandleEntity;
class IPlayerInfo;
class Vector;
struct edict_t;

enum BrushSolidities_e {
	BRUSHSOLID_TOGGLE = 0,
	BRUSHSOLID_NEVER  = 1,
	BRUSHSOLID_ALWAYS = 2,
};

edict_t * findNearestEntity(const CUtlLinkedList<edict_t*>& entities, const Vector& pos,
		float maxRadius = 0.0f);

/**
 * Find all entities with the given class name.
 */
void findEntWithMatchingName(const char* name, CUtlLinkedList<edict_t*>& result);

/**
 * Find all entities with class name with glob pattern
 */
void findEntWithPatternInName(const char* name, CUtlLinkedList<edict_t*>& result);

/**
 * Return true if given entity can be ignored when moving
 */
bool IsEntityWalkable( edict_t *entity, unsigned int flags );

bool isBreakable(edict_t* target);

bool FClassnameIs(edict_t* pEntity, const char* szClassname);

bool FClassnameIs(CBaseEntity* pEntity, const char* szClassname);

IPlayerInfo *UTIL_GetListenServerHost( void );

edict_t* UTIL_GetListenServerEnt();

#endif /* UTILS_VALVE_NAVMESH_UTIL_ENTITYUTILS_H_ */
