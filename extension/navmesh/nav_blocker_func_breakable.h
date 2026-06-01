#ifndef __NAVBOT_NAV_MESH_BLOCKER_FUNC_BREAKABLE_H_
#define __NAVBOT_NAV_MESH_BLOCKER_FUNC_BREAKABLE_H_

#include "nav_blocker.h"

class CFuncBreakableNavBlocker : public CNavBlocker<CNavArea>
{
public:
	CFuncBreakableNavBlocker()
	{
		m_teamNum = NAV_TEAM_ANY;
		m_blocked = false;
		m_negated = false;
		m_teamOnly = false;
		m_type = BreakableType::TYPE_NONE;
	}

	static constexpr int SF_BREAK_ONLY_ON_TRIGGER = 0x1;

	enum BreakableType
	{
		TYPE_NONE = 0,
		TYPE_TRIGGER_ONLY, // trigger only spawnflag is set
		TYPE_FILTER, // has a damage filter
		TYPE_BIGHEALTH, // health is over the break limit
	};

	bool IsValid() override;
	void Update() override;
	bool IsBlocked(int teamID) override;
	bool RemoveOnRecompute() override { return true; }
	const char* GetName() override { return "CFuncBreakableNavBlocker"; }
	void PrintDebugInfo() override;

	virtual void Init(CBaseEntity* breakable);

protected:
	int m_teamNum; // team restriction
	bool m_blocked; // status cache
	bool m_negated; // filter logic is inverted
	bool m_teamOnly; // breakable can only be damaged by entities from a specific team
	CHandle<CBaseEntity> m_breakable;
	BreakableType m_type;

	/**
	 * @brief Called to check if the breakable's damage filter is blocking.
	 * @param filter The breakable's damage filter entity.
	 * @return Returing true causes the breakable type to be set to FILTER, false to ignore.
	 */
	virtual bool CheckFilter(CBaseEntity* filter);
};

#endif // !__NAVBOT_NAV_MESH_BLOCKER_FUNC_BREAKABLE_H_
