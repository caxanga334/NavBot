#include <stdexcept>

#include <extension.h>
#include <server_class.h>
#include <studio.h>
#include "helpers.h"
#include "entprops.h"
#include "UtilTrace.h"

#undef max // undef mathlib defs to avoid comflicts with STD
#undef min
#undef clamp

edict_t* UtilHelpers::BaseEntityToEdict(CBaseEntity* pEntity)
{
	IServerUnknown* pUnk = (IServerUnknown*)pEntity;
	IServerNetworkable* pNet = pUnk->GetNetworkable();

	if (!pNet)
	{
		return nullptr;
	}

	return pNet->GetEdict();
}

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

/**
 * Gets the entity world center. Clone of WorldSpaceCenter()
 * @param pEntity	The entity to get the center from
 * @return			Center vector
 **/
Vector UtilHelpers::getWorldSpaceCenter(edict_t* pEntity)
{
	Vector result = getOBBCenter(pEntity);
	result = collisionToWorldSpace(result, pEntity);
	return result;
}

/**
 * Checks if a point is within a trigger
 *
 * @param pEntity	The trigger entity
 * @param vPoint	The point to be tested
 * @return			True if the given point is within pEntity
 **/
bool UtilHelpers::pointIsWithinTrigger(edict_t* pEntity, const Vector& vPoint)
{
	Ray_t ray;
	trace_t tr;
	ICollideable* pCollide = pEntity->GetCollideable();
	ray.Init(vPoint, vPoint);
	enginetrace->ClipRayToCollideable(ray, MASK_ALL, pCollide, &tr);
	return (tr.startsolid);
}

bool UtilHelpers::isBoundsDefinedInEntitySpace(edict_t* pEntity)
{
	return ((pEntity->GetCollideable()->GetSolidFlags() & FSOLID_FORCE_WORLD_ALIGNED) == 0 &&
		pEntity->GetCollideable()->GetSolid() != SOLID_BBOX && pEntity->GetCollideable()->GetSolid() != SOLID_NONE);
}

/// @brief Searches for entities by classname
/// @return Entity index/reference or INVALID_EHANDLE_INDEX if none is found
int UtilHelpers::FindEntityByClassname(int start, const char* searchname)
{
#ifdef SDKCOMPAT_HAS_SERVERTOOLSV2
	CBaseEntity* pEntity = servertools->FindEntityByClassname(GetEntity(start), searchname);
	return gamehelpers->EntityToBCompatRef(pEntity);
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
int UtilHelpers::FindEntityInSphere(int start, Vector center, float radius)
{
#ifdef SDKCOMPAT_HAS_SERVERTOOLSV2
	CBaseEntity* pEntity = servertools->FindEntityInSphere(GetEntity(start), center, radius);
	return gamehelpers->EntityToBCompatRef(pEntity);
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
		if (current == nullptr || current->IsFree())
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
#ifdef SDKCOMPAT_HAS_SERVERTOOLSV2
	auto result = servertools->FindEntityByName(UtilHelpers::GetEntity(start), targetname);
	return gamehelpers->EntityToBCompatRef(result);
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
		int length;
		
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
bool UtilHelpers::PointWithinViewAngle(Vector const& vecSrcPosition, Vector const& vecTargetPosition, Vector const& vecLookDirection, float flCosHalfFOV)
{
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
CStudioHdr* UtilHelpers::GetEntityModelPtr(edict_t* pEntity)
{
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
	return new CStudioHdr(studiomodel, imdlcache);
}

/**
 * @brief Gets the index of a bone from the given model.
 * @param hdr CStudioHDR pointer of the model you want to get the bone from
 * @param bonename Name of the bone
 * @return Bone index or -1 on failure
*/
int UtilHelpers::LookupBone(CStudioHdr* hdr, const char* bonename)
{
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

bool UtilHelpers::IsEntityAlive(const int entity)
{
	if (IsPlayerIndex(entity))
	{
		return IsPlayerAlive(entity);
	}

	return GetEntityHealth(entity) > 0;
}

bool UtilHelpers::IsPlayerAlive(const int player)
{
	auto gp = playerhelpers->GetGamePlayer(player);
	
	if (gp && gp->GetPlayerInfo()->IsDead())
	{
		return false;
	}

	return GetEntityHealth(player) > 0;
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

	if (entprops->GetEntPropEnt(entity, Prop_Data, "m_hOwnerEntity", owner))
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

	CTraceFilterSimple filter(pEntity->GetIServerEntity(), COLLISION_GROUP_NONE, nullptr);
	trace_t result;
	UTIL_TraceLine(center, center + Vector(0.0f, 0.0f, -4096.0f), MASK_SOLID, &filter, &result);
	return result.endpos + Vector(0.0f, 0.0f, 1.0f);
}

