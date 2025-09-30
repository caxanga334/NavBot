#ifndef __NAVBOT_DODS_NAV_AREA_H_
#define __NAVBOT_DODS_NAV_AREA_H_

#include <sdkports/sdk_ehandle.h>
#include <navmesh/nav_area.h>
#include <cstdint>

class CDODSNavArea : public CNavArea
{
public:
	CDODSNavArea(unsigned int place);
	~CDODSNavArea() override;

	enum DoDNavAttributes
	{
		DODNAV_NONE = 0,
		DODNAV_NO_ALLIES = (1 << 0), // always blocked to the allies team
		DODNAV_NO_AXIS = (1 << 1), // always blocked to the axis team
		DODNAV_BLOCKED_UNTIL_BOMBED = (1 << 2), // nav area is blocked until the nearest bomb target is bombed
		DODNAV_BLOCKED_WITHOUT_BOMBS = (1 << 3), // nav area is blocked if the bot doesn't have a bomb on their inventory
		DODNAV_PLANT_BOMB = (1 << 4), // the bot will stop and plant a bomb if the target hasn't been bombed yet
		DODNAV_REQUIRES_PRONE = (1 << 5), // the bot must go prone to pass on this area.
	};

	void OnRoundRestart(void) override;
	void Save(std::fstream& filestream, uint32_t version) override;
	NavErrorType Load(std::fstream& filestream, uint32_t version, uint32_t subVersion) override;
	NavErrorType PostLoad(void) override;
	bool IsBlocked(int teamID, bool ignoreNavBlockers = false) const override;
	// Returns true if the nav area has bomb related attributes assigned to it
	inline bool HasBombRelatedAttributes() const
	{
		if ((m_dodAttributes & (DoDNavAttributes::DODNAV_BLOCKED_UNTIL_BOMBED | DoDNavAttributes::DODNAV_BLOCKED_WITHOUT_BOMBS | DoDNavAttributes::DODNAV_PLANT_BOMB)) != 0)
		{
			return true;
		}

		return false;
	}
	// Returns the assigned bomb target entity to this nav area. May be NULL if the bomb target entity was deleted.
	CBaseEntity* GetAssignedBombTarget() const { return m_bombTarget.Get(); }
	// Returns true if the nav area was bombed and is now open.
	const bool WasBombed() const;
	// Returns true if it's possible to plant a bomb
	const bool CanPlantBomb() const;

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

	void FindAndAssignNearestBombTarget();
};

#endif // !__NAVBOT_DODS_NAV_AREA_H_
