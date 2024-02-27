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
	int GetNumberofPlayersOnTeam(const int team, const bool ignore_dead = false, const bool ignore_bots = false);
	std::optional<int> GetTeamManagerEntity(const int team, const char* classname);
	std::optional<int> GetOwnerEntity(const int entity);
	void CalcClosestPointOnAABB(const Vector& mins, const Vector& maxs, const Vector& point, Vector& closestOut);
	Vector GetGroundPositionFromCenter(edict_t* pEntity);
	
	/**
	 * @brief Collects player into a vector of ints containing their indexes
	 * @tparam T A class with an operator() overload with two parameters (int client, edict_t* entity), return true to add the client to the vector
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

			if (!edict || edict->IsFree() || edict->GetIServerEntity() == nullptr)
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

			if (!edict || edict->IsFree() || edict->GetIServerEntity() == nullptr)
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

			if (!edict || edict->IsFree() || edict->GetIServerEntity() == nullptr)
				continue;

			SourceMod::IGamePlayer* player = playerhelpers->GetGamePlayer(i);

			if (!player)
				continue;

			functor(i, edict, player);
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

		auto gp = playerhelpers->GetGamePlayer(i);

		if (!gp)
			continue;

		if (!gp->IsInGame())
			continue;

		if (functor(i, edict))
		{
			playersvector.push_back(i);
		}
	}
}

#endif // !UTIL_HELPERS_H_



