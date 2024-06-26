#include <stdexcept>

#include <extension.h>
#include <sdkports/sdk_traces.h>
#include <server_class.h>
#include <studio.h>
#include "helpers.h"
#include "entprops.h"

#undef max // undef mathlib defs to avoid comflicts with STD
#undef min
#undef clamp

#if SOURCE_ENGINE == SE_CSS || SOURCE_ENGINE == SE_DODS || SOURCE_ENGINE == SE_TF2 || SOURCE_ENGINE == SE_HL2DM || \
SOURCE_ENGINE == SE_BMS || SOURCE_ENGINE == SE_SDK2013

// Source SDKs which server tools V2 interface is available
#define SDKIFACE_SERVERTOOLSV2_AVAILABLE

#endif 


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
		return true;
	}

	edict_t* edict = pNet->GetEdict();
	return (edict && !edict->IsFree()) ? true : false;
}

bool UtilHelpers::IsEntNetworkable(CBaseEntity* entity)
{
	IServerNetworkable* pNet = reinterpret_cast<IServerUnknown*>(entity)->GetNetworkable();

	if (!pNet)
	{
		return true;
	}

	edict_t* edict = pNet->GetEdict();
	return (edict && !edict->IsFree()) ? true : false;
}

bool UtilHelpers::HasDataTable(SendTable* root, const char* name)
{
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

			if (FindDataTable(table,
				name,
				info,
				offset + prop->GetOffset())
				)
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
#ifdef SDKIFACE_SERVERTOOLSV2_AVAILABLE
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
int UtilHelpers::FindEntityInSphere(int start, const Vector& center, float radius)
{
#ifdef SDKIFACE_SERVERTOOLSV2_AVAILABLE
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
#ifdef SDKIFACE_SERVERTOOLSV2_AVAILABLE
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
	auto classname = pEntity->GetClassName();

	if (Q_strcmp(classname, szClassname) == 0) { return true; }

	std::string s1(classname);
	std::string s2(szClassname);

	return UtilHelpers::StringMatchesPattern(s1, s2, 0);
}

bool UtilHelpers::FClassnameIs(CBaseEntity* pEntity, const char* szClassname)
{
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

