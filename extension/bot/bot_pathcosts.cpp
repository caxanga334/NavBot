#include NAVBOT_PCH_FILE
#include "basebot.h"
#include "bot_pathcosts.h"

#ifdef EXT_VPROF_ENABLED
#include <tier0/vprof.h>
#endif // EXT_VPROF_ENABLED

void HumanMovementCaps_t::Init(IMovement* movement)
{
	m_stepheight = movement->GetStepHeight();
	m_maxjumpheight = movement->GetMaxJumpHeight();
	m_maxdjheight = movement->GetMaxDoubleJumpHeight();
	m_maxdropheight = movement->GetMaxDropHeight();
	m_maxgapjumpdistance = movement->GetMaxGapJumpDistance();
	m_candoublejump = movement->IsAbleToDoubleJump();
}

float IGroundPathCost::GetGroundMovementCost(CNavArea* toArea, CNavArea* fromArea, const CNavLadder* ladder, const NavOffMeshConnection* link, const CNavElevator* elevator, float length) const
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("IGroundPathCost::GetGroundMovementCost", "NavBot");
#endif // EXT_VPROF_ENABLED

	if (fromArea == nullptr)
	{
		// first area in path, no cost
		return 0.0f;
	}

	if (!m_moveiface->IsAreaTraversable(toArea))
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
		const CNavElevator::ElevatorFloor* fromFloor = fromArea->GetMyElevatorFloor();

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

		if (deltaZ >= m_movecaps.m_stepheight)
		{
			if (m_movecaps.m_candoublejump)
			{
				if (deltaZ > m_movecaps.m_maxdjheight)
				{
					// too high to reach by jumping
					return -1.0f;
				}
			}
			else if (deltaZ > m_movecaps.m_maxjumpheight)
			{
				// too high to reach by jumping
				return -1.0f;
			}

			// jump type is resolved by the navigator

			// add jump penalty
			constexpr auto jumpPenalty = 2.0f;
			dist *= jumpPenalty;
		}
		else if (deltaZ < -m_movecaps.m_maxdropheight)
		{
			// too far to drop
			// TO-DO: Handle areas that breaks fall damage.
			return -1.0f;
		}

		float gap = fromArea->ComputeAdjacentConnectionGapDistance(toArea);

		if (gap >= m_movecaps.m_maxgapjumpdistance)
		{
			return -1.0f; // can't jump over this gap
		}
	}
	else if (link != nullptr)
	{
		// Query the movement interface to see if we're allowed to use this off-mesh connection
		if (!m_moveiface->IsAbleToUseOffMeshConnection(link->GetType(), link))
		{
			return -1.0f;
		}
	}

	float cost = dist + fromArea->GetCostSoFar();

	RouteType type = GetRouteType();

	if (!IsIngoringDanger())
	{
		// Fastest routes always ignores danger
		if (type != FASTEST_ROUTE)
		{
			// SAFEST_ROUTE really cares about danger, the others only a little
			const float dangermult = type == SAFEST_ROUTE ? 1.0f : 0.25f;
			float danger = toArea->GetDanger(GetTeamIndex());
			
			cost += (danger * dangermult);
		}
	}

	// Crouching slows us down, avoid it when looking for fast routes
	if (type == FASTEST_ROUTE && toArea->HasAttributes(static_cast<int>(NavAttributeType::NAV_MESH_CROUCH)))
	{
		constexpr float crouchmult = 1.9f;
		cost *= crouchmult;
	}

	return cost;
}
