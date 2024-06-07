#ifndef NAVBOT_SDKPORTS_TRACES_H_
#define NAVBOT_SDKPORTS_TRACES_H_

#include <functional>
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

	constexpr auto WALK_THRU_PROP_DOORS = 0x01;
	constexpr auto WALK_THRU_FUNC_DOORS = 0x02;
	constexpr auto WALK_THRU_DOORS = (WALK_THRU_PROP_DOORS | WALK_THRU_FUNC_DOORS);
	constexpr auto WALK_THRU_BREAKABLES = 0x04;
	constexpr auto WALK_THRU_TOGGLE_BRUSHES = 0x08;
	constexpr auto WALK_THRU_EVERYTHING = (WALK_THRU_DOORS | WALK_THRU_BREAKABLES | WALK_THRU_TOGGLE_BRUSHES);

	class CTraceFilterWalkableEntities : public CTraceFilterNoNPCsOrPlayers
	{
	public:
		CTraceFilterWalkableEntities(CBaseEntity* passEnt, int collisiongroup, unsigned int flags) :
			CTraceFilterNoNPCsOrPlayers(passEnt, collisiongroup)
		{
			m_flags = flags;
		}

		bool ShouldHitEntity(int entity, CBaseEntity* pEntity, edict_t* pEdict, const int contentsMask) override;

	private:
		unsigned int m_flags;
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

	bool IsEntityWalkable(CBaseEntity* pEntity, unsigned int flags);
}

#endif // !NAVBOT_SDKPORTS_TRACES_H_


