#ifndef __NAVBOT_DODS_NAV_TEAM_WALL_BLOCKER_H_
#define __NAVBOT_DODS_NAV_TEAM_WALL_BLOCKER_H_

#include <navmesh/nav_blocker.h>

class CDoDSNavTeamWallBlocker final : public CNavBlocker<CNavArea>
{
public:
	CDoDSNavTeamWallBlocker(CBaseEntity* entity);

	bool IsValid() const final
	{
		if (m_entity.Get() == nullptr)
		{
			return false;
		}
		
		return true;
	}
	void Update() final;
	bool IsBlocked(int teamID) const final { return teamID == m_iTeamNum; }
	bool RemoveOnRecompute() const final { return true; }
	const char* GetName() const final { return "Team Wall Blocker"; }
	void PrintDebugInfo() const final;

private:
	bool m_isNewBlocker; // true if the wall entity is a func_teamblocker, else it's a func_team_wall
	int m_iTeamNum;
	CHandle<CBaseEntity> m_entity;

	void UpdateTeam();
	void FindAreas();
	void Invalidate() { m_entity.Term(); }
};

#endif // !__NAVBOT_DODS_NAV_TEAM_WALL_BLOCKER_H_
