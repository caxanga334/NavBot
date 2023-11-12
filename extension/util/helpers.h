#ifndef UTIL_HELPERS_H_
#define UTIL_HELPERS_H_
#pragma once

struct edict_t;
class CBaseEntity;
class Vector;

namespace UtilHelpers
{
	edict_t* BaseEntityToEdict(CBaseEntity* pEntity);
	CBaseEntity* GetEntity(int entity);
	bool IndexToAThings(int num, CBaseEntity** pEntData, edict_t** pEdictData);
	Vector getOBBCenter(edict_t* pEntity);
	Vector collisionToWorldSpace(const Vector& in, edict_t* pEntity);
	Vector getWorldSpaceCenter(edict_t* pEntity);
	bool pointIsWithinTrigger(edict_t* pEntity, const Vector& vPoint);
	bool isBoundsDefinedInEntitySpace(edict_t* pEntity);
	int FindEntityByClassname(int start, const char* classname);
	int FindEntityInSphere(int start, Vector center, float radius);
	int FindEntityByNetClass(int start, const char* classname);
	bool PointWithinViewAngle(Vector const& vecSrcPosition, Vector const& vecTargetPosition, Vector const& vecLookDirection, float flCosHalfFOV);
	float GetForwardViewCone(float angle);
}

#endif // !UTIL_HELPERS_H_



