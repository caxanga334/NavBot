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
		return NO_TRAVERSE_COST;
	}

	if (!m_moveiface->IsAreaTraversable(toArea))
	{
		return DEADEND_COST;
	}

	float dist = ComputeDistance(toArea, fromArea, ladder, link, elevator, length);

	if (dist < 0.0f)
	{
		return DEADEND_COST;
	}

	// only check gap and height on common connections
	if (link == nullptr && elevator == nullptr && ladder == nullptr)
	{
		if (!HandleJumpsAndDrops(toArea, fromArea, dist))
		{
			return DEADEND_COST;
		}
	}
	else if (link != nullptr)
	{
		// Query the movement interface to see if we're allowed to use this off-mesh connection
		if (!m_moveiface->IsAbleToUseOffMeshConnection(link->GetType(), link))
		{
			return DEADEND_COST;
		}
	}

	RouteType type = GetRouteType();

	if (toArea->HasAvoidanceObstacle(m_movecaps.m_stepheight))
	{
		dist *= OBSTRUCTED_COST_MULTIPLIER;
	}

	if (toArea->HasAttributes(static_cast<int>(NavAttributeType::NAV_MESH_AVOID)))
	{
		dist *= NAV_AVOID_ATTRIB_MULTI;
	}

	// Crouching slows us down, avoid it when looking for fast routes
	if (type == FASTEST_ROUTE && toArea->HasAttributes(static_cast<int>(NavAttributeType::NAV_MESH_CROUCH)))
	{
		dist *= FASTEST_ROUTE_CROUCH_MULT;
	}

	float cost = dist + fromArea->GetCostSoFar();

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

	return cost;
}

bool IGroundPathCost::CheckDrop(const CNavArea* toArea, const CNavArea* fromArea, const float deltaZ) const
{
	if (IsWaterSafe() && toArea->IsInWater())
	{
		// safe drop: we are landing in an underwater area.
		return false;
	}

	if (deltaZ < -m_movecaps.m_maxdropheight)
	{
		// unsafe drop: vertical distance is higher than the max safe drop height
		// note that deltaZ is negative when we are going from up to down.
		return true;
	}

	return false;
}

bool IGroundPathCost::HandleJumpsAndDrops(const CNavArea* toArea, const CNavArea* fromArea, float& dist) const
{
	if (fromArea->IsUnderwater() && toArea->IsUnderwater())
	{
		return true; // skip other checks, when both are underwater, assume the bot can freely change heights using move up/move down
	}

	float deltaZ = fromArea->ComputeAdjacentConnectionHeightChange(toArea);

	if (deltaZ >= m_movecaps.m_stepheight)
	{
		if (m_movecaps.m_candoublejump)
		{
			if (deltaZ > m_movecaps.m_maxdjheight)
			{
				// too high to reach by jumping
				return false;
			}
		}
		else if (deltaZ > m_movecaps.m_maxjumpheight)
		{
			// too high to reach by jumping
			return false;
		}

		// jump type is resolved by the navigator

		// add jump penalty
		dist *= JUMP_COST_MULTIPLIER;
	}
	else if (CheckDrop(toArea, fromArea, deltaZ))
	{
		// too far to drop
		// TO-DO: Handle areas that breaks fall damage.
		return false;
	}

	float gap = fromArea->ComputeAdjacentConnectionGapDistance(toArea);

	if (gap >= m_movecaps.m_maxgapjumpdistance)
	{
		return false; // can't jump over this gap
	}

	return true;
}

float IGroundPathCost::ComputeDistance(const CNavArea* toArea, const CNavArea* fromArea, const CNavLadder* ladder, const NavOffMeshConnection* link, const CNavElevator* elevator, float length) const
{
	if (link != nullptr)
	{
		return link->GetConnectionLength();
	}

	if (ladder != nullptr) // experimental, very few maps have 'true' ladders
	{
		return ladder->m_length;
	}

	if (elevator != nullptr)
	{
		const CNavElevator::ElevatorFloor* fromFloor = fromArea->GetMyElevatorFloor();

		if (!fromFloor->HasCallButton() && !fromFloor->is_here)
		{
			return DEADEND_COST; // Unable to use this elevator, lacks a button to call it to this floor and is not on this floor
		}

		return elevator->GetLengthBetweenFloors(fromArea, toArea);
	}

	if (length > 0.0f)
	{
		return length;
	}
	
	return (toArea->GetCenter() + fromArea->GetCenter()).Length();
}
