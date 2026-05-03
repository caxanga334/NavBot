#include NAVBOT_PCH_FILE
#include "../css_shareddefs.h"
#include "css_nav_mesh.h"
#include "css_nav_area.h"

CCSSNavArea::CCSSNavArea(unsigned int place) :
	CNavArea(place)
{
	m_cssattributes = 0;
}

void CCSSNavArea::Save(std::fstream& filestream, uint32_t version)
{
	CNavArea::Save(filestream, version);

	filestream.write(reinterpret_cast<char*>(&m_cssattributes), sizeof(int));
}

NavErrorType CCSSNavArea::Load(std::fstream& filestream, uint32_t version, uint32_t subVersion)
{
	NavErrorType base = CNavArea::Load(filestream, version, subVersion);

	if (base != NAV_OK)
	{
		return base;
	}

	// sub version 1: added cs attributes to nav areas
	if (subVersion >= 1)
	{
		filestream.read(reinterpret_cast<char*>(&m_cssattributes), sizeof(int));

		if (filestream.bad())
		{
			return NAV_CORRUPT_DATA;
		}
	}

	return NAV_OK;
}

bool CCSSNavArea::IsBlocked(int teamID, bool ignoreNavBlockers) const
{
	switch (static_cast<counterstrikesource::CSSTeam>(teamID))
	{
	case counterstrikesource::CSSTeam::TERRORIST:
		if (HasCSAttributes(CSSATTRIB_NO_T))
		{
			return true;
		}

		break;
	case counterstrikesource::CSSTeam::COUNTERTERRORIST:
		if (HasCSAttributes(CSSATTRIB_NO_CT))
		{
			return true;
		}

		break;
	default:
		break;
	}

	return CNavArea::IsBlocked(teamID, ignoreNavBlockers);
}

void CCSSNavArea::DrawCSSAttributes() const
{
	if (m_cssattributes == 0) { return; }

	constexpr auto MAX_DRAW_DIST = 1024.0f * 1024.0f;
	const float dist = (GetCenter() - extmanager->GetListenServerHost()->GetAbsOrigin()).LengthSqr();

	if (dist >= MAX_DRAW_DIST) { return; }

	int r = 0;
	int g = 0;
	int b = 0;
	char text[256]{};
	text[0] = 0;

	if (HasCSAttributes(CSSATTRIB_NO_HOSTAGES))
	{
		ke::SafeStrcat(text, sizeof(text), "NO_HOSTAGES ");
		r = 255;
		g = 255;
	}

	if (HasCSAttributes(CSSATTRIB_NO_T))
	{
		ke::SafeStrcat(text, sizeof(text), "NO_TERRORISTS ");
		r += 255;
	}

	if (HasCSAttributes(CSSATTRIB_NO_CT))
	{
		ke::SafeStrcat(text, sizeof(text), "NO_COUNTERTERRORISTS ");
		b += 255;
	}

	r = std::clamp(r, 0, 255);
	g = std::clamp(g, 0, 255);
	b = std::clamp(b, 0, 255);

	DrawFilled(r, g, b, 160);
	NDebugOverlay::Text(GetCenter(), text, true, NDEBUG_PERSIST_FOR_ONE_TICK);
}
