#ifndef UTIL_HELPERS_H_
#define UTIL_HELPERS_H_
#pragma once

#include <memory>
#include <vector>
#include <optional>
#include <functional>
#include <extension.h>

struct edict_t;
class CBaseEntity;
class Vector;
class CStudioHdr;
class SendTable;
class CBaseHandle;
class KeyValues;
class ServerClass;
class CBaseEntityOutput;
class variant_t;
struct typedescription_t;

namespace UtilHelpers
{
	inline edict_t* BaseEntityToEdict(CBaseEntity* pEntity)
	{
		return servergameents->BaseEntityToEdict(pEntity);
	}
	inline CBaseEntity* EdictToBaseEntity(edict_t* edict)
	{
		return servergameents->EdictToBaseEntity(edict);
	}
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
	bool IsPlayerIndex(const int index);
	inline bool IsPlayer(CBaseEntity* entity) { return IsPlayerIndex(IndexOfEntity(entity)); }
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
	Vector WorldToCollisionSpace(ICollideable* collider, const Vector& in, Vector* pResult);
	Vector getWorldSpaceCenter(edict_t* pEntity);
	Vector getWorldSpaceCenter(CBaseEntity* pEntity);
	const Vector& getEntityOrigin(edict_t* entity);
	const Vector& getEntityOrigin(CBaseEntity* entity);
	/**
	 * @brief Reimplementation of CBaseEntity::GetLocalOrigin
	 * @param entity Entity to get the local origin from
	 * @return Entity local origin vector.
	 */
	Vector getEntityLocalOrigin(CBaseEntity* entity);
	const QAngle& getEntityAngles(edict_t* entity);
	const QAngle& getEntityAngles(CBaseEntity* entity);
	void getEntityBounds(edict_t* entity, Vector& mins, Vector& maxs);
	void getEntityBounds(CBaseEntity* entity, Vector& mins, Vector& maxs);
	/**
	 * Checks if a point is within a trigger
	 *
	 * @param pEntity	The trigger entity
	 * @param vPoint	The point to be tested
	 * @return			True if the given point is within pEntity
	 **/
	bool pointIsWithinTrigger(edict_t* pEntity, const Vector& vPoint);
	/**
	 * @brief Checks if the given position is within the given entity's trigger volume.
	 * @param pEntity Entity to test.
	 * @param vPoint Position to test.
	 * @return True if the given position is within the given entity's trigger volume.
	 */
	bool pointIsWithinTrigger(CBaseEntity* pEntity, const Vector& vPoint);
	bool isBoundsDefinedInEntitySpace(edict_t* pEntity);
	bool isBoundsDefinedInEntitySpace(ICollideable* collider);
	int FindEntityByClassname(int start, const char* searchname);
	int FindEntityInSphere(int start, const Vector& center, float radius);
	int FindEntityByNetClass(int start, const char* classname);
	int FindEntityByTargetname(int start, const char* targetname);
	int FindNamedEntityByClassname(int start, const char* targetname, const char* classname);
	bool PointWithinViewAngle(const Vector& vecSrcPosition, const Vector& vecTargetPosition, const Vector& vecLookDirection, float flCosHalfFOV);
	float GetForwardViewCone(float angle);
	std::unique_ptr<CStudioHdr> GetEntityModelPtr(edict_t* pEntity);
	/**
	 * @brief Gets the index of a bone from the given model.
	 * @param hdr CStudioHDR pointer of the model you want to get the bone from
	 * @param bonename Name of the bone
	 * @return Bone index or -1 on failure
	*/
	int LookupBone(CStudioHdr* hdr, const char* bonename);

	/**
	 * @brief Retrieves a bone position and angles.
	 * 
	 * This function does NOT validates the bone index.
	 * @param entity Entity to get the bone from (Must derive from CBaseAnimating).
	 * @param bone Bone index.
	 * @param origin Bone position result.
	 * @param angles Bone angle result.
	 * @return Returns false on failure.
	 */
	bool GetBonePosition(CBaseEntity* entity, int bone, Vector& origin, QAngle& angles);
	/**
	 * @brief Retrieves a bone position and angles.
	 * 
	 * This function validates the bone index.
	 * @param entity Entity to get the bone from (Must derive from CBaseAnimating).
	 * @param hdr Entity's CStudioHdr pointer.
	 * @param bone Bone index.
	 * @param origin Bone position result.
	 * @param angles Bone angles result.
	 * @return Returns false on failure.
	 */
	bool GetBonePosition(CBaseEntity* entity, CStudioHdr* hdr, int bone, Vector& origin, QAngle& angles);

	/**
	 * @brief Retrieves a bone position and angles.
	 *
	 * This function validates the bone index.
	 * @param entity Entity to get the bone from (Must derive from CBaseAnimating).
	 * @param hdr Entity's CStudioHdr pointer.
	 * @param name Bone name.
	 * @param origin Bone position result.
	 * @param angles Bone angles result.
	 * @return Returns false on failure.
	 */
	inline bool GetBonePosition(CBaseEntity* entity, CStudioHdr* hdr, const char* name, Vector& origin, QAngle& angles)
	{
		int bone = LookupBone(hdr, name);
		return GetBonePosition(entity, hdr, bone, origin, angles);
	}

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
	 * @param pred Player collection filter, only called to clients that are valid and in game.
	 */
	template <typename T>
	void CollectPlayers(std::vector<int>& playersvector, T pred);
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
	inline void ForEachEdict(T& functor)
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
	 * @tparam T A predicate with 3 parameters (int client, edict_t* entity, SourceMod::IGamePlayer* player)
	 * @param pred Predicate to run on each player
	 * @return Number of players counted.
	 */
	template <typename T>
	inline int CountPlayers(T pred)
	{
		int playercount = 0;

		for (int i = 1; i <= gpGlobals->maxClients; i++)
		{
			auto edict = gamehelpers->EdictOfIndex(i);

			if (!edict || edict->IsFree() || edict->GetIServerEntity() == nullptr || edict->GetUnknown() == nullptr)
				continue;

			auto player = playerhelpers->GetGamePlayer(i);

			if (pred(i, edict, player))
			{
				++playercount;
			}
		}

		return playercount;
	}

	/**
	 * @brief Predicate for counting the number of players on a team.
	 */
	class PredPlayersInTeam
	{
	public:
		PredPlayersInTeam(int team) :
			m_team(team)
		{
		}

		bool operator()(int client, edict_t* entity, SourceMod::IGamePlayer* player);

	private:
		int m_team;
	};

	/**
	 * @brief Runs a function on each player.
	 * @tparam T A class with operator() overload with 3 parameters (int client, edict_t* entity, SourceMod::IGamePlayer* player)
	 * @param functor Function to run on each player.
	 */
	template <typename T>
	inline void ForEachPlayer(T& functor)
	{
		for (int i = 1; i <= gpGlobals->maxClients; i++)
		{
			edict_t* edict = gamehelpers->EdictOfIndex(i);

			if (!edict || edict->IsFree() || edict->GetIServerEntity() == nullptr)
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
	inline void ForEachEntityOfClassname(const char* classname ,T& functor)
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
	inline void ForEachEntityInSphere(const Vector& center, float radius,T& functor)
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
	inline void ForEveryEntity(T& functor)
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

	/**
	 * @brief Runs a function on every edict (networked entity).
	 * @tparam T A lambda or class with operator() overload. bool (int index, edict_t* edict, CBaseEntity* entity).
	 * 
	 * Return false to exit the loop early.
	 * @param functor Functor to run.
	 */
	template <typename T>
	inline void ForEveryEdict(T& functor)
	{
		for (int i = 0; i < gpGlobals->maxEntities; i++)
		{
			edict_t* pEdict = gamehelpers->EdictOfIndex(i);

			if (UtilHelpers::IsValidEdict(pEdict))
			{
				CBaseEntity* pEntity = pEdict->GetIServerEntity()->GetBaseEntity();

				if (!functor(i, pEdict, pEntity))
				{
					return;
				}
			}
		}
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
			return "Half-Life 2: Deathmatch/Source SDK 2013 (2025) Branch";
		case SE_DODS:
			return "Day of Defeat: Source Branch";
		case SE_EPISODEONE:
			return "Episode One/Original Branch";
		case SE_DARKMESSIAH:
			return "Dark Messiah Branch";
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
		case SE_DOI:
			return "Day of Infamy Branch";
		case SE_MCV:
			return "Military Conflict: Vietnam Branch";
		default:
			return "Unknown";
		}
	}

	/**
	 * @brief Entity Enumerator implementation for IPartitionEnumerator
	 */
	class CEntityEnumerator : public IPartitionEnumerator
	{
	public:
		CEntityEnumerator();

		/**
		 * @brief 
		 * @tparam F Filter function bool (IHandleEntity* entity) return true to include the entity
		 * @param filterfunctor 
		 */
		template <typename F>
		inline CEntityEnumerator(F filterfunctor)
		{
			m_filter = filterfunctor;
			m_ents.reserve(256);
		}

		// Inherited via IPartitionEnumerator
		IterationRetval_t EnumElement(IHandleEntity* pHandleEntity) override;
		// Number of entities collected
		inline std::size_t Count() const { return m_ents.size(); }
		const std::vector<CBaseEntity*>& GetEntityVector() { return m_ents; }

		/**
		 * @brief Runs a function on every collected entity
		 * @tparam F bool (CBaseEntity* entity)
		 * @param func function to run. Return false to stop looping
		 */
		template <typename F>
		inline void ForEach(F& func)
		{
			for (CBaseEntity* entity : m_ents)
			{
				if (!func(entity))
				{
					return;
				}
			}
		}

	private:
		std::vector<CBaseEntity*> m_ents;
		std::function<bool(IHandleEntity*)> m_filter;
	};

	/**
	 * @brief Gets all entities in a particular volume... Port of UTIL_EntitiesInBox from game/server/util.h
	 * @param mins Min bounds.
	 * @param maxs Max bounds.
	 * @param enumerator Entity Enumerator.
	 * @param mask Mask to use.
	 * @param coarseTest if coarseTest == true, it'll return all elements that are in spatial partitions that intersect the box.
	 * 
	 * if coarseTest == false, it'll return only elements that truly intersect
	 * @return Number of entities collected.
	 */
	std::size_t EntitiesInBox(const Vector& mins, const Vector& maxs, CEntityEnumerator& enumerator, SpatialPartitionListMask_t mask = static_cast<SpatialPartitionListMask_t>(PARTITION_ENGINE_NON_STATIC_EDICTS), bool coarseTest = false);

	/**
	 * @brief Gets all entities in a sphere.
	 * @param origin Sphere center.
	 * @param radius Sphere radius.
	 * @param enumerator Entity enumerator.
	 * @param mask Mask to use.
	 * @param coarseTest if coarseTest == true, it'll return all elements that are in spatial partitions that intersect the sphere.
	 * @return Number of entities collected.
	 */
	std::size_t EntitiesInSphere(const Vector& origin, float radius, CEntityEnumerator& enumerator, SpatialPartitionListMask_t mask = static_cast<SpatialPartitionListMask_t>(PARTITION_ENGINE_NON_STATIC_EDICTS), bool coarseTest = false);

	/**
	 * @brief A general purpose generic filter interface.
	 * @tparam T Object to filter.
	 */
	template <typename T>
	class IGenericFilter
	{
	public:

		/**
		 * @brief Should this object be selected?
		 * @param object Object being filtered.
		 * @return True if it should be selected, false otherwise.
		 */
		virtual bool IsSelected(T object) = 0;
	};

	/**
	 * @brief Checks via datamaps if the given entity derives from the given class.
	 * @param pEntity Entity to check.
	 * @param classname The name of the class to check. (IE: CBaseCombatCharacter).
	 * @return True if the entity derives from the given class, false if not.
	 */
	bool EntityDerivesFrom(CBaseEntity* pEntity, const char* classname);
}

template<typename T>
inline void UtilHelpers::CollectPlayers(std::vector<int>& playersvector, T pred)
{
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		edict_t* edict = gamehelpers->EdictOfIndex(i);

		if (!edict || !UtilHelpers::IsValidEdict(edict))
			continue;

		SourceMod::IGamePlayer* gp = playerhelpers->GetGamePlayer(i);

		if (!gp)
			continue;

		if (!gp->IsInGame())
			continue;

		if (pred(i, edict, gp))
		{
			playersvector.push_back(i);
		}
	}
}

namespace UtilHelpers::math
{
	/**
	 * @brief Builds a normalized direction vector that points towards the 'to' vector.
	 * 
	 * Addition moves closer to 'to', subtraction moves away from 'to'.
	 * @param from From position
	 * @param to To position
	 * @return Normalized direction vector.
	 */
	inline Vector BuildDirectionVector(const Vector& from, const Vector& to)
	{
		Vector dir = (to - from);
		dir.NormalizeInPlace();
		return dir;
	}

	/**
	 * @brief Given an entity, calculates the nearest point to the entity's AABB
	 * @param entity Entity to get the nearest point to.
	 * @param vecWorldPt World position to search the nearest point from.
	 * @param VecNearestWorldPt World position of the entity's AABB nearest of vecWorldPt
	 */
	void CalcClosestPointOfEntity(CBaseEntity* entity, const Vector& vecWorldPt, Vector& VecNearestWorldPt);
}

namespace UtilHelpers::parsers
{
	/**
	 * @brief Utility function for parsing random values from configuration files.
	 * 
	 * Format "MIN:MAX". Example: "25:50", a random number between 25 and 50
	 * @param str String to parse.
	 * @param out Variable to store the random output.
	 * @param min Minimum value allowed.
	 * @param max Maximum value allowed.
	 * @return True if parsed successfully, false on error.
	 */
	bool ParseRandomInt(const char* str, int& out, const int min, const int max);
}

namespace UtilHelpers::textformat
{
	// Formats the given vector into a string and returns the string
	const char* FormatVector(const Vector& vec);
	// Formats the given angles into a string and returns the string
	const char* FormatAngles(const QAngle& angles);
	/**
	 * @brief Formats the given boolean into a string and returns the string.
	 * @param value Bool to format.
	 * @param yesno If true. Use "Yes" to represent true and "No" for false.
	 * @return String.
	 */
	const char* FormatBool(const bool value, const bool yesno = true);
	/**
	 * @brief Formats a string with variable arguments.
	 * @param fmt String to format.
	 * @param  Format args.
	 * @return Pointer to string.
	 */
	const char* FormatVarArgs(const char* fmt, ...);
	// Formats the given entity into a string and returns the string. Accepts NULL entities.
	const char* FormatEntity(CBaseEntity* entity);
}

namespace UtilHelpers::players
{
	/**
	 * @brief Gets a random teammate of the given player.
	 * @param me Player entity ptr.
	 * @param alive If true, only search for alive teammates.
	 * @param onlyhumans If true, skip bots.
	 * @return Teammate entity ptr or NULL if none found.
	 */
	CBaseEntity* GetRandomTeammate(CBaseEntity* me, const bool alive = false, const bool onlyhumans = false);
}

/**
 * @brief Studio/Models related helper functions
 */
namespace UtilHelpers::studio
{
	constexpr int INVALID_BONE_INDEX = -1;

	/**
	 * @brief Looks up a bone of the given entity.
	 * @param pEntity Entity to get the bone from.
	 * @param bone Bone name to search.
	 * @return Bone index or INVALID_BONE_INDEX (-1) if not found.
	 */
	int LookupBone(CBaseEntity* pEntity, const char* bone);

	bool GetBonePosition(CBaseEntity* pEntity, int bone, Vector& position, QAngle& angles);
}

/**
 * @brief Utility functions for compatibility between source engine branches.
 */
namespace UtilHelpers::sdkcompat
{
	/**
	 * @brief Saves the given keyvalues to a file.
	 * @param kvRoot Keyvalues to save.
	 * @param filename File name.
	 * @param pathid Filesystem path id. (IE: "MOD").
	 * @param sortKeys Sort keys?
	 * @param allowEmptyString Allow empty strings?
	 * @return True if the file was saved.
	 */
	bool SaveKeyValuesToFile(KeyValues* kvRoot, const char* filename, const char* pathid, const bool sortKeys = false, const bool allowEmptyString = false);

	int GetTypeDescOffset(typedescription_t* td);
}

/**
 * @brief Utility functions related to datamaps.
 */
namespace UtilHelpers::datamap
{
	/**
	 * @brief Retrieves an entity input function.
	 * @param entity Entity to get the input function from.
	 * @param inputName Input field name. See a SourceMod datamap dump for a list of name.
	 * @return Input function if found or NULL if not found.
	 */
	inputfunc_t GetEntityInputFunc(CBaseEntity* entity, const char* inputName);
	// Converts a datadesc of an entity output into an entity output
	CBaseEntityOutput* GetEntityOutputFromDataDesc(CBaseEntity* entity, typedescription_t* dataDesc);
	/**
	 * @brief Retrieves an entity output.
	 * @param entity Entity to get the output from.
	 * @param outputName Output field name. See a SourceMod datamap dump for a list of name.
	 * @return Entity output or NULL if not found.
	 */
	CBaseEntityOutput* GetEntityOutput(CBaseEntity* entity, const char* outputName);

	void DumpEntityDatamap(CBaseEntity* entity);
}

/**
 * @brief Entity input/output helper functions.
 */
namespace UtilHelpers::io
{
	// Removes the given entity from the game (if supported).
	void RemoveEntity(CBaseEntity* entity);
	// Fire an input on the entity. Raw method.
	void FireInputRaw(CBaseEntity* entity, inputfunc_t func, inputdata_t data);
	/**
	 * @brief Fires an input to the entity.
	 * @param entity Entity to fire the input at.
	 * @param inputName Input name. This function uses Sourcemod to look up the input function and input names must have a "Input" prefix. IE: "Kill" becomes "InputKill"
	 * @param pActivator Input activator.
	 * @param pCaller Input Caller.
	 * @param variant Variant containing input data. (The data needed depends on the input being called).
	 * @param outputID Output ID, depends on the input being used.
	 * @return True if the input was fired, false on failure.
	 */
	bool FireInput(CBaseEntity* entity, const char* inputName, CBaseEntity* pActivator, CBaseEntity* pCaller, variant_t variant, int outputID = 0);
	/**
	 * @brief Checks if the given entity has any Output targetting the given target name.
	 * 
	 * Notes: Stops on the first match, entity may have multiple outputs targetting the same entity.
	 * @param entity Entity to search.
	 * @param targetname Name of the target entity.
	 * @param inputName Optional string to store the input targetting the entity.
	 * @return True if the entity targets the given targetname, false otherwise.
	 */
	bool IsConnectedTo(CBaseEntity* entity, const char* targetname, std::string* inputName = nullptr);
}

#endif // !UTIL_HELPERS_H_



