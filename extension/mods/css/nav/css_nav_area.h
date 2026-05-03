#ifndef __NAVBOT_MOD_CSS_NAV_AREA_H_
#define __NAVBOT_MOD_CSS_NAV_AREA_H_

#include <navmesh/nav_area.h>

class CCSSNavArea : public CNavArea
{
public:
	CCSSNavArea(unsigned int place);

	enum CSSAttributes
	{
		CSSATTRIB_NONE = 0,
		CSSATTRIB_NO_HOSTAGES = 0x1, // this area is blocked for bots escorting hostages
		CSSATTRIB_NO_T = 0x2, // this area is blocked for terrorists bots
		CSSATTRIB_NO_CT = 0x4, // this area is blocked for counter-terrorist bots
	};

	static CSSAttributes NameToCSSAttrib(const char* name);

	void Save(std::fstream& filestream, uint32_t version) override;
	NavErrorType Load(std::fstream& filestream, uint32_t version, uint32_t subVersion) override;
	bool IsBlocked(int teamID, bool ignoreNavBlockers = false) const override;

	IMPLEMENT_ATTRIBUTE_BIT_FUNCS(m_cssattributes, CSSAttributes, CS);

	void DrawCSSAttributes() const;

private:
	int m_cssattributes;
};

#endif // !__NAVBOT_MOD_CSS_NAV_AREA_H_
