#include <extension.h>
#include <util/entprops.h>
#include <mods/blackmesa/blackmesadm_mod.h>
#include "bmbot.h"

CBlackMesaBot::CBlackMesaBot(edict_t* edict) :
	CBaseBot(edict)
{
	m_bmmovement = std::make_unique<CBlackMesaBotMovement>(this);
	m_bmsensor = std::make_unique<CBlackMesaBotSensor>(this);
}

CBlackMesaBot::~CBlackMesaBot()
{
}

bool CBlackMesaBot::HasJoinedGame()
{
	if (CBlackMesaDeathmatchMod::GetBMMod()->IsTeamPlay())
	{
		return GetCurrentTeamIndex() > TEAM_SPECTATOR;
	}

	// in deathmatch mode, all players are on team unassigned

	return GetCurrentTeamIndex() != TEAM_SPECTATOR;
}

void CBlackMesaBot::TryJoinGame()
{
	DelayedFakeClientCommand("jointeam -2");
}

void CBlackMesaBot::FirstSpawn()
{
	CBaseBot::FirstSpawn();

	engine->SetFakeClientConVarValue(GetEdict(), "cl_toggle_duck", "0");
}

blackmesa::BMTeam CBlackMesaBot::GetMyBMTeam() const
{
	int team = 0;
	entprops->GetEntProp(GetIndex(), Prop_Send, "m_iTeamNum", team);
	return static_cast<blackmesa::BMTeam>(team);
}
