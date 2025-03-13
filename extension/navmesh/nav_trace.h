#ifndef NAV_TRACE_H_
#define NAV_TRACE_H_

#include <sdkports/sdk_traces.h>

constexpr auto WALK_THRU_PROP_DOORS = 0x01;
constexpr auto WALK_THRU_FUNC_DOORS = 0x02;
constexpr auto WALK_THRU_DOORS = (WALK_THRU_PROP_DOORS | WALK_THRU_FUNC_DOORS);
constexpr auto WALK_THRU_BREAKABLES = 0x04;
constexpr auto WALK_THRU_TOGGLE_BRUSHES = 0x08;
constexpr auto WALK_THRU_EVERYTHING = (WALK_THRU_DOORS | WALK_THRU_BREAKABLES | WALK_THRU_TOGGLE_BRUSHES);

class CTraceFilterWalkableEntities : public trace::CTraceFilterNoNPCsOrPlayers
{
public:
	CTraceFilterWalkableEntities(CBaseEntity* passEnt, int collisiongroup, unsigned int flags) :
		trace::CTraceFilterNoNPCsOrPlayers(passEnt, collisiongroup)
	{
		m_flags = flags;
	}

	bool ShouldHitEntity(IHandleEntity* pHandleEntity, int contentsMask) override;

private:
	unsigned int m_flags;
};

class CTraceFilterTransientAreas : public trace::CTraceFilterNoNPCsOrPlayers
{
public:
	CTraceFilterTransientAreas(CBaseEntity* passEnt, int collgroup) :
		trace::CTraceFilterNoNPCsOrPlayers(passEnt, collgroup)
	{
	}

	bool ShouldHitEntity(IHandleEntity* pHandleEntity, int contentsMask) override;
};

#endif // !NAV_TRACE_H_
