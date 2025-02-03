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
	engine->SetFakeClientConVarValue(GetEdict(), "cl_charactertype", "mp_scientist_female");
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

CBlackMesaBotPathCost::CBlackMesaBotPathCost(CBlackMesaBot* bot, RouteType routetype)
{
	m_me = bot;
	m_routetype = routetype;
	m_stepheight = bot->GetMovementInterface()->GetStepHeight();
	m_maxjumpheight = bot->GetMovementInterface()->GetMaxJumpHeight();
	m_maxdropheight = bot->GetMovementInterface()->GetMaxDropHeight();
	m_maxdjheight = bot->GetMovementInterface()->GetMaxDoubleJumpHeight();
	m_maxgapjumpdistance = bot->GetMovementInterface()->GetMaxGapJumpDistance();
	m_candoublejump = bot->GetMovementInterface()->IsAbleToDoubleJump();
	m_canblastjump = bot->GetMovementInterface()->IsAbleToBlastJump();
}

float CBlackMesaBotPathCost::operator()(CNavArea* toArea, CNavArea* fromArea, const CNavLadder* ladder, const NavOffMeshConnection* link, const CNavElevator* elevator, float length) const
{
	if (fromArea == nullptr)
	{
		// first area in path, no cost
		return 0.0f;
	}

	if (!m_me->GetMovementInterface()->IsAreaTraversable(toArea))
	{
		return -1.0f;
	}

	float dist = 0.0f;

	if (link != nullptr)
	{
		dist = link->GetConnectionLength();
	}
	else if (ladder != nullptr) // experimental, very few maps have 'true' ladders
	{
		dist = ladder->m_length;
	}
	else if (elevator != nullptr)
	{
		auto fromFloor = fromArea->GetMyElevatorFloor();

		if (!fromFloor->HasCallButton() && !fromFloor->is_here)
		{
			return -1.0f; // Unable to use this elevator, lacks a button to call it to this floor and is not on this floor
		}

		dist = elevator->GetLengthBetweenFloors(fromArea, toArea);
	}
	else if (length > 0.0f)
	{
		dist = length;
	}
	else
	{
		dist = (toArea->GetCenter() + fromArea->GetCenter()).Length();
	}

	// only check gap and height on common connections
	if (link == nullptr && elevator == nullptr && ladder == nullptr)
	{
		float deltaZ = fromArea->ComputeAdjacentConnectionHeightChange(toArea);

		if (deltaZ >= m_stepheight)
		{
			if (m_candoublejump && deltaZ > m_maxdjheight)
			{
				// too high to reach with double jumps
				return -1.0f;
			}
			else if (deltaZ > m_maxjumpheight)
			{
				// too high to reach with regular jumps
				return -1.0f;
			}

			// jump type is resolved by the navigator

			// add jump penalty
			constexpr auto jumpPenalty = 2.0f;
			dist *= jumpPenalty;
		}
		else if (deltaZ < -m_maxdropheight)
		{
			// too far to drop
			return -1.0f;
		}

		float gap = fromArea->ComputeAdjacentConnectionGapDistance(toArea);

		if (gap >= m_maxgapjumpdistance)
		{
			return -1.0f; // can't jump over this gap
		}
	}
	else if (link != nullptr)
	{
		// Don't use double jump links if we can't perform a double jump
		if (link->GetType() == OffMeshConnectionType::OFFMESH_DOUBLE_JUMP && !m_candoublejump)
		{
			return -1.0f;
		}
		else if (link->GetType() == OffMeshConnectionType::OFFMESH_BLAST_JUMP && !m_canblastjump)
		{
			return -1.0f;
		}
	}

	float cost = dist + fromArea->GetCostSoFar();

	return cost;
}
