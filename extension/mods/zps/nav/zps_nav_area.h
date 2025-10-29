#ifndef __NAVBOT_ZPS_NAV_AREA_H_
#define __NAVBOT_ZPS_NAV_AREA_H_

#include <navmesh/nav_area.h>

class CZPSNavArea : public CNavArea
{
public:
	CZPSNavArea(unsigned int place);

	enum ZPSAttributes
	{
		ZPS_ATTRIBUTE_INVALID = 0, // invalid attribute value
		ZPS_ATTRIBUTE_NOZOMBIES = 0x0001, // this area is blocked to zombies
		ZPS_ATTRIBUTE_NOHUMANS = 0x0002, // this area is blocked to survivors
		ZPS_ATTRIBUTE_TRANSIENT_SURVIVORSONLY = 0x0004, // this area is a transient area for survivors only
		ZPS_ATTRIBUTE_TRANSIENT_PHYSPROPS = 0x0008, // this area is a transient area blocked by prop_physics
	};

	// Converts the attribute name to it's value, returns ZPS_ATTRIBUTE_INVALID on failure
	static ZPSAttributes GetZPSAttributeFromName(const char* name);

	void Save(std::fstream& filestream, uint32_t version) override;
	NavErrorType Load(std::fstream& filestream, uint32_t version, uint32_t subVersion) override;
	void OnUpdate() override;
	void OnRoundRestart(void) override;
	bool IsBlocked(int teamID, bool ignoreNavBlockers = false) const override;
	void UpdateBlocked(bool force = false, int teamID = NAV_TEAM_ANY) override;
	void SetZPSAttributes(ZPSAttributes attribs) { m_zpsattributes |= attribs; }
	void ClearZPSAttributes(ZPSAttributes attribs) { m_zpsattributes &= ~attribs; }
	bool HasZPSAttributes(ZPSAttributes attribs) const { return (m_zpsattributes & attribs) != 0; }
	void WipeZPSAttributes() { m_zpsattributes = 0; }

private:
	int m_zpsattributes;
	bool m_blockedByPhysProps;

	bool UpdateZPSPhysPropsTransientStatus();
};


#endif // !__NAVBOT_ZPS_NAV_AREA_H_
