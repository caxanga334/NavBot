#include <extension.h>
#include <navmesh/nav_mesh.h>
#include <navmesh/nav_area.h>
#include "basebot_pathcost.h"

CBaseBotPathCost::CBaseBotPathCost(CBaseBot* bot)
{
	auto mover = bot->GetMovementInterface();
	m_me = bot;
	m_stepheight = mover->GetStepHeight();
	m_maxjumpheight = mover->GetMaxJumpHeight();
	m_maxdropheight = mover->GetMaxDropHeight();
	m_maxdjheight = mover->GetMaxDoubleJumpHeight();
	m_maxgapjumpdistance = mover->GetMaxGapJumpDistance();
	m_candoublejump = mover->IsAbleToDoubleJump();
	m_canblastjump = mover->IsAbleToBlastJump();
}

float CBaseBotPathCost::operator()(CNavArea* toArea, CNavArea* fromArea, const CNavLadder* ladder, const NavOffMeshConnection* link, const CNavElevator* elevator, float length) const
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
