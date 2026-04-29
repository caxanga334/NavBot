#include NAVBOT_PCH_FILE
#include <mods/css/css_mod.h>
#include "cssbot.h"

#ifdef EXT_VPROF_ENABLED
#include <tier0/vprof.h>
#endif // EXT_VPROF_ENABLED

CCSSBot::CCSSBot(edict_t* edict) :
	CBaseBot(edict)
{
	m_cssbehavior = std::make_unique<CCSSBotBehavior>(this);
	m_csscombat = std::make_unique<CCSSBotCombat>(this);
	m_cssinventory = std::make_unique<CCSSBotInventory>(this);
	m_cssmovement = std::make_unique<CCSSBotMovement>(this);
	m_csssensor = std::make_unique<CCSSBotSensor>(this);
	m_buyprofile = CCounterStrikeSourceMod::GetCSSMod()->GetBuyManager().GetRandomBuyProfile();
}

CCSSBot::~CCSSBot()
{
	m_buyprofile = nullptr;
}

void CCSSBot::SendBuyCommand(const char* item)
{
	char fmt[256];
	ke::SafeSprintf(fmt, sizeof(fmt), "buy %s", item);
	DelayedFakeClientCommand(fmt);
}

float CCSSBotPathCost::operator()(CNavArea* toArea, CNavArea* fromArea, const CNavLadder* ladder, const NavOffMeshConnection* link, const CNavElevator* elevator, float length) const
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("CCSSBotPathCost::operator()", "NavBot");
#endif // EXT_VPROF_ENABLED

	float cost = CBasicPathCost<CCSSBot>::operator()(toArea, fromArea, ladder, link, elevator, length);

	// first area in the path;
	if (fromArea == nullptr)
	{
		return cost;
	}

	return cost;
}
