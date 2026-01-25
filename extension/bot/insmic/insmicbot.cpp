#include NAVBOT_PCH_FILE
#include <mods/insmic/insmicmod.h>
#include "insmicbot.h"

CInsMICBot::CInsMICBot(edict_t* edict) :
	CBaseBot(edict)
{
	m_insmicsensor = std::make_unique<CInsMICBotSensor>(this);
	m_insmicbehavior = std::make_unique<CInsMICBotBehavior>(this);
	m_insmicinventory = std::make_unique<CInsMICBotInventory>(this);
}

bool CInsMICBot::HasJoinedGame()
{
	insmic::InsMICTeam myteam = GetMyInsurgencyTeam();
	return myteam == insmic::InsMICTeam::INSMICTEAM_USMC || myteam == insmic::InsMICTeam::INSMICTEAM_INSURGENTS;
}

void CInsMICBot::TryJoinGame()
{
	/*
	int teamid, data;

	CInsMICMod::GetInsurgencyMod()->SelectTeamAndSquadForBot(teamid, data);

	if (data != insmic::INVALID_SLOT)
	{
		char cmd[64];
		ke::SafeSprintf(cmd, sizeof(cmd), "pfullsetup %i %i\n", teamid, data);
		DelayedFakeClientCommand(cmd);
	}
	*/

	// this seems to auto assign bots to both a team and squad.
	DelayedFakeClientCommand("pteamsetup 2");
}

CInsMICBotPathCost::CInsMICBotPathCost(CInsMICBot* bot, RouteType type) :
	IGroundPathCost(bot), m_me(bot), m_routetype(type)
{
}

float CInsMICBotPathCost::operator()(CNavArea* toArea, CNavArea* fromArea, const CNavLadder* ladder, const NavOffMeshConnection* link, const CNavElevator* elevator, float length) const
{
	float cost = GetGroundMovementCost(toArea, fromArea, ladder, link, elevator, length);

	if (cost < 0.0f)
	{
		return cost; // dead end
	}

	// insurgency specific costs goes here

	return cost;
}
