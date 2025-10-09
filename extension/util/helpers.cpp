#include NAVBOT_PCH_FILE
#include <stdexcept>
#include <cmath>

#include <extension.h>
#include <sdkports/sdk_traces.h>
#include <entities/baseentity.h>
#include <server_class.h>
#include <studio.h>
#include "sdkcalls.h"
#include "entprops.h"
#include "helpers.h"

#ifdef EXT_VPROF_ENABLED
#include <tier0/vprof.h>
#endif // EXT_VPROF_ENABLED

#undef max // undef mathlib defs to avoid comflicts with STD
#undef min
#undef clamp

#if SOURCE_ENGINE == SE_CSS || SOURCE_ENGINE == SE_DODS || SOURCE_ENGINE == SE_TF2 || SOURCE_ENGINE == SE_HL2DM || \
SOURCE_ENGINE == SE_BMS || SOURCE_ENGINE == SE_SDK2013

// Source SDKs which server tools V2 interface is available
#define SDKIFACE_SERVERTOOLSV2_AVAILABLE

#endif 

edict_t* UtilHelpers::GetEdict(int entity)
{

	edict_t* pEdict;
	if (!IndexToAThings(entity, nullptr, &pEdict))
	{
		return nullptr;
	}

	return pEdict;
}

CBaseEntity* UtilHelpers::GetEntity(int entity)
{
	CBaseEntity* pEntity;
	if (!IndexToAThings(entity, &pEntity, nullptr))
	{
		return nullptr;
	}

	return pEntity;
}

/* Given an entity reference or index, fill out a CBaseEntity and/or edict.
   If lookup is successful, returns true and writes back the two parameters.
   If lookup fails, returns false and doesn't touch the params.  */
bool UtilHelpers::IndexToAThings(int num, CBaseEntity** pEntData, edict_t** pEdictData)
{
	CBaseEntity* pEntity = gamehelpers->ReferenceToEntity(num);

	if (!pEntity)
	{
		return false;
	}

	int index = gamehelpers->ReferenceToIndex(num);
	if (index > 0 && index <= playerhelpers->GetMaxClients())
	{
		SourceMod::IGamePlayer* pPlayer = playerhelpers->GetGamePlayer(index);
		if (!pPlayer || !pPlayer->IsConnected())
		{
			return false;
		}
	}

	if (pEntData)
	{
		*pEntData = pEntity;
	}

	if (pEdictData)
	{
		edict_t* pEdict = BaseEntityToEdict(pEntity);
		if (!pEdict || pEdict->IsFree())
		{
			pEdict = nullptr;
		}

		*pEdictData = pEdict;
	}

	return true;
}

int UtilHelpers::IndexOfEntity(CBaseEntity* entity)
{
	return gamehelpers->EntityToBCompatRef(entity);
}

bool UtilHelpers::IsEntNetworkable(int index)
{
	CBaseEntity* entity = gamehelpers->ReferenceToEntity(index);

	if (!entity)
	{
		return false;
	}

	IServerNetworkable* pNet = reinterpret_cast<IServerUnknown*>(entity)->GetNetworkable();

	if (!pNet)
	{
		return false;
	}

	edict_t* edict = pNet->GetEdict();
	return (edict && !edict->IsFree()) ? true : false;
}

bool UtilHelpers::IsEntNetworkable(CBaseEntity* entity)
{
	IServerNetworkable* pNet = reinterpret_cast<IServerUnknown*>(entity)->GetNetworkable();

	if (!pNet)
	{
		return false;
	}

	edict_t* edict = pNet->GetEdict();
	return (edict && !edict->IsFree()) ? true : false;
}

bool UtilHelpers::HasDataTable(SendTable* root, const char* name)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("UtilHelpers::HasDataTable", "NavBot");
#endif // EXT_VPROF_ENABLED

	const char* pname = nullptr;
	int props = root->GetNumProps();
	SendProp* prop = nullptr;
	SendTable* table = nullptr;

	for (int i = 0; i < props; i++)
	{
		prop = root->GetProp(i);

		if ((table = prop->GetDataTable()) != nullptr)
		{
			pname = table->GetName();

			if (pname && strcmp(name, pname) == 0)
			{
				return true;
			}

			if (HasDataTable(table, name))
			{
				return true;
			}
		}
	}

	return false;
}

bool UtilHelpers::FindDataTable(SendTable* pTable, const char* name, sm_sendprop_info_t* info, unsigned int offset)
{
	const char* pname;
	int props = pTable->GetNumProps();
	SendProp* prop;
	SendTable* table;

	for (int i = 0; i < props; i++)
	{
		prop = pTable->GetProp(i);

		if ((table = prop->GetDataTable()) != NULL)
		{
			pname = table->GetName();
			if (pname && strcmp(name, pname) == 0)
			{
				info->prop = prop;
				info->actual_offset = offset + info->prop->GetOffset();
				return true;
			}

			if (FindDataTable(table, name, info, offset + prop->GetOffset()))
			{
				return true;
			}
		}
	}

	return false;
}

Vector UtilHelpers::getOBBCenter(edict_t* pEntity)
{
	Vector result = Vector(0, 0, 0);
	VectorLerp(pEntity->GetCollideable()->OBBMins(), pEntity->GetCollideable()->OBBMaxs(), 0.5f, result);
	return result;
}

Vector UtilHelpers::collisionToWorldSpace(const Vector& in, edict_t* pEntity)
{
	Vector result = Vector(0, 0, 0);

	if (!isBoundsDefinedInEntitySpace(pEntity) || pEntity->GetCollideable()->GetCollisionAngles() == vec3_angle)
	{
		VectorAdd(in, pEntity->GetCollideable()->GetCollisionOrigin(), result);
	}
	else
	{
		VectorTransform(in, pEntity->GetCollideable()->CollisionToWorldTransform(), result);
	}

	return result;
}

Vector UtilHelpers::collisionToWorldSpace(const Vector& in, ICollideable* collider)
{
	Vector result;

	if (!isBoundsDefinedInEntitySpace(collider) || collider->GetCollisionAngles() == vec3_angle)
	{
		VectorAdd(in, collider->GetCollisionOrigin(), result);
	}
	else
	{
		VectorTransform(in, collider->CollisionToWorldTransform(), result);
	}

	return result;
}

/**
 * Gets the entity world center. Clone of WorldSpaceCenter()
 * @param pEntity	The entity to get the center from
 * @return			Center vector
 **/
Vector UtilHelpers::getWorldSpaceCenter(edict_t* pEntity)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("UtilHelpers::getWorldSpaceCenter( edict )", "NavBot");
#endif // EXT_VPROF_ENABLED

	Vector result = getOBBCenter(pEntity);
	result = collisionToWorldSpace(result, pEntity);
	return result;
}

Vector UtilHelpers::getWorldSpaceCenter(CBaseEntity* pEntity)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("UtilHelpers::getWorldSpaceCenter( CBaseEntity )", "NavBot");
#endif // EXT_VPROF_ENABLED

	ICollideable* collider = reinterpret_cast<IServerEntity*>(pEntity)->GetCollideable();

	if (collider == nullptr)
	{
		return vec3_origin;
	}

	Vector result;
	VectorLerp(collider->OBBMins(), collider->OBBMaxs(), 0.5f, result);
	result = collisionToWorldSpace(result, collider);
	return result;
}

const Vector& UtilHelpers::getEntityOrigin(edict_t* entity)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("UtilHelpers::getEntityOrigin( edict )", "NavBot");
#endif // EXT_VPROF_ENABLED

	return entity->GetCollideable()->GetCollisionOrigin();
}

const Vector& UtilHelpers::getEntityOrigin(CBaseEntity* entity)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("UtilHelpers::getEntityOrigin( CBaseEntity )", "NavBot");
#endif // EXT_VPROF_ENABLED

	return reinterpret_cast<IServerEntity*>(entity)->GetCollideable()->GetCollisionOrigin();
}

const QAngle& UtilHelpers::getEntityAngles(edict_t* entity)
{
	return entity->GetCollideable()->GetCollisionAngles();
}

const QAngle& UtilHelpers::getEntityAngles(CBaseEntity* entity)
{
	return reinterpret_cast<IServerEntity*>(entity)->GetCollideable()->GetCollisionAngles();
}

void UtilHelpers::getEntityBounds(edict_t* entity, Vector& mins, Vector& maxs)
{
	mins = entity->GetCollideable()->OBBMins();
	maxs = entity->GetCollideable()->OBBMaxs();
}

void UtilHelpers::getEntityBounds(CBaseEntity* entity, Vector& mins, Vector& maxs)
{
	mins = reinterpret_cast<IServerEntity*>(entity)->GetCollideable()->OBBMins();
	maxs = reinterpret_cast<IServerEntity*>(entity)->GetCollideable()->OBBMaxs();
}

bool UtilHelpers::pointIsWithinTrigger(edict_t* pEntity, const Vector& vPoint)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("UtilHelpers::pointIsWithinTrigger (edict_t)", "NavBot");
#endif // EXT_VPROF_ENABLED

	Ray_t ray;
	trace_t tr;
	ICollideable* pCollide = pEntity->GetCollideable();
	ray.Init(vPoint, vPoint);
	enginetrace->ClipRayToCollideable(ray, MASK_ALL, pCollide, &tr);
	return (tr.startsolid);
}

bool UtilHelpers::pointIsWithinTrigger(CBaseEntity* pEntity, const Vector& vPoint)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("UtilHelpers::pointIsWithinTrigger (CBaseEntity)", "NavBot");
#endif // EXT_VPROF_ENABLED

	Ray_t ray;
	trace_t tr;
	ICollideable* pCollide = reinterpret_cast<IServerUnknown*>(pEntity)->GetCollideable();
	ray.Init(vPoint, vPoint);
	enginetrace->ClipRayToCollideable(ray, MASK_ALL, pCollide, &tr);
	return (tr.startsolid);
}

bool UtilHelpers::isBoundsDefinedInEntitySpace(edict_t* pEntity)
{
	return ((pEntity->GetCollideable()->GetSolidFlags() & FSOLID_FORCE_WORLD_ALIGNED) == 0 &&
		pEntity->GetCollideable()->GetSolid() != SOLID_BBOX && pEntity->GetCollideable()->GetSolid() != SOLID_NONE);
}

bool UtilHelpers::isBoundsDefinedInEntitySpace(ICollideable* collider)
{
	return ((collider->GetSolidFlags() & FSOLID_FORCE_WORLD_ALIGNED) == 0 && collider->GetSolid() != SOLID_BBOX && collider->GetSolid() != SOLID_NONE);
}

/// @brief Searches for entities by classname
/// @return Entity index/reference or INVALID_EHANDLE_INDEX if none is found
int UtilHelpers::FindEntityByClassname(int start, const char* searchname)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("UtilHelpers::FindEntityByClassname", "NavBot");
#endif // EXT_VPROF_ENABLED

#ifdef SDKIFACE_SERVERTOOLSV2_AVAILABLE
	CBaseEntity* pEntity = servertools->FindEntityByClassname(GetEntity(start), searchname);
	return gamehelpers->EntityToBCompatRef(pEntity);
#elif SOURCE_ENGINE == SE_EPISODEONE
	CBaseEntity* pEntity = nullptr;
	CBaseHandle hCurrent;

	if (start == -1)
	{
		hCurrent = g_EntList->FirstHandle();
		pEntity = UtilHelpers::GetBaseEntityFromCBaseHandle(hCurrent);
	}
	else
	{
		pEntity = gamehelpers->ReferenceToEntity(start);

		if (!pEntity)
		{
			return INVALID_EHANDLE_INDEX;
		}

		hCurrent = g_EntList->NextHandle(hCurrent);
		pEntity = UtilHelpers::GetBaseEntityFromCBaseHandle(hCurrent);
	}
	
	if (!pEntity)
	{
		return INVALID_EHANDLE_INDEX;
	}

	const char* classname = nullptr;
	int lastletterpos = 0;

	while (pEntity)
	{
		classname = entityprops::GetEntityClassname(pEntity);

		lastletterpos = strlen(searchname) - 1;
		if (searchname[lastletterpos] == '*')
		{
			if (strncasecmp(searchname, classname, lastletterpos) == 0)
			{
				return gamehelpers->EntityToBCompatRef(pEntity);
			}
		}
		else if (strcasecmp(searchname, classname) == 0)
		{
			return gamehelpers->EntityToBCompatRef(pEntity);
		}

		hCurrent = g_EntList->NextHandle(hCurrent);
		pEntity = UtilHelpers::GetBaseEntityFromCBaseHandle(hCurrent);
	}

	return INVALID_EHANDLE_INDEX;

#else

	// from: https://cs.alliedmods.net/sourcemod/source/extensions/sdktools/vnatives.cpp#873

	CBaseEntity* pEntity = nullptr;

	if (start == -1)
	{
		pEntity = (CBaseEntity*)servertools->FirstEntity();
	}
	else
	{
		pEntity = gamehelpers->ReferenceToEntity(start);
		if (!pEntity)
		{
			return INVALID_EHANDLE_INDEX;
		}
		pEntity = (CBaseEntity*)servertools->NextEntity(pEntity);
	}

	// it's tough to find a good ent these days
	if (!pEntity)
	{
		return INVALID_EHANDLE_INDEX;
	}

	const char* classname = nullptr;
	int lastletterpos;

	static int offset = -1;
	if (offset == -1)
	{
		SourceMod::sm_datatable_info_t info;
		if (!gamehelpers->FindDataMapInfo(gamehelpers->GetDataMap(pEntity), "m_iClassname", &info))
		{
			return INVALID_EHANDLE_INDEX;
		}

		offset = info.actual_offset;
	}

	string_t s;

	while (pEntity)
	{
		if ((s = *(string_t*)((uint8_t*)pEntity + offset)) == NULL_STRING)
		{
			pEntity = (CBaseEntity*)servertools->NextEntity(pEntity);
			continue;
		}

		classname = STRING(s);

		lastletterpos = strlen(searchname) - 1;
		if (searchname[lastletterpos] == '*')
		{
			if (strncasecmp(searchname, classname, lastletterpos) == 0)
			{
				return gamehelpers->EntityToBCompatRef(pEntity);
			}
		}
		else if (strcasecmp(searchname, classname) == 0)
		{
			return gamehelpers->EntityToBCompatRef(pEntity);
		}

		pEntity = (CBaseEntity*)servertools->NextEntity(pEntity);
	}

	return INVALID_EHANDLE_INDEX;

#endif // SOURCE_ENGINE > SE_ORANGEBOX
}

/// @brief Searches for entities in a sphere
/// @return Entity index/reference or INVALID_EHANDLE_INDEX if none is found
int UtilHelpers::FindEntityInSphere(int start, const Vector& center, float radius)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("UtilHelpers::FindEntityInSphere", "NavBot");
#endif // EXT_VPROF_ENABLED

#ifdef SDKIFACE_SERVERTOOLSV2_AVAILABLE
	CBaseEntity* pEntity = servertools->FindEntityInSphere(GetEntity(start), center, radius);
	return gamehelpers->EntityToBCompatRef(pEntity);
#elif SOURCE_ENGINE == SE_EPISODEONE
	CBaseEntity* pEntity = nullptr;
	CBaseHandle hCurrent;

	if (start == -1)
	{
		hCurrent = g_EntList->FirstHandle();
		pEntity = UtilHelpers::GetBaseEntityFromCBaseHandle(hCurrent);
	}
	else
	{
		pEntity = gamehelpers->ReferenceToEntity(start);

		if (!pEntity)
		{
			return INVALID_EHANDLE_INDEX;
		}

		hCurrent = g_EntList->NextHandle(hCurrent);
		pEntity = UtilHelpers::GetBaseEntityFromCBaseHandle(hCurrent);
	}

	if (!pEntity)
	{
		return INVALID_EHANDLE_INDEX;
	}

	Vector pos(0.0f, 0.0f, 0.0f);

	while (pEntity)
	{
		int index = gamehelpers->EntityToBCompatRef(pEntity);

		if (entprops->GetEntPropVector(index, Prop_Data, "m_vecOrigin", pos) == true)
		{
			float distancesqr = (center - pos).LengthSqr();

			if (distancesqr <= radius * radius)
			{
				return gamehelpers->EntityToBCompatRef(pEntity);
			}
		}

		hCurrent = g_EntList->NextHandle(hCurrent);
		pEntity = UtilHelpers::GetBaseEntityFromCBaseHandle(hCurrent);
	}

	return INVALID_EHANDLE_INDEX;
#else
	CBaseEntity* pEntity = nullptr;

	if (start == -1)
	{
		pEntity = static_cast<CBaseEntity*>(servertools->FirstEntity());
	}
	else
	{
		pEntity = gamehelpers->ReferenceToEntity(start);

		if (!pEntity)
		{
			return INVALID_EHANDLE_INDEX;
		}

		pEntity = static_cast<CBaseEntity*>(servertools->NextEntity(pEntity));
	}

	if (!pEntity)
	{
		return INVALID_EHANDLE_INDEX;
	}

	Vector pos(0.0f, 0.0f, 0.0f);

	while (pEntity)
	{
		int index = gamehelpers->EntityToBCompatRef(pEntity);

		if (entprops->GetEntPropVector(index, Prop_Data, "m_vecOrigin", pos) == true)
		{
			float distancesqr = (center - pos).LengthSqr();

			if (distancesqr <= radius * radius)
			{
				return gamehelpers->EntityToBCompatRef(pEntity);
			}
		}

		pEntity = static_cast<CBaseEntity*>(servertools->NextEntity(pEntity));
	}

	return INVALID_EHANDLE_INDEX;
#endif // SOURCE_ENGINE > SE_ORANGEBOX
}

/// @brief Searches for entities by their networkable class
/// @return Entity index or INVALID_EHANDLE_INDEX if none is found
int UtilHelpers::FindEntityByNetClass(int start, const char* classname)
{
	edict_t* current;

	for (int i = ((start != -1) ? start : 0); i < gpGlobals->maxEntities; i++)
	{
		current = gamehelpers->EdictOfIndex(i);
		if (!IsValidEdict(current))
		{
			continue;
		}

		IServerNetworkable* network = current->GetNetworkable();

		if (network == nullptr)
		{
			continue;
		}

		ServerClass* sClass = network->GetServerClass();
		const char* name = sClass->GetName();


		if (strcmp(name, classname) == 0)
		{
			return i;
		}
	}

	return INVALID_EHANDLE_INDEX;
}

/**
 * @brief Searches for an entity by their targetname (The entity name given by the level design for I/O purposes)
 * @param start Search starting entity or -1 for the first entity.
 * @param targetname Targetname to search for.
 * @return Entity index if found or INVALID_EHANDLE_INDEX if not found.
*/
int UtilHelpers::FindEntityByTargetname(int start, const char* targetname)
{
#ifdef SDKIFACE_SERVERTOOLSV2_AVAILABLE
	auto result = servertools->FindEntityByName(UtilHelpers::GetEntity(start), targetname);
	return gamehelpers->EntityToBCompatRef(result);
#elif SOURCE_ENGINE == SE_EPISODEONE

	if (!targetname || targetname[0] == 0)
	{
		return INVALID_EHANDLE_INDEX;
	}

	if (targetname[0] == '!')
	{
		// I don't see a need to implement this, throw an exception to alert the programmer
		throw std::runtime_error("Procedural entity search not supported!");
	}

	CBaseEntity* pEntity = nullptr;
	CBaseHandle hCurrent;

	if (start == -1)
	{
		hCurrent = g_EntList->FirstHandle();
		pEntity = UtilHelpers::GetBaseEntityFromCBaseHandle(hCurrent);
	}
	else
	{
		pEntity = gamehelpers->ReferenceToEntity(start);

		if (!pEntity)
		{
			return INVALID_EHANDLE_INDEX;
		}

		hCurrent = g_EntList->NextHandle(hCurrent);
		pEntity = UtilHelpers::GetBaseEntityFromCBaseHandle(hCurrent);
	}

	if (!pEntity)
	{
		return INVALID_EHANDLE_INDEX;
	}

	const char* name = nullptr;
	static int offset = -1;
	if (offset == -1)
	{
		SourceMod::sm_datatable_info_t info;
		if (!gamehelpers->FindDataMapInfo(gamehelpers->GetDataMap(pEntity), "m_iName", &info))
		{
			return INVALID_EHANDLE_INDEX;
		}

		offset = info.actual_offset;
	}

	string_t s;

	while (pEntity)
	{
		if ((s = *(string_t*)((uint8_t*)pEntity + offset)) == NULL_STRING)
		{
			hCurrent = g_EntList->NextHandle(hCurrent);
			pEntity = UtilHelpers::GetBaseEntityFromCBaseHandle(hCurrent);
			continue;
		}

		name = STRING(s);

		if (strcasecmp(name, targetname) == 0)
		{
			return gamehelpers->EntityToBCompatRef(pEntity);
		}

		hCurrent = g_EntList->NextHandle(hCurrent);
		pEntity = UtilHelpers::GetBaseEntityFromCBaseHandle(hCurrent);
	}

	return INVALID_EHANDLE_INDEX;
#else

	if (!targetname || targetname[0] == 0)
	{
		return INVALID_EHANDLE_INDEX;
	}

	if (targetname[0] == '!')
	{
		// I don't see a need to implement this, throw an exception to alert the programmer
		throw std::runtime_error("Procedural entity search not supported!");
	}

	CBaseEntity* pEntity = UtilHelpers::GetEntity(start);

	if (pEntity == nullptr)
	{
		pEntity = static_cast<CBaseEntity*>(servertools->FirstEntity());
	}

	const char* name = nullptr;
	static int offset = -1;
	if (offset == -1)
	{
		SourceMod::sm_datatable_info_t info;
		if (!gamehelpers->FindDataMapInfo(gamehelpers->GetDataMap(pEntity), "m_iName", &info))
		{
			return INVALID_EHANDLE_INDEX;
		}

		offset = info.actual_offset;
	}

	string_t s;

	while (pEntity)
	{
		if ((s = *(string_t*)((uint8_t*)pEntity + offset)) == NULL_STRING)
		{
			pEntity = (CBaseEntity*)servertools->NextEntity(pEntity);
			continue;
		}

		name = STRING(s);

		if (strcasecmp(name, targetname) == 0)
		{
			return gamehelpers->EntityToBCompatRef(pEntity);
		}

		pEntity = (CBaseEntity*)servertools->NextEntity(pEntity);
	}

	return INVALID_EHANDLE_INDEX;

#endif // SOURCE_ENGINE > SE_ORANGEBOX
}

/**
 * @brief Searches for an entity by classname and targetname.
 * @param start Search start entity.
 * @param targetname Targetname to search for.
 * @param classname Limit search to this classname.
 * @return Entity index if found or INVALID_EHANDLE_INDEX if not found.
*/
int UtilHelpers::FindNamedEntityByClassname(int start, const char* targetname, const char* classname)
{
	int i = start;

	while ((i = FindEntityByClassname(i, classname)) != INVALID_EHANDLE_INDEX)
	{
		char name[64];
		size_t length;
		
		if (entprops->GetEntPropString(i, Prop_Data, "m_iName", name, 64, length))
		{
			if (strcasecmp(name, targetname) == 0)
			{
				return i;
			}
		}
	}

	return INVALID_EHANDLE_INDEX;
}

/// @brief check if a point is in the field of a view of an object. supports up to 180 degree fov.
/// @param vecSrcPosition Source position of the view.
/// @param vecTargetPosition Point to check if within view angle.
/// @param vecLookDirection The direction to look towards.  Note that this must be a forward angle vector.
/// @param flCosHalfFOV The width of the forward view cone as a dot product result.
/// @return True if the point is within view from the source position at the specified FOV.
/// @note https://github.com/ValveSoftware/source-sdk-2013/blob/beaae8ac45a2f322a792404092d4482065bef7ef/sp/src/public/mathlib/vector.h#L462-L477
bool UtilHelpers::PointWithinViewAngle(const Vector& vecSrcPosition, const Vector& vecTargetPosition, const Vector& vecLookDirection, float flCosHalfFOV)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("UtilHelpers::PointWithinViewAngle", "NavBot");
#endif // EXT_VPROF_ENABLED

	Vector vecDelta = vecTargetPosition - vecSrcPosition;
	float cosDiff = DotProduct(vecLookDirection, vecDelta);

	if (cosDiff < 0)
		return false;

	float flLen2 = vecDelta.LengthSqr();

	// a/sqrt(b) > c  == a^2 > b * c ^2
	return (cosDiff * cosDiff > flLen2 * flCosHalfFOV * flCosHalfFOV);
}

/// @brief Calculates the width of the forward view cone as a dot product result from the given angle.
/// This manually calculates the value of CBaseCombatCharacter's `m_flFieldOfView` data property.
/// @param angle The FOV value in degree
/// @return Width of the forward view cone as a dot product result
float UtilHelpers::GetForwardViewCone(float angle)
{
	return cosf(DEG2RAD(angle) / 2.0f);
}

/**
 * @brief Allocates a new CStudioHdr object. 
 * @param pEntity Entity to get the model from
 * @return CStudioHdr pointer. IT MUST BE DELETED AFTER USE. Returns NULL on failure
*/
std::unique_ptr<CStudioHdr> UtilHelpers::GetEntityModelPtr(edict_t* pEntity)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("UtilHelpers::GetEntityModelPtr", "NavBot");
#endif // EXT_VPROF_ENABLED

	auto collide = pEntity->GetCollideable();

	if (!collide)
	{
		return nullptr;
	}

	auto model = collide->GetCollisionModel();

	if (!model)
	{
		return nullptr;
	}

	auto studiomodel = modelinfo->GetStudiomodel(model);

	if (!studiomodel)
	{
		return nullptr;
	}

	// TO-DO: Smart Pointers
	return std::make_unique<CStudioHdr>(studiomodel, imdlcache);
}

int UtilHelpers::LookupBone(CStudioHdr* hdr, const char* bonename)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("UtilHelpers::LookupBone", "NavBot");
#endif // EXT_VPROF_ENABLED

	// binary search for the bone matching pName
	int start = 0, end = hdr->numbones() - 1;
	const byte* pBoneTable = hdr->GetBoneTableSortedByName();
	mstudiobone_t* pbones = hdr->pBone(0);
	while (start <= end)
	{
		int mid = (start + end) >> 1;
		int cmp = Q_stricmp(pbones[pBoneTable[mid]].pszName(), bonename);

		if (cmp < 0)
		{
			start = mid + 1;
		}
		else if (cmp > 0)
		{
			end = mid - 1;
		}
		else
		{
			return pBoneTable[mid];
		}
	}

	return -1;
}

bool UtilHelpers::GetBonePosition(CBaseEntity* entity, int bone, Vector& origin, QAngle& angles)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("UtilHelpers::GetBonePosition", "NavBot");
#endif // EXT_VPROF_ENABLED

	if (!sdkcalls->IsGetBoneTransformAvailable())
	{
#ifdef EXT_DEBUG
		// alert developers
		smutils->LogError(myself, "UtilHelpers::GetBonePosition called but SDK Call to GetBoneTransform is not available!");
#endif // EXT_DEBUG

		return false;
	}

	matrix3x4_t matrix;
	sdkcalls->CBaseAnimating_GetBoneTransform(entity, bone, &matrix);
	MatrixAngles(matrix, angles, origin);
	return true;
}

bool UtilHelpers::GetBonePosition(CBaseEntity* entity, CStudioHdr* hdr, int bone, Vector& origin, QAngle& angles)
{
	if (!sdkcalls->IsGetBoneTransformAvailable())
	{
#ifdef EXT_DEBUG
		// alert developers
		smutils->LogError(myself, "UtilHelpers::GetBonePosition called but SDK Call to GetBoneTransform is not available!");
#endif // EXT_DEBUG

		return false;
	}

	if (bone < 0 || bone >= hdr->numbones())
	{
		return false;
	}

	matrix3x4_t matrix;
	sdkcalls->CBaseAnimating_GetBoneTransform(entity, bone, &matrix);
	MatrixAngles(matrix, angles, origin);
	return true;
}

/**
 * @brief Tests if a given entity index is in the player entity index range
 * @param index Entity index to check
 * @return TRUE if the given entity index in inside the reserved player entity range
*/
bool UtilHelpers::IsPlayerIndex(const int index)
{
	return index > 0 && index <= gpGlobals->maxClients;
}

bool UtilHelpers::FindNestedDataTable(SendTable* pTable, const char* name)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("UtilHelpers::FindNestedDataTable", "NavBot");
#endif // EXT_VPROF_ENABLED

	if (strcmp(pTable->GetName(), name) == 0)
	{
		return true;
	}

	int props = pTable->GetNumProps();
	SendProp* prop;

	for (int i = 0; i < props; i++)
	{
		prop = pTable->GetProp(i);
		if (prop->GetDataTable())
		{
			if (FindNestedDataTable(prop->GetDataTable(), name))
			{
				return true;
			}
		}
	}

	return false;
}

float UtilHelpers::GetEntityGravity(int entity)
{
	static bool gotconfig = false;
	static char datamap[32];

	if (!gotconfig)
	{
		SourceMod::IGameConfig* gamedata = nullptr;
		gameconfs->LoadGameConfigFile("core.games", &gamedata, nullptr, 0);
		auto key = gamedata->GetKeyValue("m_flGravity");
		gameconfs->CloseGameConfigFile(gamedata);

		if (key == nullptr)
		{
			ke::SafeSprintf(datamap, sizeof(datamap), "m_flGravity");
		}
		else
		{
#ifdef EXT_DEBUG
			smutils->LogMessage(myself, "Loaded gamedata for GetEntityGravity! \"%s\".", key);
#endif // EXT_DEBUG

			ke::SafeSprintf(datamap, sizeof(datamap), "%s", key);
		}

		gotconfig = true;
	}

	float gravity = 0.0f;
	entprops->GetEntPropFloat(entity, Prop_Data, datamap, gravity);
	return gravity;
}

MoveType_t UtilHelpers::GetEntityMoveType(int entity)
{
	static bool gotconfig = false;
	static char datamap[32];

	if (!gotconfig)
	{
		SourceMod::IGameConfig* gamedata = nullptr;
		gameconfs->LoadGameConfigFile("core.games", &gamedata, nullptr, 0);
		auto key = gamedata->GetKeyValue("m_MoveType");
		gameconfs->CloseGameConfigFile(gamedata);

		if (key == nullptr)
		{
			ke::SafeSprintf(datamap, sizeof(datamap), "m_MoveType");
		}
		else
		{
#ifdef EXT_DEBUG
			smutils->LogMessage(myself, "Loaded gamedata for GetEntityMoveType! \"%s\".", key);
#endif // EXT_DEBUG
			ke::SafeSprintf(datamap, sizeof(datamap), "%s", key);
		}

		gotconfig = true;
	}

	int movetype = 0;
	entprops->GetEntProp(entity, Prop_Data, datamap, movetype);
	return static_cast<MoveType_t>(movetype);
}

Vector UtilHelpers::GetNormalizedVector(const Vector& other)
{
	Vector norm = other;
	VectorNormalize(norm);
	return norm;
}

int UtilHelpers::GetEntityHealth(int entity)
{
	static bool gotconfig = false;
	static char datamap[32];

	if (!gotconfig)
	{
		SourceMod::IGameConfig* gamedata = nullptr;
		gameconfs->LoadGameConfigFile("core.games", &gamedata, nullptr, 0);
		auto key = gamedata->GetKeyValue("m_iHealth");
		gameconfs->CloseGameConfigFile(gamedata);

		if (key == nullptr)
		{
			ke::SafeSprintf(datamap, sizeof(datamap), "m_iHealth");
		}
		else
		{
#ifdef EXT_DEBUG
			smutils->LogMessage(myself, "Loaded gamedata for GetEntityHealth! \"%s\".", key);
#endif // EXT_DEBUG
			ke::SafeSprintf(datamap, sizeof(datamap), "%s", key);
		}

		gotconfig = true;
	}

	int health = 0;
	entprops->GetEntProp(entity, Prop_Data, datamap, health);
	return health;
}

bool UtilHelpers::IsPlayerAlive(const int player)
{
	auto gp = playerhelpers->GetGamePlayer(player);
	
	if (gp && gp->GetPlayerInfo()->IsDead())
	{
		return false;
	}

	int lifestate = 0;

	if (entprops->GetEntProp(player, Prop_Data, "m_lifeState", lifestate))
	{
		if (lifestate != LIFE_ALIVE)
		{
			return false;
		}
	}
#ifdef EXT_DEBUG
	else
	{
		// log mods that doesn't support 'm_lifeState'
		smutils->LogError(myself, "Warning: Failed to get m_lifeState propdata!");
	}
#endif // EXT_DEBUG


	return GetEntityHealth(player) > 0;
}

bool UtilHelpers::IsEntityAlive(CBaseEntity* entity)
{
	return entityprops::GetEntityLifeState(entity) == LIFE_ALIVE;
}

int UtilHelpers::GetNumberofPlayersOnTeam(const int team, const bool ignore_dead, const bool ignore_bots)
{
	int count = 0;

	for (int client = 1; client <= gpGlobals->maxClients; client++)
	{
		auto player = playerhelpers->GetGamePlayer(client);

		if (!player)
			continue;

		if (!player->IsInGame())
			continue;

		if (ignore_bots && player->IsFakeClient())
			continue;

		auto info = player->GetPlayerInfo();

		if (!info)
			continue;

		if (ignore_dead && info->IsDead())
			continue;

		if (info->GetTeamIndex() == team)
		{
			count++;
		}
	}

	return count;
}

/**
 * @brief Finds the team manager entity index
 * @param team Team index to search
 * @param classname classname of the team manager entity. (For example, in tf2 it's 'tf_team').
 * Use sourcemod to dump a list of entity classnames.
 * @return Entity index if found or NULL optional if not found.
*/
std::optional<int> UtilHelpers::GetTeamManagerEntity(const int team, const char* classname)
{
	int entity = INVALID_EHANDLE_INDEX;

	while ((entity = FindEntityByClassname(entity, classname)) != INVALID_EHANDLE_INDEX)
	{
		int entityteam = -1;

		if (!entprops->GetEntProp(entity, Prop_Send, "m_iTeamNum", entityteam)) // Entity lacks m_iTeamNum or it's not networked, fail
			continue;

		if (entityteam == team)
		{
			return entity;
		}
	}

	return std::nullopt;
}

std::optional<int> UtilHelpers::GetOwnerEntity(const int entity)
{
	int owner = INVALID_EHANDLE_INDEX;

	if (entprops->GetEntPropEnt(entity, Prop_Data, "m_hOwnerEntity", &owner))
	{
		return owner;
	}

	return std::nullopt;
}

void UtilHelpers::CalcClosestPointOnAABB(const Vector& mins, const Vector& maxs, const Vector& point, Vector& closestOut)
{
	closestOut.x = std::clamp(point.x, mins.x, maxs.x);
	closestOut.y = std::clamp(point.y, mins.y, maxs.y);
	closestOut.z = std::clamp(point.z, mins.z, maxs.z);
}

Vector UtilHelpers::GetGroundPositionFromCenter(edict_t* pEntity)
{
	Vector center = getWorldSpaceCenter(pEntity);

	trace::CTraceFilterSimple filter(reinterpret_cast<CBaseEntity*>(pEntity->GetIServerEntity()), COLLISION_GROUP_NONE);
	trace_t result;
	trace::line(center, center + Vector(0.0f, 0.0f, -4096.0f), MASK_SOLID, &filter, result);
	return result.endpos + Vector(0.0f, 0.0f, 1.0f);
}

const char* UtilHelpers::GetPlayerDebugIdentifier(int player)
{
	static char message[256];
	auto gp = playerhelpers->GetGamePlayer(player);

	if (!gp)
		return "";

	auto name = gp->GetName();
	ke::SafeSprintf(message, sizeof(message), "%s<#%i/%i>", name ? name : "", player, gp->GetUserId());
	return message;
}

const char* UtilHelpers::GetPlayerDebugIdentifier(edict_t* player)
{
	static char message[256];
	auto gp = playerhelpers->GetGamePlayer(player);

	if (!gp)
		return "";

	auto name = gp->GetName();
	ke::SafeSprintf(message, sizeof(message), "%s<#%i/%i>", name ? name : "", gamehelpers->IndexOfEdict(player), gp->GetUserId());
	return message;
}

CBaseEntity* UtilHelpers::GetNearestEntityOfClassname(const Vector& source, const char* classname, float maxRange)
{
	CBaseEntity* pBest = nullptr;
	float distance = std::numeric_limits<float>::max();
	float current = 0.0f;

	int entity = INVALID_EHANDLE_INDEX;
	while ((entity = UtilHelpers::FindEntityByClassname(entity, classname)) != INVALID_EHANDLE_INDEX)
	{
		CBaseEntity* pBaseEntity = nullptr;

		if (!IndexToAThings(entity, &pBaseEntity, nullptr))
			continue;

		entities::HBaseEntity be{ pBaseEntity };

		Vector center = be.WorldSpaceCenter();

		current = source.DistTo(center);

		if (current > maxRange)
			continue;

		if (current < distance)
		{
			distance = current;
			pBest = pBaseEntity;
		}
	}

	return pBest;
}

CBaseEntity* UtilHelpers::FindEntityByHammerID(int iHammerID)
{
#ifdef SDKIFACE_SERVERTOOLSV2_AVAILABLE
	return servertools->FindEntityByHammerID(iHammerID);
#elif SOURCE_ENGINE == SE_EPISODEONE
	return nullptr;
#else
	CBaseEntity* entity = static_cast<CBaseEntity*>(servertools->FirstEntity());

	while (entity != nullptr)
	{
		int* hammerid = entprops->GetPointerToEntData<int>(entity, Prop_Data, "m_iHammerID");

		if (hammerid && *hammerid == iHammerID)
		{
			return entity;
		}

		entity = static_cast<CBaseEntity*>(servertools->NextEntity(static_cast<void*>(entity)));
	}

	return nullptr;
#endif // SDKIFACE_SERVERTOOLSV2_AVAILABLE
}

int UtilHelpers::GetEntityHammerID(CBaseEntity* entity)
{
	int* hammerid = entprops->GetPointerToEntData<int>(entity, Prop_Data, "m_iHammerID");

	if (hammerid == nullptr)
	{
		return -1;
	}

	return *hammerid;
}

void UtilHelpers::FakeClientCommandKeyValues(edict_t* client, KeyValues* kv)
{
#if SOURCE_ENGINE >= SE_EYE
	gameclients->ClientCommandKeyValues(client, kv);
#else
	throw std::runtime_error("This engine branch does not support FakeClientCommandKeyValues!");
#endif
}

ServerClass* UtilHelpers::FindEntityServerClass(CBaseEntity* pEntity)
{
	IServerNetworkable* pNetwork = reinterpret_cast<IServerUnknown*>(pEntity)->GetNetworkable();

	if (pNetwork == nullptr)
	{
		return nullptr;
	}

	return pNetwork->GetServerClass();
}

bool UtilHelpers::StringMatchesPattern(const char* source, const char* pattern, int nFlags)
{
	const char* pszSource = source;
	const char* pszPattern = pattern;
	bool bExact = true;

	while (1)
	{
		if ((*pszPattern) == 0)
		{
			return ((*pszSource) == 0);
		}

		if ((*pszPattern) == '*')
		{
			pszPattern++;

			if ((*pszPattern) == 0)
			{
				return true;
			}

			bExact = false;
			continue;
		}

		int nLength = 0;

		while ((*pszPattern) != '*' && (*pszPattern) != 0)
		{
			nLength++;
			pszPattern++;
		}

		while (1)
		{
			const char* pszStartPattern = pszPattern - nLength;
			const char* pszSearch = pszSource;

			for (int i = 0; i < nLength; i++, pszSearch++, pszStartPattern++)
			{
				if ((*pszSearch) == 0)
				{
					return false;
				}

				if ((*pszSearch) != (*pszStartPattern))
				{
					break;
				}
			}

			if (pszSearch - pszSource == nLength)
			{
				break;
			}

			if (bExact == true)
			{
				return false;
			}

			if ((nFlags & UtilHelpers::PATTERN_DIRECTORY) != 0)
			{
				if ((*pszPattern) != '/' && (*pszSource) == '/')
				{
					return false;
				}
			}

			pszSource++;
		}

		pszSource += nLength;
	}
}

bool UtilHelpers::StringMatchesPattern(const std::string& source, const std::string& pattern, int nFlags)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("UtilHelpers::StringMatchesPattern( std::string )", "NavBot");
#endif // EXT_VPROF_ENABLED

	const char* pszSource = source.c_str();
	const char* pszPattern = pattern.c_str();
	bool bExact = true;

	while (1)
	{
		if ((*pszPattern) == 0)
		{
			return ((*pszSource) == 0);
		}

		if ((*pszPattern) == '*')
		{
			pszPattern++;

			if ((*pszPattern) == 0)
			{
				return true;
			}

			bExact = false;
			continue;
		}

		int nLength = 0;

		while ((*pszPattern) != '*' && (*pszPattern) != 0)
		{
			nLength++;
			pszPattern++;
		}

		while (1)
		{
			const char* pszStartPattern = pszPattern - nLength;
			const char* pszSearch = pszSource;

			for (int i = 0; i < nLength; i++, pszSearch++, pszStartPattern++)
			{
				if ((*pszSearch) == 0)
				{
					return false;
				}

				if ((*pszSearch) != (*pszStartPattern))
				{
					break;
				}
			}

			if (pszSearch - pszSource == nLength)
			{
				break;
			}

			if (bExact == true)
			{
				return false;
			}

			if ((nFlags & UtilHelpers::PATTERN_DIRECTORY) != 0)
			{
				if ((*pszPattern) != '/' && (*pszSource) == '/')
				{
					return false;
				}
			}

			pszSource++;
		}

		pszSource += nLength;
	}
}

bool UtilHelpers::FClassnameIs(edict_t* pEntity, const char* szClassname)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("UtilHelpers::FClassnameIs( edict )", "NavBot");
#endif // EXT_VPROF_ENABLED

	auto classname = pEntity->GetClassName();

	if (Q_strcmp(classname, szClassname) == 0) { return true; }

	std::string s1(classname);
	std::string s2(szClassname);

	return UtilHelpers::StringMatchesPattern(s1, s2, 0);
}

bool UtilHelpers::FClassnameIs(CBaseEntity* pEntity, const char* szClassname)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("UtilHelpers::FClassnameIs( CBaseEntity )", "NavBot");
#endif // EXT_VPROF_ENABLED

	auto classname = gamehelpers->GetEntityClassname(pEntity);

	if (Q_strcmp(classname, szClassname) == 0) { return true; }

	std::string s1(classname);
	std::string s2(szClassname);

	return UtilHelpers::StringMatchesPattern(s1, s2, 0);
}

bool UtilHelpers::FindSendPropOffset(CBaseEntity* entity, const char* prop, int& out)
{
	ServerClass* pClass = gamehelpers->FindEntityServerClass(entity);

	if (pClass != nullptr)
	{
		sm_sendprop_info_t info;
		if (gamehelpers->FindSendPropInfo(pClass->GetName(), prop, &info))
		{
			out = static_cast<int>(info.actual_offset);
			return true;
		}
	}

	out = -1;
	return false;
}

bool UtilHelpers::LineIntersectsAABB(const Vector& lineStart, const Vector& lineEnd, const Vector& origin, const Vector& mins, const Vector& maxs)
{
	Vector boxmins = (origin + mins);
	Vector boxmaxs = (origin + maxs);
	Vector dir = (lineEnd - lineStart);

	// Initialize the parameter values
	float tMin = 0.0f;
	float tMax = 1.0f;

	// Check intersection for each axis
	for (int i = 0; i < 3; i++)
	{
		float invDir = 1.0f / dir[i];
		float t0 = (boxmins[i] - lineStart[i]) * invDir;
		float t1 = (boxmaxs[i] - lineStart[i]) * invDir;

		// Ensure t0 is the minimum and t1 is the maximum
		if (invDir < 0.0f)
		{
			float temp = t0;
			t0 = t1;
			t1 = temp;
		}

		// Update tMin and tMax
		tMin = std::max(tMin, t0);
		tMax = std::min(tMax, t1);

		// If the ranges do not overlap, there is no intersection
		if (tMin > tMax)
		{
			return false;
		}
	}

	// If we reach here, the line intersects the AABB
	return true;
}

bool UtilHelpers::LineIntersectsAABB(const Vector& lineStart, const Vector& lineEnd, const Vector& mins, const Vector& maxs)
{
	Vector dir = (lineEnd - lineStart);

	// Initialize the parameter values
	float tMin = 0.0f;
	float tMax = 1.0f;

	// Check intersection for each axis
	for (int i = 0; i < 3; i++)
	{
		float invDir = 1.0f / dir[i];
		float t0 = (mins[i] - lineStart[i]) * invDir;
		float t1 = (maxs[i] - lineStart[i]) * invDir;

		// Ensure t0 is the minimum and t1 is the maximum
		if (invDir < 0.0f)
		{
			float temp = t0;
			t0 = t1;
			t1 = temp;
		}

		// Update tMin and tMax
		tMin = std::max(tMin, t0);
		tMax = std::min(tMax, t1);

		// If the ranges do not overlap, there is no intersection
		if (tMin > tMax)
		{
			return false;
		}
	}

	// If we reach here, the line intersects the AABB
	return true;
}

bool UtilHelpers::PointIsInsideAABB(const Vector& point, const Vector& origin, const Vector& mins, const Vector& maxs)
{
	Vector boxmins = (origin + mins);
	Vector boxmaxs = (origin + maxs);

	return PointIsInsideAABB(point, boxmins, boxmaxs);
}

bool UtilHelpers::PointIsInsideAABB(const Vector& point, const Vector& mins, const Vector& maxs)
{
	if (point.x > mins.x && point.x < maxs.x &&
		point.y > mins.y && point.y < maxs.y &&
		point.z > mins.z && point.z < maxs.z)
	{
		return true;
	}

	return false;
}

bool UtilHelpers::AABBIntersectsAABB(const Vector& mins1, const Vector& maxs1, const Vector& mins2, const Vector& maxs2)
{
	if (mins1.x <= maxs2.x &&
		maxs1.x >= mins2.x &&
		mins1.y <= maxs2.y &&
		maxs1.y >= mins2.y &&
		mins1.z <= maxs2.z &&
		maxs1.z >= mins2.z)
	{
		return true;
	}

	return false;
}

bool UtilHelpers::PointIsInsideSphere(const Vector& point, const Vector& sphereCenter, const float sphereRadius)
{
	float distance = sqrtf(powf((point.x - sphereCenter.x), 2.0f) + powf((point.y - sphereCenter.y), 2.0f) + powf((point.z - sphereCenter.z), 2.0f));
	return distance < sphereRadius;
}

bool UtilHelpers::PointIsInsideSphereSqr(const Vector& point, const Vector& sphereCenter, const float sphereRadius)
{
	float distance = powf((point.x - sphereCenter.x), 2.0f) + powf((point.y - sphereCenter.y), 2.0f) + powf((point.z - sphereCenter.z), 2.0f);
	return distance < sphereRadius * sphereRadius;
}

void UtilHelpers::GetRandomPointInsideAABB(const Vector& mins, const Vector& maxs, Vector& out, const float margin)
{
	out.x = randomgen->GetRandomReal<float>(mins.x + margin, maxs.x - margin);
	out.y = randomgen->GetRandomReal<float>(mins.y + margin, maxs.y - margin);
	out.z = randomgen->GetRandomReal<float>(mins.z + margin, maxs.z - margin);
}

std::size_t UtilHelpers::EntitiesInBox(const Vector& mins, const Vector& maxs, CEntityEnumerator& enumerator, SpatialPartitionListMask_t mask, bool coarseTest)
{
	partition->EnumerateElementsInBox(mask, mins, maxs, coarseTest, &enumerator);
	return enumerator.Count();
}

std::size_t UtilHelpers::EntitiesInSphere(const Vector& origin, float radius, CEntityEnumerator& enumerator, SpatialPartitionListMask_t mask, bool coarseTest)
{
	partition->EnumerateElementsInSphere(mask, origin, radius, coarseTest, &enumerator);
	return enumerator.Count();
}

bool UtilHelpers::EntityDerivesFrom(CBaseEntity* pEntity, const char* classname)
{
	datamap_t* dmap = gamehelpers->GetDataMap(pEntity);

	while (dmap != nullptr)
	{
		if (V_stricmp(dmap->dataClassName, classname) == 0)
		{
			return true;
		}

		dmap = dmap->baseMap;
	}

	return false;
}

UtilHelpers::CEntityEnumerator::CEntityEnumerator()
{
	m_ents.reserve(256);
}

IterationRetval_t UtilHelpers::CEntityEnumerator::EnumElement(IHandleEntity* pHandleEntity)
{
	if (m_filter)
	{
		if (m_filter(pHandleEntity) == false)
		{
			return IterationRetval_t::ITERATION_CONTINUE;
		}
	}

	m_ents.push_back(reinterpret_cast<CBaseEntity*>(pHandleEntity));
	return IterationRetval_t::ITERATION_CONTINUE;
}

bool UtilHelpers::PredPlayersInTeam::operator()(int client, edict_t* entity, SourceMod::IGamePlayer* player)
{
	int client_team = 0;
	entprops->GetEntProp(client, Prop_Send, "m_iTeamNum", client_team);
	return client_team == m_team;
}

bool UtilHelpers::parsers::ParseRandomInt(const char* str, int& out, const int min, const int max)
{
	try
	{
		std::string string{ str };
		std::string::size_type dividerpos = string.find_first_of(':');

		if (dividerpos == std::string::npos)
		{
			return false;
		}

		std::string first = string.substr(0, dividerpos);
		std::string second = string.substr(dividerpos + 1U);
		int smin = std::stoi(first);
		int smax = std::stoi(second);
		smin = std::max(smin, min);
		smax = std::min(smax, max);

		if (smin == smax)
		{
			out = smax;
			return true;
		}

		if (smin > smax)
		{
			return false;
		}

		out = randomgen->GetRandomInt<int>(smin, smax);
		return true;
	}
	catch (const std::exception& ex)
	{
		smutils->LogError(myself, "C++ Exception throw! %s", ex.what());
		out = 0;
		return false;
	}
}

CBaseEntity* UtilHelpers::players::GetRandomTeammate(CBaseEntity* me, const bool alive, const bool onlyhumans)
{
	std::vector<CBaseEntity*> candidates;
	candidates.reserve(ABSOLUTE_PLAYER_LIMIT);

	int myteam = -2;
	entprops->GetEntProp(me, Prop_Send, "m_iTeamNum", myteam);

	for (int client = 1; client <= gpGlobals->maxClients; client++)
	{
		SourceMod::IGamePlayer* gameplayer = playerhelpers->GetGamePlayer(client);

		if (!gameplayer || !gameplayer->IsInGame())
		{
			continue;
		}

		if (onlyhumans && gameplayer->IsFakeClient())
		{
			continue;
		}

		edict_t* edict = gamehelpers->EdictOfIndex(client);

		if (!UtilHelpers::IsValidEdict(edict))
		{
			continue;
		}

		CBaseEntity* ent = UtilHelpers::EdictToBaseEntity(edict);

		if (!ent || ent == me)
		{
			continue;
		}

		if (alive && !UtilHelpers::IsPlayerAlive(client))
		{
			continue;
		}

		int theirteam = -1;
		entprops->GetEntProp(me, Prop_Send, "m_iTeamNum", theirteam);

		if (myteam == theirteam)
		{
			candidates.push_back(ent);
		}
	}

	if (candidates.empty())
	{
		return nullptr;
	}

	if (candidates.size() == 1U)
	{
		return candidates[0];
	}

	return librandom::utils::GetRandomElementFromVector(candidates);
}

int UtilHelpers::studio::LookupBone(CBaseEntity* pEntity, const char* bone)
{
	const model_t* model = reinterpret_cast<IServerUnknown*>(pEntity)->GetCollideable()->GetCollisionModel();

	if (!model)
	{
		return INVALID_BONE_INDEX;
	}

	const studiohdr_t* studiomodel = modelinfo->GetStudiomodel(model);

	if (!studiomodel)
	{
		return INVALID_BONE_INDEX;
	}

	const int n = studiomodel->numbones;

	for (int i = 0; i < n; i++)
	{
		const mstudiobone_t* pBone = studiomodel->pBone(i);
		const char* name = pBone->pszName();

		if (V_strcmp(name, bone) == 0)
		{
			return i;
		}
	}

	return INVALID_BONE_INDEX;
}

bool UtilHelpers::studio::GetBonePosition(CBaseEntity* pEntity, int bone, Vector& position, QAngle& angles)
{
	static matrix3x4_t mat;

	if (!sdkcalls->IsGetBoneTransformAvailable()) { return false; }

	mat.Init(vec3_origin, vec3_origin, vec3_origin, vec3_origin);

	sdkcalls->CBaseAnimating_GetBoneTransform(pEntity, bone, &mat);
	MatrixAngles(mat, angles, position);

	return true;
}
