#ifndef UTIL_HELPERS_H_
#define UTIL_HELPERS_H_
#pragma once

struct edict_t;
class CBaseEntity;
class Vector;
class CStudioHdr;
class SendTable;

namespace UtilHelpers
{
	edict_t* BaseEntityToEdict(CBaseEntity* pEntity);
	edict_t* GetEdict(int entity);
	CBaseEntity* GetEntity(int entity);
	bool IndexToAThings(int num, CBaseEntity** pEntData, edict_t** pEdictData);
	Vector getOBBCenter(edict_t* pEntity);
	Vector collisionToWorldSpace(const Vector& in, edict_t* pEntity);
	Vector getWorldSpaceCenter(edict_t* pEntity);
	bool pointIsWithinTrigger(edict_t* pEntity, const Vector& vPoint);
	bool isBoundsDefinedInEntitySpace(edict_t* pEntity);
	int FindEntityByClassname(int start, const char* searchname);
	int FindEntityInSphere(int start, Vector center, float radius);
	int FindEntityByNetClass(int start, const char* classname);
	bool PointWithinViewAngle(Vector const& vecSrcPosition, Vector const& vecTargetPosition, Vector const& vecLookDirection, float flCosHalfFOV);
	float GetForwardViewCone(float angle);
	CStudioHdr* GetEntityModelPtr(edict_t* pEntity);
	int LookupBone(CStudioHdr* hdr, const char* bonename);
	// TO-DO: Investigate but the bone cache might complicate things
	// bool GetBoneTransform(CStudioHdr* hdr, int iBone, matrix3x4_t& pBoneToWorld);
	// bool GetBonePosition(CStudioHdr* hdr, int iBone, Vector& origin, QAngle& angles);

	bool IsPlayerIndex(const int index);
	bool FindNestedDataTable(SendTable* pTable, const char* name);
}

#endif // !UTIL_HELPERS_H_



