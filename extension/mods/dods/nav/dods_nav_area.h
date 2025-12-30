#ifndef __NAVBOT_DODS_NAV_AREA_H_
#define __NAVBOT_DODS_NAV_AREA_H_

#include <sdkports/sdk_ehandle.h>
#include <navmesh/nav_area.h>
#include <cstdint>

class CDoDSNavArea : public CNavArea
{
public:
	CDoDSNavArea(unsigned int place);
	~CDoDSNavArea() override;

	enum DoDNavAttributes
	{
		DODNAV_NONE = 0,
		DODNAV_NO_ALLIES = 0x1, // always blocked to the allies team
		DODNAV_NO_AXIS = 0x2, // always blocked to the axis team
		DODNAV_DEPRECATED1 = 0x4, // nav area is blocked until the nearest bomb target is bombed
		DODNAV_DEPRECATED2 = 0x8, // nav area is blocked if the bot doesn't have a bomb on their inventory
		DODNAV_BOMBS_TO_OPEN = 0x10, // path is blocked by a obstacle that can be bombed
		DODNAV_REQUIRES_PRONE = 0x20, // the bot must go prone to pass on this area.
	};

	void OnUpdate() override;
	void OnRoundRestart(void) override;
	void Save(std::fstream& filestream, uint32_t version) override;
	NavErrorType Load(std::fstream& filestream, uint32_t version, uint32_t subVersion) override;
	NavErrorType PostLoad(void) override;
	bool IsBlocked(int teamID, bool ignoreNavBlockers = false) const override;
	// Returns true if the nav area has bomb related attributes assigned to it
	inline bool HasBombRelatedAttributes() const
	{
		if ((m_dodAttributes & DODNAV_BOMBS_TO_OPEN) != 0)
		{
			return true;
		}

		return false;
	}
	// Returns the assigned bomb target entity to this nav area. May be NULL if the bomb target entity was deleted.
	CBaseEntity* GetAssignedBombTarget() const { return m_bombTarget.Get(); }
	// Returns true if the nav area was bombed and is now open.
	const bool WasBombed() const { return m_bombed; }
	// Returns true if it's possible to plant a bomb
	const bool CanPlantBomb() const;
	// Returns true if the given team index is allowed to plant a bomb here.
	const bool CanMyTeamPlantBomb(const int team) const { return m_bombTeam == NAV_TEAM_ANY || m_bombTeam == team; }

	void SetDoDAttributes(DoDNavAttributes attribute)
	{
		m_dodAttributes |= attribute;
	}

	void ClearDoDAttributes(DoDNavAttributes attribute)
	{
		m_dodAttributes &= ~attribute;
	}

	bool HasDoDAttributes(DoDNavAttributes attributes) const
	{
		return (m_dodAttributes & attributes) != 0;
	}

	inline void WipeDoDAttributes()
	{
		m_dodAttributes = 0;
	}

	void ShowAreaInfo() const override;

private:
	int m_dodAttributes;
	CHandle<CBaseEntity> m_bombTarget; // bomb target nearest of this nav area
	bool m_bombed; // true if the bomb target is bombed
	int m_bombTeam; // team the bomb target is assigned to

	void FindAndAssignNearestBombTarget();
};

#endif // !__NAVBOT_DODS_NAV_AREA_H_
