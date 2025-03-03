#ifndef NAVBOT_SDKPORTS_TRACES_H_
#define NAVBOT_SDKPORTS_TRACES_H_

#include <vector>
#include <functional>
#include <eiface.h>
#include <IEngineTrace.h>

class CBaseEntity;
struct edict_t;

namespace trace
{

	/**
	 * @brief Extended trace filter interface that provides a custom should hit function with entity index, BE and Edict pointers.
	 */
	class IExtendedTraceFilter : public ITraceFilter
	{
	public:
		/**
		 * @brief Should the trace collide with this entity?
		 * @param entity Entity index
		 * @param pEntity CBaseEntity pointer
		 * @param pEdict edict_t pointer
		 * @param contentsMask Mask
		 * @return true if the trace should collide, false otherwise
		 * @note The entity will be NULL if colliding with a static prop!
		 */
		virtual bool ShouldHitEntity(int entity, CBaseEntity* pEntity, edict_t* pEdict, const int contentsMask) = 0;
	};

	class CBaseTraceFilter : public IExtendedTraceFilter
	{
	public:
		bool ShouldHitEntity(int entity, CBaseEntity* pEntity, edict_t* pEdict, const int contentsMask) override
		{
			return true;
		}

		TraceType_t	GetTraceType() const override
		{
			return TRACE_EVERYTHING;
		}

	private:
		bool ShouldHitEntity(IHandleEntity* pEntity, int contentsMask) override final;
	};

	class CTraceFilterSimple : public CBaseTraceFilter
	{
	public:
		CTraceFilterSimple()
		{
			m_passEntity = nullptr;
			m_collisiongroup = 0;
			m_extraHitFunc = nullptr;
		}

		CTraceFilterSimple(CBaseEntity* pass, int collisiongroup)
		{
			m_passEntity = pass;
			m_collisiongroup = collisiongroup;
			m_extraHitFunc = nullptr;
		}

		CTraceFilterSimple(int collisiongroup, CBaseEntity* pass)
		{
			m_passEntity = pass;
			m_collisiongroup = collisiongroup;
			m_extraHitFunc = nullptr;
		}

		template <typename F>
		CTraceFilterSimple(CBaseEntity* pass, int collisiongroup, F extrafunc)
		{
			m_passEntity = pass;
			m_collisiongroup = collisiongroup;
			m_extraHitFunc = extrafunc;
		}

		bool ShouldHitEntity(int entity, CBaseEntity* pEntity, edict_t* pEdict, const int contentsMask) override;

		void SetCollisionGroup(int group) { m_collisiongroup = group; }
		void SetPassEntity(CBaseEntity* ent) { m_passEntity = ent; }

		template <typename F>
		void SetExtraFunc(F func)
		{
			m_extraHitFunc = func;
		}

		void ClearExtraFunc() { m_extraHitFunc = nullptr; }

	private:
		CBaseEntity* m_passEntity;
		std::function<bool(int, CBaseEntity*, edict_t*, const int)> m_extraHitFunc;
		int m_collisiongroup;
	};

	class CTraceFilterPlayersOnly : public CTraceFilterSimple
	{
	public:
		CTraceFilterPlayersOnly(CBaseEntity* passent = nullptr) :
			CTraceFilterSimple(COLLISION_GROUP_NONE, passent)
		{
			m_team = -1;
		}

		CTraceFilterPlayersOnly(int team, CBaseEntity* passent) :
			CTraceFilterSimple(COLLISION_GROUP_NONE, passent)
		{
			m_team = team;
		}

		bool ShouldHitEntity(int entity, CBaseEntity* pEntity, edict_t* pEdict, const int contentsMask) override;

		TraceType_t	GetTraceType() const override
		{
			return TRACE_ENTITIES_ONLY;
		}

	private:
		int m_team;
	};

	class CTraceFilterNoNPCsOrPlayers : public CTraceFilterSimple
	{
	public:
		CTraceFilterNoNPCsOrPlayers(CBaseEntity* ignore = nullptr, int collisiongroup = COLLISION_GROUP_NONE) :
			CTraceFilterSimple(collisiongroup, ignore)
		{
		}

		bool ShouldHitEntity(int entity, CBaseEntity* pEntity, edict_t* pEdict, const int contentsMask) override;

	private:
	};

	/**
	 * @brief Trace filter that supports ignoring multiple entities stored on an std::vector
	 */
	class CTraceFilterIgnoreVector : public CTraceFilterSimple
	{
	public:
		CTraceFilterIgnoreVector() :
			CTraceFilterSimple(nullptr, COLLISION_GROUP_NONE)
		{
		}

		CTraceFilterIgnoreVector(std::vector<CBaseEntity*>&& ignoreVec) :
			CTraceFilterSimple(nullptr, COLLISION_GROUP_NONE)
		{
			m_ignoreList = ignoreVec;
		}

		void AddEntityToIgnore(CBaseEntity* ignore)
		{
			for (auto ent : m_ignoreList)
			{
				if (ent == ignore)
				{
					return;
				}
			}

			m_ignoreList.push_back(ignore);
		}

	private:
		std::vector<CBaseEntity*> m_ignoreList;
	};

	inline void line(const Vector& start, const Vector& end, unsigned int mask, trace_t& result)
	{
		Ray_t ray;
		ray.Init(start, end);
		CTraceFilterSimple filter;
		enginetrace->TraceRay(ray, mask, &filter, &result);
	}

	inline void line(const Vector& start, const Vector& end, unsigned int mask, CBaseEntity* ignore, int collisiongroup, trace_t& result)
	{
		Ray_t ray;
		ray.Init(start, end);
		CTraceFilterSimple filter(collisiongroup, ignore);
		enginetrace->TraceRay(ray, mask, &filter, &result);
	}

	inline void line(const Vector& start, const Vector& end, unsigned int mask, ITraceFilter* filter, trace_t& result)
	{
		Ray_t ray;
		ray.Init(start, end);
		enginetrace->TraceRay(ray, mask, filter, &result);
	}

	inline void hull(const Vector& start, const Vector& end, const Vector& mins, const Vector& maxs, unsigned int mask, CBaseEntity* ignore, int collisiongroup, trace_t& result)
	{
		Ray_t ray;
		ray.Init(start, end, mins, maxs);
		CTraceFilterSimple filter(collisiongroup, ignore);
		enginetrace->TraceRay(ray, mask, &filter, &result);
	}

	inline void hull(const Vector& start, const Vector& end, const Vector& mins, const Vector& maxs, unsigned int mask, ITraceFilter* filter, trace_t& result)
	{
		Ray_t ray;
		ray.Init(start, end, mins, maxs);
		enginetrace->TraceRay(ray, mask, filter, &result);
	}

	inline void ray(const Ray_t& ray, unsigned int mask, ITraceFilter* filter, trace_t& result)
	{
		enginetrace->TraceRay(ray, mask, filter, &result);
	}

	inline int pointcontents(const Vector& point)
	{
		return enginetrace->GetPointContents(point);
	}

	/**
	 * @brief Checks if a specific point is within this entity's zone
	 * @param collideable Entity collideable
	 * @param point Point to check
	 * @none See: https://github.com/ValveSoftware/source-sdk-2013/blob/0d8dceea4310fde5706b3ce1c70609d72a38efdf/mp/src/game/server/triggers.cpp#L310-L318
	 * @hint Can be used to check if a point is inside a brush entity.
	 * @return true if the given point is inside the entity's collision, false otherwise
	 */
	inline bool pointwithin(ICollideable* collideable, const Vector& point)
	{
		Ray_t ray;
		ray.Init(point, point);
		trace_t result;
		enginetrace->ClipRayToCollideable(ray, MASK_ALL, collideable, &result);
		return result.startsolid;
	}

	inline bool pointwithin(edict_t* edict, const Vector& point)
	{
		ICollideable* collideable = edict->GetCollideable();

		if (collideable == nullptr)
		{
			return false;
		}

		Ray_t ray;
		ray.Init(point, point);
		trace_t result;
		enginetrace->ClipRayToCollideable(ray, MASK_ALL, collideable, &result);
		return result.startsolid;
	}

	inline bool pointwithin(CBaseEntity* entity, const Vector& point)
	{
		ICollideable* collideable = reinterpret_cast<IServerEntity*>(entity)->GetCollideable();

		if (collideable == nullptr)
		{
			return false;
		}

		Ray_t ray;
		ray.Init(point, point);
		trace_t result;
		enginetrace->ClipRayToCollideable(ray, MASK_ALL, collideable, &result);
		return result.startsolid;
	}

	inline bool pointwithin(IHandleEntity* entity, const Vector& point)
	{
		Ray_t ray;
		ray.Init(point, point);
		trace_t result;
		enginetrace->ClipRayToEntity(ray, MASK_ALL, entity, &result);
		return result.startsolid;
	}

	inline void pointwithin(ICollideable* collideable, const Vector& point, unsigned int mask, trace_t& result)
	{
		Ray_t ray;
		ray.Init(point, point);
		enginetrace->ClipRayToCollideable(ray, mask, collideable, &result);
	}

	inline void pointwithin(IHandleEntity* entity, const Vector& point, unsigned int mask, trace_t& result)
	{
		Ray_t ray;
		ray.Init(point, point);
		enginetrace->ClipRayToEntity(ray, mask, entity, &result);
	}

	inline bool pointoutisdeworld(const Vector& point)
	{
		return enginetrace->PointOutsideWorld(point);
	}

	inline ICollideable* entitytocollideable(IHandleEntity* entity)
	{
		return enginetrace->GetCollideable(entity);
	}

	// Given a point, trace downwards to find a solid ground (line)
	inline Vector getground(const Vector& point)
	{
		Vector end = point;
		end.z -= 16384.0f;
		CTraceFilterWorldAndPropsOnly filter;
		Ray_t ray;
		ray.Init(point, end);
		trace_t tr;
		enginetrace->TraceRay(ray, MASK_SOLID, &filter, &tr);
		return tr.endpos;
	}

	// Given a point, trace downwards to find a solid ground (hull)
	inline Vector getground(const Vector& point, const Vector& mins, const Vector& maxs)
	{
		Vector end = point;
		end.z -= 16384.0f;
		CTraceFilterWorldAndPropsOnly filter;
		Ray_t ray;
		ray.Init(point, end, mins, maxs);
		trace_t tr;
		enginetrace->TraceRay(ray, MASK_SOLID, &filter, &tr);
		return tr.endpos;
	}

	/**
	 * @brief Gets the ground surface normal.
	 * @param point Point to trace downwards to get the ground from.
	 * @param normal Vector to store the normal result at.
	 * @return true if the trace hit something (ground was found). false otherwise.
	 */
	inline bool getgroundnormal(const Vector& point, Vector& normal)
	{
		Vector end = point;
		end.z -= 16384.0f;
		CTraceFilterWorldAndPropsOnly filter;
		Ray_t ray;
		ray.Init(point, end);
		trace_t tr;
		enginetrace->TraceRay(ray, MASK_SOLID, &filter, &tr);
		normal = tr.plane.normal;
		return tr.fraction < 1.0f;
	}
}

#endif // !NAVBOT_SDKPORTS_TRACES_H_


