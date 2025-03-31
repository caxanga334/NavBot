#include <extension.h>
#include <mods/dods/dodslib.h>
#include "dodsbot.h"

CDoDSBot::CDoDSBot(edict_t* edict) :
	CBaseBot(edict)
{
	m_dodsensor = std::make_unique<CDoDSBotSensor>(this);
	m_dodbehavior = std::make_unique<CDoDSBotBehavior>(this);
}

CDoDSBot::~CDoDSBot()
{
}

bool CDoDSBot::HasJoinedGame()
{
	return GetMyDoDTeam() >= dayofdefeatsource::DoDTeam::DODTEAM_ALLIES && GetMyClassType() != dayofdefeatsource::DoDClassType::DODCLASS_INVALID;
}

void CDoDSBot::TryJoinGame()
{
	DelayedFakeClientCommand("jointeam 0"); // Random Team
	DelayedFakeClientCommand("cls_random"); // Random Class
}

dayofdefeatsource::DoDTeam CDoDSBot::GetMyDoDTeam() const
{
	return dodslib::GetDoDTeam(GetEntity());
}

dayofdefeatsource::DoDClassType CDoDSBot::GetMyClassType() const
{
	return dodslib::GetPlayerClassType(GetEntity());
}

CDoDSBotPathCost::CDoDSBotPathCost(CDoDSBot* bot, RouteType type)
{
	m_me = bot;
	m_routetype = type;
	m_stepheight = bot->GetMovementInterface()->GetStepHeight();
	m_maxjumpheight = bot->GetMovementInterface()->GetMaxJumpHeight();
	m_maxdropheight = bot->GetMovementInterface()->GetMaxDropHeight();
	m_maxgapjumpdistance = bot->GetMovementInterface()->GetMaxGapJumpDistance();
}

float CDoDSBotPathCost::operator()(CNavArea* toArea, CNavArea* fromArea, const CNavLadder* ladder, const NavOffMeshConnection* link, const CNavElevator* elevator, float length) const
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
			if (deltaZ > m_maxjumpheight)
			{
				// too high, DoD doesn't have double jumps so we don't bother checking
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
		if (link->GetType() == OffMeshConnectionType::OFFMESH_DOUBLE_JUMP || link->GetType() == OffMeshConnectionType::OFFMESH_BLAST_JUMP)
		{
			return -1.0f;
		}
	}

	float cost = dist + fromArea->GetCostSoFar();

	return cost;
}
