/*
 * UtilTraceLine.cpp
 *
 *  Created on: Apr 15, 2017
 */

#include "UtilTrace.h"

#include "EntityUtils.h"
#include "BaseEntity.h"
#include <model_types.h>
#include <eiface.h>
#include <iplayerinfo.h>
#include <engine/ivdebugoverlay.h>
#include <engine/IEngineTrace.h>
#include <iserverunknown.h>
#include <mathlib/vector.h>
// #include <icvar.h>

extern IVDebugOverlay* debugoverlay;
extern IEngineTrace *enginetrace;
// extern ICvar* cVars;

edict_t *entityFromEntityHandle(const IHandleEntity *pHandleEntity) {
	return reinterpret_cast<IServerUnknown*>(const_cast<IHandleEntity*>(pHandleEntity))->GetNetworkable()->GetEdict();
}

CTraceFilterSimple::CTraceFilterSimple(const IHandleEntity *passedict,
		int collisionGroup, ShouldHitFunc_t pExtraShouldHitFunc) {
	m_pPassEnt = passedict;
	m_collisionGroup = collisionGroup;
	m_pExtraShouldHitCheckFunction = pExtraShouldHitFunc;
}

bool StandardFilterRules(IHandleEntity *pHandleEntity, int fContentsMask) {
	edict_t *pCollide = entityFromEntityHandle(pHandleEntity);
	// Static prop case...
	if (!pCollide)
		return true;
	SolidType_t solid = pCollide->GetCollideable()->GetSolid();
	extern IVModelInfo *modelinfo;
	BaseEntity baseEntity(pCollide);
	return modelinfo->GetModelType(modelinfo->GetModel(*baseEntity.getModelIndex())) == mod_brush
			&& (solid == SOLID_BSP || solid == SOLID_VPHYSICS)
			&& (fContentsMask & CONTENTS_MONSTER) == 0
	// This code is used to cull out tests against see-thru entities
			&& ((fContentsMask & CONTENTS_WINDOW) || *baseEntity.getRenderMode() != kRenderNormal)
			// FIXME: this is to skip BSP models that are entities that can be
			// potentially moved/deleted, similar to a monster but doors don't seem to
			// be flagged as monsters
			// FIXME: the FL_WORLDBRUSH looked promising, but it needs to be set on
			// everything that's actually a worldbrush and it currently isn't
			&& ((fContentsMask & CONTENTS_MOVEABLE) || (*baseEntity.getMoveType() & 15) != MOVETYPE_PUSH);// !(touch->flags & FL_WORLDBRUSH) )
}

bool CGameRulesShouldCollide(int collisionGroup0, int collisionGroup1) {
	if (collisionGroup0 > collisionGroup1) {
		// swap so that lowest is always first
		::V_swap(collisionGroup0, collisionGroup1);
	}
#ifndef HL2MP
	if ((collisionGroup0 == COLLISION_GROUP_PLAYER
			|| collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT)
			&& collisionGroup1 == COLLISION_GROUP_PUSHAWAY) {
		return false;
	}
#endif
	if (collisionGroup0 == COLLISION_GROUP_DEBRIS
			&& collisionGroup1 == COLLISION_GROUP_PUSHAWAY) {
		// let debris and multiplayer objects collide
		return true;
	}
	// --------------------------------------------------------------------------
	// NOTE: All of this code assumes the collision groups have been sorted!!!!
	// NOTE: Don't change their order without rewriting this code !!!
	// --------------------------------------------------------------------------

	// Don't bother if either is in a vehicle...
	if (collisionGroup0 == COLLISION_GROUP_IN_VEHICLE
			|| collisionGroup1 == COLLISION_GROUP_IN_VEHICLE
			|| (collisionGroup1 == COLLISION_GROUP_DOOR_BLOCKER
					&& collisionGroup0 != COLLISION_GROUP_NPC)
			|| (collisionGroup0 == COLLISION_GROUP_PLAYER
					&& collisionGroup1 == COLLISION_GROUP_PASSABLE_DOOR)
			|| collisionGroup0 == COLLISION_GROUP_DEBRIS
			|| collisionGroup0 == COLLISION_GROUP_DEBRIS_TRIGGER
			// put exceptions here, right now this will only collide with COLLISION_GROUP_NONE
			// Dissolving guys only collide with COLLISION_GROUP_NONE
			|| ((collisionGroup0 == COLLISION_GROUP_DISSOLVING
					|| collisionGroup1 == COLLISION_GROUP_DISSOLVING)
					&& collisionGroup0 != COLLISION_GROUP_NONE)
			// doesn't collide with other members of this group
			// or debris, but that's handled above
			|| (collisionGroup0 == COLLISION_GROUP_INTERACTIVE_DEBRIS && collisionGroup1 == COLLISION_GROUP_INTERACTIVE_DEBRIS)) {
		return false;
	}
#ifndef HL2MP
	// This change was breaking HL2DM
	// Adrian: TEST! Interactive Debris doesn't collide with the player.
	if (collisionGroup0 == COLLISION_GROUP_INTERACTIVE_DEBRIS
			&& (collisionGroup1 == COLLISION_GROUP_PLAYER_MOVEMENT
					|| collisionGroup1 == COLLISION_GROUP_PLAYER))
		return false;
#endif
	if ((((collisionGroup0 == COLLISION_GROUP_BREAKABLE_GLASS
			&& collisionGroup1 == COLLISION_GROUP_BREAKABLE_GLASS)
	// interactive objects collide with everything except debris & interactive debris
			|| (collisionGroup1 == COLLISION_GROUP_INTERACTIVE
					&& collisionGroup0 != COLLISION_GROUP_NONE)
			// Projectiles hit everything but debris, weapons, + other projectiles
			|| collisionGroup1 == COLLISION_GROUP_PROJECTILE)
			&& (collisionGroup0 == COLLISION_GROUP_DEBRIS
					|| collisionGroup0 == COLLISION_GROUP_WEAPON
					|| collisionGroup0 == COLLISION_GROUP_PROJECTILE))
	// Don't let vehicles collide with weapons
	// Don't let players collide with weapons...
	// Don't let NPCs collide with weapons
	// Weapons are triggers, too, so they should still touch because of that
			|| (collisionGroup1 == COLLISION_GROUP_WEAPON
					&& (collisionGroup0 == COLLISION_GROUP_VEHICLE
							|| collisionGroup0 == COLLISION_GROUP_PLAYER
							|| collisionGroup0 == COLLISION_GROUP_NPC))) {
		return false;
	}
	// collision with vehicle clip entity??
	return (collisionGroup0 != COLLISION_GROUP_VEHICLE_CLIP
			&& collisionGroup1 != COLLISION_GROUP_VEHICLE_CLIP)
		// yes then if it's a vehicle, collide, otherwise no collision
		// vehicle sorts lower than vehicle clip, so must be in 0
			|| collisionGroup0 == COLLISION_GROUP_VEHICLE;
		// vehicle clip against non-vehicle, no collision
}

bool PassServerEntityFilter( const IHandleEntity *pTouch, const IHandleEntity *pPass )
{
	if ( !pPass )
		return true;
	if ( pTouch == pPass )
		return false;
	edict_t *pEntTouch = entityFromEntityHandle( pTouch );
	edict_t *pEntPass = entityFromEntityHandle( pPass );
	return !pEntTouch || !pEntPass || pEntTouch != pEntPass;
}

bool CTraceFilterSimple::ShouldHitEntity(IHandleEntity *pHandleEntity,
		int contentsMask) {
	if ( !StandardFilterRules( pHandleEntity, contentsMask )
			|| (m_pPassEnt && !PassServerEntityFilter( pHandleEntity, m_pPassEnt ) ))
	{
		return false;
	}
	// Don't test if the game code tells us we should ignore this collision...
	edict_t *pEntity = entityFromEntityHandle( pHandleEntity );
	if (pEntity == nullptr) {
		return false;
	}
	int collisionGroup = pEntity->GetCollideable()->GetCollisionGroup();
	return (collisionGroup != COLLISION_GROUP_DEBRIS || (contentsMask & CONTENTS_DEBRIS))
			&& CGameRulesShouldCollide( m_collisionGroup, collisionGroup)
			&& (m_pExtraShouldHitCheckFunction == nullptr
					|| m_pExtraShouldHitCheckFunction( pHandleEntity, contentsMask )) ;
}

bool CTraceFilterWalkableEntities::ShouldHitEntity(IHandleEntity *pServerEntity,
		int contentsMask) {
	return CTraceFilterNoNPCsOrPlayer::ShouldHitEntity(pServerEntity, contentsMask)
		&& !IsEntityWalkable(entityFromEntityHandle( pServerEntity ), m_flags );
}

bool CTraceFilterNoNPCsOrPlayer::ShouldHitEntity(IHandleEntity *pHandleEntity,
		int contentsMask) {
	if (CTraceFilterSimple::ShouldHitEntity(pHandleEntity, contentsMask)) {
		edict_t *pEntity = entityFromEntityHandle(pHandleEntity);
		if (!pEntity)
			return false;
		/*
		 * TODO
		 #ifndef CLIENT_DLL
		 if ( pEntity->Classify() == CLASS_PLAYER_ALLY )
		 return false; // CS hostages are CLASS_PLAYER_ALLY but not IsNPC()
		 #endif
		 */
		extern CGlobalVars *gpGlobals;
		extern IVEngineServer* engine;

#if SOURCE_ENGINE == SE_SDK2013
		return pEntity->m_EdictIndex > 0
			&& engine->IndexOfEdict(pEntity) > gpGlobals->maxClients; //TODO && !player->IsNPC()
#elif SOURCE_ENGINE == SE_ORANGEBOX
		return engine->IndexOfEdict(pEntity) > gpGlobals->maxClients; // Orange box doesn't have CBaseEdict::m_EdictIndex
#else
		return pEntity->m_iIndex > 0
			&& engine->IndexOfEdict(pEntity) > gpGlobals->maxClients; //TODO && !player->IsNPC()
#endif

	}
	return false;
}

void UTIL_Trace(const Ray_t& ray, unsigned int mask, const ITraceFilter& filter,
		trace_t *ptr) {
	enginetrace->TraceRay(ray, mask, const_cast<ITraceFilter*>(&filter), ptr);
	/*if (cVars->FindVar("r_visualizetraces")->GetBool()) {
		debugoverlay->AddLineOverlay(ptr->startpos, ptr->endpos, 255, 0, 0,
				true, -1.0f);
	} */
}

void UTIL_TraceLine(const Vector& vecAbsStart, const Vector& vecAbsEnd,
		unsigned int mask, ITraceFilter *pFilter, trace_t *ptr) {
	Ray_t ray;
	ray.Init(vecAbsStart, vecAbsEnd);
	UTIL_Trace(ray, mask, *pFilter, ptr);
}

void UTIL_TraceLine(const Vector& vecAbsStart, const Vector& vecAbsEnd,
		unsigned int mask, const IHandleEntity *ignore, int collisionGroup,
		trace_t *ptr) {
	CTraceFilterSimple filter(ignore, collisionGroup);
	UTIL_TraceLine(vecAbsStart, vecAbsEnd, mask, &filter, ptr);
}

void UTIL_TraceHull(const Vector &vecAbsStart, const Vector &vecAbsEnd,
		const Vector &hullMin, const Vector &hullMax, unsigned int mask,
		const ITraceFilter &filter, trace_t *ptr, bool draw) {
	Ray_t ray;
	ray.Init(vecAbsStart, vecAbsEnd, hullMin, hullMax);
	UTIL_Trace(ray, mask, filter, ptr);
	if (!draw /* && !cVars->FindVar("r_visualizetraces")->GetBool() */) {
		return;
	}
	QAngle angle(0, 0, 0);
	int color = ptr->startsolid || ptr->allsolid ? 0 : 255;
	color = ptr->DidHit() ? 0 : 255;
	debugoverlay->AddLineOverlay(vecAbsStart, ptr->endpos, 255, color, color, true, -1.0f);
	debugoverlay->AddBoxOverlay(vecAbsStart, hullMin, hullMax, angle,
			255, color, color, true, -1.0f);
	if (!ptr->startsolid) {
		debugoverlay->AddBoxOverlay(ptr->endpos, hullMin, hullMax, angle,
				ptr->DidHit() ? 0 : 255, 255, 0, true, -1.0f);
	}
}

void UTIL_TraceHull(const Vector &vecAbsStart,
		const Vector &vecAbsEnd,
		const Vector &hullMin,
		const Vector &hullMax,
		unsigned int mask,
		const IHandleEntity *ignore,
		int collisionGroup,
		trace_t *ptr,
		bool draw) {
	UTIL_TraceHull(vecAbsStart, vecAbsEnd, hullMin, hullMax, mask,
			CTraceFilterSimple(ignore, collisionGroup), ptr, draw);
}

Vector UTIL_FindGround(const Vector& loc) {
	CTraceFilterWalkableEntities filter( NULL, COLLISION_GROUP_PLAYER_MOVEMENT,
	WALK_THRU_EVERYTHING);
	trace_t tr;
// half human height;
	Vector start(loc.x, loc.y, loc.z + 55 - 0.1f);
	Vector end = loc;
// death drop
	end.z -= 400.0f;
	UTIL_TraceHull(start, end, TRACE_MINS, TRACE_MAXS,
	MASK_NPCSOLID_BRUSHONLY, filter, &tr);
	return tr.endpos;
}

