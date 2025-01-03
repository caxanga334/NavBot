#ifndef UTIL_HELPERS_H_
#define UTIL_HELPERS_H_
#pragma once

#include <vector>
#include <optional>
#include <extension.h>

struct edict_t;
class CBaseEntity;
class Vector;
class CStudioHdr;
class SendTable;
class CBaseHandle;
class KeyValues;
class ServerClass;

namespace UtilHelpers
{
	edict_t* BaseEntityToEdict(CBaseEntity* pEntity);
	edict_t* GetEdict(int entity);
	CBaseEntity* GetEntity(int entity);
	bool IndexToAThings(int num, CBaseEntity** pEntData, edict_t** pEdictData);
	// True if this is a valid CBaseEntity index
	inline bool IsValidEntity(int index)
	{
		return gamehelpers->ReferenceToEntity(index) != nullptr;
	}
	// True if this is a valid edict index
	inline bool IsValidEdict(int index)
	{
		edict_t* edict = nullptr;

		if (!IndexToAThings(index, nullptr, &edict))
		{
			return false;
		}

		return edict != nullptr;
	}

	inline bool IsValidEdict(edict_t* edict)
	{
		if (edict == nullptr || edict->IsFree() || edict->GetIServerEntity() == nullptr)
		{
			return false;
		}

		return true;
	}

	int IndexOfEntity(CBaseEntity* entity);
	inline bool IsPlayer(CBaseEntity* entity);
	// Returns whether or not an entity has a valid networkable edict.
	bool IsEntNetworkable(int index);
	// Returns whether or not an entity has a valid networkable edict.
	bool IsEntNetworkable(CBaseEntity* entity);
	// Checks if the given sendtable contains a networked table. WARNING: BAD CPU PERF
	bool HasDataTable(SendTable* root, const char* name);
	bool FindDataTable(SendTable* pTable, const char* name, sm_sendprop_info_t* info, unsigned int offset);
	Vector getOBBCenter(edict_t* pEntity);
	Vector collisionToWorldSpace(const Vector& in, edict_t* pEntity); 
	Vector collisionToWorldSpace(const Vector& in, ICollideable* collider);
	Vector getWorldSpaceCenter(edict_t* pEntity);
	Vector getWorldSpaceCenter(CBaseEntity* pEntity);
	const Vector& getEntityOrigin(edict_t* entity);
	const Vector& getEntityOrigin(CBaseEntity* entity);
	void getEntityBounds(edict_t* entity, Vector& mins, Vector& maxs);
	void getEntityBounds(CBaseEntity* entity, Vector& mins, Vector& maxs);
	bool pointIsWithinTrigger(edict_t* pEntity, const Vector& vPoint);
	bool isBoundsDefinedInEntitySpace(edict_t* pEntity);
	bool isBoundsDefinedInEntitySpace(ICollideable* collider);
	int FindEntityByClassname(int start, const char* searchname);
	int FindEntityInSphere(int start, const Vector& center, float radius);
	int FindEntityByNetClass(int start, const char* classname);
	int FindEntityByTargetname(int start, const char* targetname);
	int FindNamedEntityByClassname(int start, const char* targetname, const char* classname);
	bool PointWithinViewAngle(const Vector& vecSrcPosition, const Vector& vecTargetPosition, const Vector& vecLookDirection, float flCosHalfFOV);
	float GetForwardViewCone(float angle);
	CStudioHdr* GetEntityModelPtr(edict_t* pEntity);
	int LookupBone(CStudioHdr* hdr, const char* bonename);
	// TO-DO: Investigate but the bone cache might complicate things
	// bool GetBoneTransform(CStudioHdr* hdr, int iBone, matrix3x4_t& pBoneToWorld);
	// bool GetBonePosition(CStudioHdr* hdr, int iBone, Vector& origin, QAngle& angles);

	bool IsPlayerIndex(const int index);
	bool FindNestedDataTable(SendTable* pTable, const char* name);
	float GetEntityGravity(int entity);
	MoveType_t GetEntityMoveType(int entity);
	// Vector::Normalized is not available in all SDK version
	Vector GetNormalizedVector(const Vector& other);
	int GetEntityHealth(int entity);
	bool IsPlayerAlive(const int player);
	inline bool IsEntityAlive(const int entity)
	{
		if (IsPlayerIndex(entity))
		{
			return IsPlayerAlive(entity);
		}

		return GetEntityHealth(entity) > 0;
	}
	bool IsEntityAlive(CBaseEntity* entity);
	int GetNumberofPlayersOnTeam(const int team, const bool ignore_dead = false, const bool ignore_bots = false);
	std::optional<int> GetTeamManagerEntity(const int team, const char* classname);
	std::optional<int> GetOwnerEntity(const int entity);
	void CalcClosestPointOnAABB(const Vector& mins, const Vector& maxs, const Vector& point, Vector& closestOut);
	Vector GetGroundPositionFromCenter(edict_t* pEntity);
	
	/**
	 * @brief Collects player into a vector of ints containing their indexes
	 * @tparam T A class with an operator() overload with two parameters (int client, edict_t* entity, SourceMod::IGamePlayer* player), return true to add the client to the vector
	 * @param playersvector Vector to store the players indexes
	 * @param functor Player collection filter, only called to clients that are valid and in game.
	 */
	template <typename T>
	void CollectPlayers(std::vector<int>& playersvector, T functor);
	const char* GetPlayerDebugIdentifier(int player);
	const char* GetPlayerDebugIdentifier(edict_t* player);
	
	inline edict_t* GetEdictFromCBaseHandle(const CBaseHandle& handle)
	{
		if (!handle.IsValid())
			return nullptr;

		int index = handle.GetEntryIndex();
		edict_t* edict = nullptr;
		CBaseEntity* entity = nullptr;

		if (!IndexToAThings(index, &entity, &edict))
		{
			return nullptr;
		}

		if (!edict || !entity)
		{
			return nullptr;
		}

		auto se = edict->GetIServerEntity();

		if (se->GetRefEHandle() != handle)
		{
			return nullptr;
		}

		return edict;
	}

	inline CBaseEntity* GetBaseEntityFromCBaseHandle(const CBaseHandle& handle)
	{
		if (!handle.IsValid())
			return nullptr;

		int index = handle.GetEntryIndex();

		CBaseEntity* entity = nullptr;

		if (!IndexToAThings(index, &entity, nullptr))
		{
			return nullptr;
		}

		if (!entity)
		{
			return nullptr;
		}

		IServerEntity* se = reinterpret_cast<IServerEntity*>(entity);

		if (se->GetRefEHandle() != handle)
		{
			return nullptr;
		}

		return entity;
	}

	inline void SetHandleEntity(CBaseHandle& handle, edict_t* entity)
	{
		IServerEntity* se = entity->GetIServerEntity();

		if (se == nullptr)
		{
			return;
		}

		handle.Set(se);
	}

	inline void SetHandleEntity(CBaseHandle& handle, CBaseEntity* entity)
	{
		IServerEntity* se = reinterpret_cast<IServerEntity*>(entity);
		handle.Set(se);
	}

	/**
	 * @brief Runs a function on every valid edict
	 * @tparam T Function to call. Overload operator() with 1 parameter (edict_t* edict)
	 * @param functor Function to call
	 */
	template <typename T>
	inline void ForEachEdict(T functor)
	{
		for (int i = 0; i < gpGlobals->maxEntities; i++)
		{
			auto edict = gamehelpers->EdictOfIndex(i);

			if (!edict || edict->IsFree() || edict->GetIServerEntity() == nullptr || edict->GetUnknown() == nullptr)
				continue;

			functor(edict);
		}
	}

	/**
	 * @brief Counts the number of players.
	 * @tparam T A class with operator() overload with 3 parameters (int client, edict_t* entity, SourceMod::IGamePlayer* player)
	 * @param functor Function to run on each player. Return true to count the player.
	 * @return Number of players counted.
	 */
	template <typename T>
	inline int CountPlayers(T functor)
	{
		int playercount = 0;

		for (int i = 1; i <= gpGlobals->maxClients; i++)
		{
			auto edict = gamehelpers->EdictOfIndex(i);

			if (!edict || edict->IsFree() || edict->GetIServerEntity() == nullptr || edict->GetUnknown() == nullptr)
				continue;

			auto player = playerhelpers->GetGamePlayer(i);

			if (functor(i, edict, player))
			{
				++playercount;
			}
		}

		return playercount;
	}

	/**
	 * @brief Runs a function on each player.
	 * @tparam T A class with operator() overload with 3 parameters (int client, edict_t* entity, SourceMod::IGamePlayer* player)
	 * @param functor Function to run on each player.
	 */
	template <typename T>
	inline void ForEachPlayer(T functor)
	{
		for (int i = 1; i <= gpGlobals->maxClients; i++)
		{
			edict_t* edict = gamehelpers->EdictOfIndex(i);

			if (!edict || edict->IsFree() || edict->GetIServerEntity() == nullptr || edict->GetUnknown() == nullptr)
				continue;

			SourceMod::IGamePlayer* player = playerhelpers->GetGamePlayer(i);

			if (!player)
				continue;

			functor(i, edict, player);
		}
	}

	/**
	 * @brief Runs a function on each entity of the given classname found.
	 * @tparam T A class with bool operator() overload with 3 parameter (int index, edict_t* edict, CBaseEntity* entity), Edict may be null if the entity is not networked. Return false to exit early.
	 * @param classname Entity classname to search
	 * @param functor Function to run
	 */
	template <typename T>
	inline void ForEachEntityOfClassname(const char* classname ,T functor)
	{
		int entity = INVALID_EHANDLE_INDEX;
		while ((entity = UtilHelpers::FindEntityByClassname(entity, classname)) != INVALID_EHANDLE_INDEX)
		{
			edict_t* edict = nullptr;
			CBaseEntity* be = nullptr;

			if (!IndexToAThings(entity, &be, &edict))
				continue;

			if (functor(entity, edict, be) == false)
			{
				return;
			}
		}
	}

	/**
	 * @brief Runs a function on each entity inside the sphere's radius.
	 * @tparam T A class with bool operator() overload with 3 parameter (int index, edict_t* edict, CBaseEntity* entity), Edict may be null if the entity is not networked. Return false to exit early.
	 * @param center Search center point.
	 * @param radius Search sphere radius.
	 * @param functor Function to run.
	 */
	template <typename T>
	inline void ForEachEntityInSphere(const Vector& center, float radius, T functor)
	{
		int entity = INVALID_EHANDLE_INDEX;
		while ((entity = UtilHelpers::FindEntityInSphere(entity, center, radius)) != INVALID_EHANDLE_INDEX)
		{
			edict_t* edict = nullptr;
			CBaseEntity* be = nullptr;

			if (!IndexToAThings(entity, &be, &edict))
				continue;

			if (functor(entity, edict, be) == false)
			{
				return;
			}
		}
	}

	/**
	 * @brief Runs a function on every entity.
	 * @tparam T A function with the following parameters: (int index, edict_t* edict, CBaseEntity* entity)
	 * @param functor Function to call.
	 */
	template <typename T>
	inline void ForEveryEntity(T functor)
	{
		CBaseHandle next = g_EntList->FirstHandle();

		do
		{
			CBaseEntity* pEntity = UtilHelpers::GetBaseEntityFromCBaseHandle(next);
			edict_t* pEdict = UtilHelpers::GetEdictFromCBaseHandle(next);
			int index = next.GetEntryIndex();

			functor(index, pEdict, pEntity);

			next = g_EntList->NextHandle(next);

		} while (next.IsValid());

	}

	// Gets the listen server host entity. NULL if on dedicated server.
	inline edict_t* GetListenServerHost()
	{
		if (!engine->IsDedicatedServer())
		{
			return gamehelpers->EdictOfIndex(1);
		}

		return nullptr;
	}

	/**
	 * @brief Gets the nearest entity from the source of the given classname.
	 * @param source Source position for distance calculations.
	 * @param classname Entity classname to search.
	 * @param maxRange Maximum entity search range.
	 * @return CBaseEntity pointer of the entity or NULL if none is found.
	 */
	CBaseEntity* GetNearestEntityOfClassname(const Vector& source, const char* classname, float maxRange = 65535.0f);
	/**
	 * @brief Searches for an entity with a matching hammer ID.
	 * @param iHammerID Hammer id to search.
	 * @return Entity if found. NULL if not found.
	 */
	CBaseEntity* FindEntityByHammerID(int iHammerID);
	/**
	 * @brief Gets an entity hammer ID.
	 * @param entity Entity to read the hammer ID from.
	 * @return Hammer ID or -1 on failure.
	 */
	int GetEntityHammerID(CBaseEntity* entity);

	/**
	 * @brief Calls ClientCommandKeyValues for the given client. Throws an exception if the engine branch doesn't support keyvalue commands.
	 * @param client Client that will be sending the command.
	 * @param kv Keyvalue command.
	 */
	void FakeClientCommandKeyValues(edict_t* client, KeyValues* kv);

	/**
	 * @brief Finds a given entity's server class.
	 *
	 * @return				ServerClass pointer on success, nullptr on failure.
	 * 
	 * @note Ported from Sourcemod to here for compatibility reasons. Also available in gamehelpers but requires a very recent SM dev build
	 */
	ServerClass* FindEntityServerClass(CBaseEntity* pEntity);

	constexpr int PATTERN_DIRECTORY = 0x00000001;

	bool StringMatchesPattern(const char* source, const char* pattern, int nFlags);
	bool StringMatchesPattern(const std::string& source, const std::string& pattern, int nFlags);

	bool FClassnameIs(edict_t* pEntity, const char* szClassname);
	bool FClassnameIs(CBaseEntity* pEntity, const char* szClassname);

	/**
	 * @brief Finds the offset for a given networked property on an entity
	 * @param entity Entity that contains the property
	 * @param prop Name of the property to search for
	 * @param out Offset value.
	 * @return 
	 */
	bool FindSendPropOffset(CBaseEntity* entity, const char* prop, int& out);

	/**
	 * @brief Checks if a line intersects with an Axis Aligned Bounding Box
	 * @param lineStart Line start position
	 * @param lineEnd Line end position
	 * @param origin AABB origin
	 * @param mins AABB minimum extent
	 * @param maxs AABB maximum extent
	 * @return true if the line intersects with the AABB, false otherwise.
	 */
	bool LineIntersectsAABB(const Vector& lineStart, const Vector& lineEnd, const Vector& origin, const Vector& mins, const Vector& maxs);
	/**
	 * @brief Checks if a line intersects with an Axis Aligned Bounding Box
	 * @param lineStart Line start position
	 * @param lineEnd Line end position
	 * @param mins AABB minimum extent
	 * @param maxs AABB maximum extent
	 * @return true if the line intersects with the AABB, false otherwise.
	 */
	bool LineIntersectsAABB(const Vector& lineStart, const Vector& lineEnd, const Vector& mins, const Vector& maxs);

	/**
	 * @brief Checks if the given point is inside an AABB.
	 * @param point Point to check.
	 * @param origin AABB origin.
	 * @param mins AABB minimum extent
	 * @param maxs AABB maximum extent
	 * @return True if the point is inside the AABB. False otherwise.
	 */
	bool PointIsInsideAABB(const Vector& point, const Vector& origin, const Vector& mins, const Vector& maxs);
	/**
	 * @brief Checks if the given point is inside an AABB.
	 * @param point Point to check.
	 * @param mins AABB minimum extent
	 * @param maxs AABB maximum extent
	 * @return 
	 */
	bool PointIsInsideAABB(const Vector& point, const Vector& mins, const Vector& maxs);

	/**
	 * @brief Checks if an AABB intersects with another AABB.
	 * @param mins1 First AABB mins.
	 * @param maxs1 First AABB maxs.
	 * @param mins2 Second AABB mins.
	 * @param maxs2 Second AABB maxs.
	 * @return True if they intersect each other. False otherwise.
	 */
	bool AABBIntersectsAABB(const Vector& mins1, const Vector& maxs1, const Vector& mins2, const Vector& maxs2);

	/**
	 * @brief Checks if a point is inside a sphere.
	 * @param point Point to check.
	 * @param sphereCenter Sphere center position.
	 * @param sphereRadius Sphere radius.
	 * @return True if the given point is inside the sphere. False otherwise.
	 */
	bool PointIsInsideSphere(const Vector& point, const Vector& sphereCenter, const float sphereRadius);
	/**
	 * @brief Checks if a point is inside a sphere.
	 * 
	 * This version doesn't use square root.
	 * @param point Point to check.
	 * @param sphereCenter Sphere center position.
	 * @param sphereRadius Sphere radius.
	 * @return True if the given point is inside the sphere. False otherwise.
	 */
	bool PointIsInsideSphereSqr(const Vector& point, const Vector& sphereCenter, const float sphereRadius);

	void GetRandomPointInsideAABB(const Vector& mins, const Vector& maxs, Vector& out, const float margin = 0.0f);

	/**
	 * @brief Converts a string to boolean.
	 * 
	 * Supports the following words: yes no true false
	 * 
	 * Any non zero number will be considered as true.
	 * @param str String to convert.
	 * @return Boolean output.
	 */
	inline bool StringToBoolean(const char* str)
	{
		if (strncasecmp(str, "yes", 3) == 0)
		{
			return true;
		}
		else if (strncasecmp(str, "true", 4) == 0)
		{
			return true;
		}
		else if (strncasecmp(str, "no", 2) == 0)
		{
			return false;
		}
		else if (strncasecmp(str, "false", 5) == 0)
		{
			return false;
		}

		return atoi(str) != 0;
	}

	inline const char* GetEngineBranchName()
	{
		switch (SOURCE_ENGINE)
		{
		case SE_TF2:
			return "Team Fortress 2 Branch";
		case SE_ORANGEBOX:
			return "Orange Box Branch";
		case SE_CSS:
			return "Counter-Strike: Source Branch";
		case SE_CSGO:
			return "Counter-Strike: Global Offensive Branch";
		case SE_SDK2013:
			return "Source SDK 2013 Branch";
		case SE_HL2DM:
			return "Half-Life 2: Deathmatch Branch";
		case SE_DODS:
			return "Day of Defeat: Source Branch";
		case SE_EPISODEONE:
			return "Episode One/Original Branch";
		case SE_BMS:
			return "Black Mesa Branch";
		case SE_LEFT4DEAD:
			return "Left 4 Dead Branch";
		case SE_LEFT4DEAD2:
			return "Left 4 Dead 2 Branch";
		case SE_PORTAL2:
			return "Portal 2 Branch";
		case SE_ALIENSWARM:
			return "Alien Swarm Branch";
		case SE_INSURGENCY:
			return "Insurgency Branch";
		default:
			return "Unknown";
		}
	}
}

template<typename T>
inline void UtilHelpers::CollectPlayers(std::vector<int>& playersvector, T functor)
{
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		auto edict = gamehelpers->EdictOfIndex(i);

		if (!edict)
			continue;

		if (edict->IsFree())
			continue;

		if (edict->GetUnknown() == nullptr)
			continue;

		auto gp = playerhelpers->GetGamePlayer(i);

		if (!gp)
			continue;

		if (!gp->IsInGame())
			continue;

		if (functor(i, edict, gp))
		{
			playersvector.push_back(i);
		}
	}
}

#endif // !UTIL_HELPERS_H_



