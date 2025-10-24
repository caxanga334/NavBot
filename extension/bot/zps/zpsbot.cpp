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

void CZPSBot::FirstSpawn()
{
	CBaseBot::FirstSpawn();

	engine->SetFakeClientConVarValue(GetEdict(), "cl_playermodel", "random");
	engine->SetFakeClientConVarValue(GetEdict(), "cl_togglewalk", "1");
}

bool CZPSBot::IsAbleToBreak(CBaseEntity* entity)
{
	bool result = CBaseBot::IsAbleToBreak(entity);

	if (result)
	{
		const char* classname = gamehelpers->GetEntityClassname(entity);

		if (GetMyZPSTeam() == zps::ZPSTeam::ZPS_TEAM_SURVIVORS)
		{
			// Survivor bots don't attack these
			if (std::strcmp(classname, "prop_barricade") == 0)
			{
				bool allowpickup = false;
				
				if (entprops->GetEntPropBool(entity, Prop_Data, "m_bAllowPickup", allowpickup) && !allowpickup)
				{
					// player placed barricade have m_bAllowPickup set to true, this one is probably a level designer placed
					return true;
				}

				return false;
			}
			if (std::strcmp(classname, "prop_door_rotating") == 0)
			{
				return false;
			}
		}
	}

	return result;
}

zps::ZPSTeam CZPSBot::GetMyZPSTeam() const
{
	return static_cast<zps::ZPSTeam>(GetCurrentTeamIndex());
}

bool CZPSBot::IsCarrier() const
{
	return zpslib::IsPlayerCarrier(this->GetEntity());
}

bool CZPSBot::IsInfected() const
{
	return zpslib::IsPlayerInfected(this->GetEntity());
}

bool CZPSBot::IsWalking() const
{
	return zpslib::IsPlayerWalking(this->GetEntity());
}

CZPSBotPathCost::CZPSBotPathCost(CZPSBot* bot, RouteType type) :
	m_routetype(type), IGroundPathCost(bot)
{
	m_me = bot;

	if (type == FASTEST_ROUTE)
	{
		SetIgnoreDanger(true);
	}
}

float CZPSBotPathCost::operator()(CNavArea* toArea, CNavArea* fromArea, const CNavLadder* ladder, const NavOffMeshConnection* link, const CNavElevator* elevator, float length) const
{
	float cost = GetGroundMovementCost(toArea, fromArea, ladder, link, elevator, length);

	// add any ZPS specific cost changes here

	return cost;
}

