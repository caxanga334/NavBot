#include NAVBOT_PCH_FILE
#include <mods/zps/zps_mod.h>
#include "zpsbot.h"

CZPSBot::CZPSBot(edict_t* edict) :
	CBaseBot(edict)
{
	m_zpsbehavior = std::make_unique<CZPSBot::CZPSBotBehavior>(this);
	m_zpssensor = std::make_unique<CZPSBotSensor>(this);
	m_zpsmovement = std::make_unique<CZPSBotMovement>(this);
	m_zpsinventory = std::make_unique<CZPSBotInventory>(this);
	m_zpscombat = std::make_unique<CZPSBotCombat>(this);
}

CZPSBot::~CZPSBot()
{
}

zps::ZPSTeam CZPSBot::GetMyZPSTeam() const
{
	return static_cast<zps::ZPSTeam>(GetCurrentTeamIndex());
}

CZPSBotPathCost::CZPSBotPathCost(CZPSBot* bot, RouteType type) :
	m_routetype(type), IGroundPathCost(bot)
{
	m_me = bot;

	if (type == FASTEST_ROUTE)
	{
		SetIgnoreDanger(true);
	}

	SetTeamIndex(bot->GetCurrentTeamIndex());
}

float CZPSBotPathCost::operator()(CNavArea* toArea, CNavArea* fromArea, const CNavLadder* ladder, const NavOffMeshConnection* link, const CNavElevator* elevator, float length) const
{
	float cost = GetGroundMovementCost(toArea, fromArea, ladder, link, elevator, length);

	// add any ZPS specific cost changes here

	return cost;
}

