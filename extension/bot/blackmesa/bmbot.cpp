#include NAVBOT_PCH_FILE
#include <extension.h>
#include <util/entprops.h>
#include <mods/blackmesa/blackmesadm_mod.h>
#include "bmbot.h"

CBlackMesaBot::CBlackMesaBot(edict_t* edict) :
	CBaseBot(edict)
{
	m_bmmovement = std::make_unique<CBlackMesaBotMovement>(this);
	m_bmsensor = std::make_unique<CBlackMesaBotSensor>(this);
	m_bmbehavior = std::make_unique<CBlackMesaBotBehavior>(this);
	m_bminventory = std::make_unique<CBlackMesaBotInventory>(this);
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
	engine->SetFakeClientConVarValue(GetEdict(), "cl_defaultweapon", "weapon_glock");
	engine->SetFakeClientConVarValue(GetEdict(), "cl_weapon_autoswitch", "0");
	engine->SetFakeClientConVarValue(GetEdict(), "cl_auto_crouch_jump", "1");

	auto model = CBlackMesaDeathmatchMod::GetBMMod()->GetRandomPlayerModel();

	if (model)
	{
		int skin = 0;

		if (model->second > 1)
		{
			skin = CBaseBot::s_botrng.GetRandomInt<int>(0, model->second - 1);
		}

		std::unique_ptr<char[]> skinstring = std::make_unique<char[]>(32);
		ke::SafeSprintf(skinstring.get(), 32, "%i", skin);
		engine->SetFakeClientConVarValue(GetEdict(), "cl_charactertype", model->first.c_str());
		engine->SetFakeClientConVarValue(GetEdict(), "cl_characterskin", skinstring.get());
	}
	else
	{
		engine->SetFakeClientConVarValue(GetEdict(), "cl_charactertype", "mp_scientist_hev");
		engine->SetFakeClientConVarValue(GetEdict(), "cl_characterskin", "0");
	}
}

blackmesa::BMTeam CBlackMesaBot::GetMyBMTeam() const
{
	int team = 0;
	entprops->GetEntProp(GetIndex(), Prop_Send, "m_iTeamNum", team);
	return static_cast<blackmesa::BMTeam>(team);
}

int CBlackMesaBot::GetArmor() const
{
	int armor = 0;
	entprops->GetEntProp(GetIndex(), Prop_Send, "m_ArmorValue", armor);
	return armor;
}

bool CBlackMesaBot::HasLongJump() const
{
	bool longjump = false;
	entprops->GetEntPropBool(GetIndex(), Prop_Send, "m_bHasJumpModule", longjump);
	return longjump;
}

CBlackMesaBotPathCost::CBlackMesaBotPathCost(CBlackMesaBot* bot, RouteType routetype) :
	m_routetype(routetype), IGroundPathCost(bot)
{
	m_me = bot;

	if (routetype == FASTEST_ROUTE)
	{
		SetIgnoreDanger(true);
	}

	SetTeamIndex(bot->GetCurrentTeamIndex());
}

float CBlackMesaBotPathCost::operator()(CNavArea* toArea, CNavArea* fromArea, const CNavLadder* ladder, const NavOffMeshConnection* link, const CNavElevator* elevator, float length) const
{
	float cost = GetGroundMovementCost(toArea, fromArea, ladder, link, elevator, length);

	// add any Black Mesa specific cost changes here

	return cost;
}
