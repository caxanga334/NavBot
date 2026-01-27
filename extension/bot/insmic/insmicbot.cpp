#include NAVBOT_PCH_FILE
#include <mods/insmic/insmicmod.h>
#include "insmicbot.h"

CInsMICBot::CInsMICBot(edict_t* edict) :
	CBaseBot(edict)
{
	m_insmicsensor = std::make_unique<CInsMICBotSensor>(this);
	m_insmicbehavior = std::make_unique<CInsMICBotBehavior>(this);
	m_insmicinventory = std::make_unique<CInsMICBotInventory>(this);
	m_insmicmovement = std::make_unique<CInsMICBotMovement>(this);
	m_insmiccombat = std::make_unique<CInsMICBotCombat>(this);
}

CBaseEntity* CInsMICBot::GetActiveWeapon() const
{
	CBaseEntity* weapon = nullptr;
	entprops->GetEntPropEnt(GetEntity(), Prop_Send, "m_hActiveWeapon", nullptr, &weapon);
	return weapon;
}

bool CInsMICBot::HasJoinedGame()
{
	insmic::InsMICTeam myteam = GetMyInsurgencyTeam();
	
	if (myteam == insmic::InsMICTeam::INSMICTEAM_USMC || myteam == insmic::InsMICTeam::INSMICTEAM_INSURGENTS)
	{
		if (CInsMICMod::GetInsurgencyMod()->GetPlayerSquadData(GetEntity()) != 0)
		{
			return true;
		}
	}

	return false;
}

void CInsMICBot::TryJoinGame()
{
	insmic::InsMICTeam myteam = GetMyInsurgencyTeam();

	// auto assign doesn't work on maps without an IMC config
	if (myteam == insmic::InsMICTeam::INSMICTEAM_USMC || myteam == insmic::InsMICTeam::INSMICTEAM_INSURGENTS)
	{
		int teamid, data;

		CInsMICMod::GetInsurgencyMod()->SelectTeamAndSquadForBot(teamid, data);

		if (data != insmic::INVALID_SLOT)
		{
			char cmd[64];
			ke::SafeSprintf(cmd, sizeof(cmd), "pfullsetup %i %i\n", teamid, data);
			DelayedFakeClientCommand(cmd);
		}

		return;
	}

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
