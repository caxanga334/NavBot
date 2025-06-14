//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
// nav_pathfind.h
// Path-finding mechanisms using the Navigation Mesh
// Author: Michael S. Booth (mike@turtlerockstudios.com), January 2003

#ifndef _NAV_PATHFIND_H_
#define _NAV_PATHFIND_H_

#include <algorithm>
#include <vector>
#include <stack>
#include <queue>
#include <unordered_set>
#include <unordered_map>
#include <functional>
#include <stdexcept>
#include <cstdlib>
#include <cinttypes>

#include <util/librandom.h>
#include "nav_area.h"
#include "nav_elevator.h"


#undef max
#undef min
#undef clamp

#ifdef STAGING_ONLY
extern int g_DebugPathfindCounter;
#endif


//-------------------------------------------------------------------------------------------------------------------
/**
 * Used when building a path to determine the kind of path to build
 */
enum RouteType
{
	DEFAULT_ROUTE,
	FASTEST_ROUTE,
	SAFEST_ROUTE,
	RETREAT_ROUTE,
};


//--------------------------------------------------------------------------------------------------------------
/**
 * Functor used with NavAreaBuildPath()
 */
class ShortestPathCost
{
public:
	float operator() ( CNavArea *area, CNavArea *fromArea, const CNavLadder *ladder, const NavOffMeshConnection *link, const CNavElevator *elevator, float length ) const
	{
		if ( fromArea == NULL )
		{
			// first area in path, no cost
			return 0.0f;
		}
		else
		{
			// compute distance traveled along path so far
			float dist;
			
			if (link)
			{
				dist = link->GetConnectionLength();
			}
			else if (elevator)
			{
				auto fromFloor = fromArea->GetMyElevatorFloor();

				if (!fromFloor->HasCallButton() && !fromFloor->is_here)
				{
					return -1.0f; // Unable to use this elevator, lacks a button to call it to this floor and is not on this floor
				}

				dist = elevator->GetLengthBetweenFloors(fromArea, area);
			}
			else if ( ladder )
			{
				dist = ladder->m_length;
			}
			else if ( length > 0.0 )
			{
				dist = length;
			}
			else
			{
				dist = ( area->GetCenter() - fromArea->GetCenter() ).Length();
			}

			float cost = dist + fromArea->GetCostSoFar();

			// if this is a "crouch" area, add penalty
			if ( area->GetAttributes() & NAV_MESH_CROUCH )
			{
				const float crouchPenalty = 20.0f;		// 10
				cost += crouchPenalty * dist;
			}

			// if this is a "jump" area, add penalty
			if ( area->GetAttributes() & NAV_MESH_JUMP )
			{
				const float jumpPenalty = 5.0f;
				cost += jumpPenalty * dist;
			}

			return cost;
		}
	}
};

//--------------------------------------------------------------------------------------------------------------
/**
 * Find path from startArea to goalArea via an A* search, using supplied cost heuristic.
 * If cost functor returns -1 for an area, that area is considered a dead end.
 * This doesn't actually build a path, but the path is defined by following parent
 * pointers back from goalArea to startArea.
 * If 'closestArea' is non-NULL, the closest area to the goal is returned (useful if the path fails).
 * If 'goalArea' is NULL, will compute a path as close as possible to 'goalPos'.
 * If 'goalPos' is NULL, will use the center of 'goalArea' as the goal position.
 * If 'maxPathLength' is nonzero, path building will stop when this length is reached.
 * Returns true if a path exists.
 */
#define IGNORE_NAV_BLOCKERS true
template< typename CostFunctor >
bool NavAreaBuildPath( CNavArea *startArea, CNavArea *goalArea, const Vector *goalPos,
		const CostFunctor &costFunc, CNavArea **closestArea = NULL, float maxPathLength = 0.0f, int teamID = NAV_TEAM_ANY, bool ignoreNavBlockers = false )
{
	if ( closestArea )
	{
		*closestArea = startArea;
	}

#ifdef STAGING_ONLY
	bool isDebug = ( g_DebugPathfindCounter-- > 0 );
#endif

	if (startArea == NULL)
		return false;

	startArea->SetParent( NULL );

	if (goalArea != NULL && goalArea->IsBlocked( teamID, ignoreNavBlockers ))
		goalArea = NULL;

	if (goalArea == NULL && goalPos == NULL)
		return false;

	// if we are already in the goal area, build trivial path
	if (startArea == goalArea)
	{
		return true;
	}

	// determine actual goal position
	Vector actualGoalPos = (goalPos) ? *goalPos : goalArea->GetCenter();

	// start search
	CNavArea::ClearSearchLists();

	// compute estimate of path length
	/// @todo Cost might work as "manhattan distance"
	startArea->SetTotalCost( (startArea->GetCenter() - actualGoalPos).Length() );

	/* CNavArea *area, CNavArea *fromArea, const CNavLadder *ladder, const NavOffMeshConnection *link, const CFuncElevator *elevator, float length */
	float initCost = costFunc( startArea, nullptr, nullptr, nullptr, nullptr, -1.0f );	
	if (initCost < 0.0f)
		return false;
	startArea->SetCostSoFar( initCost );
	startArea->SetPathLengthSoFar( 0.0 );

	startArea->AddToOpenList();

	// keep track of the area we visit that is closest to the goal
	float closestAreaDist = startArea->GetTotalCost();

	// do A* search
	while( !CNavArea::IsOpenListEmpty() )
	{
		// get next area to check
		CNavArea *area = CNavArea::PopOpenList();

#ifdef STAGING_ONLY
		if ( isDebug )
		{
			area->DrawFilled( 0, 255, 0, 128, 30.0f );
		}
#endif

		// don't consider blocked areas
		if ( area->IsBlocked( teamID, ignoreNavBlockers ) )
			continue;

		// check if we have found the goal area or position
		if (area == goalArea || (goalArea == NULL && goalPos && area->Contains( *goalPos )))
		{
			if (closestArea)
			{
				*closestArea = area;
			}

			return true;
		}

		// search adjacent areas
		enum SearchType
		{
			SEARCH_FLOOR, SEARCH_LADDERS, SEARCH_ELEVATORS, SEARCH_LINKS
		};
		SearchType searchWhere = SEARCH_FLOOR;
		int searchIndex = 0;
		size_t linkIndex = 0U;

		int dir = NORTH;
		const NavConnectVector *floorList = area->GetAdjacentAreas( NORTH );
		auto& linklist = area->GetOffMeshConnections();
		size_t maxlinks = area->GetOffMeshConnectionCount();

		bool ladderUp = true;
		const NavLadderConnectVector *ladderList = nullptr;
		std::size_t ladderConnIndex = 0;
		const std::vector<LadderToAreaConnection>* ladderAreaList = nullptr;
		std::size_t elevFloorIndex = 0;
		bool bHaveMaxPathLength = ( maxPathLength > 0.0f );
		float length = -1;
		
		while( true )
		{
			CNavArea *newArea = nullptr;
			NavTraverseType how;
			const CNavLadder *ladder = nullptr;
			const CNavElevator *elevator = nullptr;
			const NavOffMeshConnection* currentlink = nullptr;

			//
			// Get next adjacent area - either on floor or via ladder
			//
			if ( searchWhere == SEARCH_FLOOR )
			{
				// if exhausted adjacent connections in current direction, begin checking next direction
				if ( searchIndex >= floorList->Count() )
				{
					++dir;

					if ( dir == NUM_DIRECTIONS )
					{
						// checked all directions on floor - check ladders next
						searchWhere = SEARCH_LADDERS;

						ladderList = area->GetLadders( CNavLadder::LADDER_UP );
						ladderConnIndex = 0;
					}
					else
					{
						// start next direction
						floorList = area->GetAdjacentAreas( (NavDirType)dir );
					}
					searchIndex = 0;
					continue;
				}

				const NavConnect &floorConnect = floorList->Element( searchIndex );
				newArea = floorConnect.area;
				length = floorConnect.length;
				how = (NavTraverseType)dir;
				++searchIndex;

				if ( IsX360() && searchIndex < floorList->Count() )
				{
					PREFETCH360( floorList->Element( searchIndex ).area, 0  );
				}
			}
			else if ( searchWhere == SEARCH_LADDERS )
			{
				if ( searchIndex >= ladderList->Count() )
				{
					if ( !ladderUp )
					{
						// checked both ladder directions - check elevators next
						searchWhere = SEARCH_ELEVATORS;
						ladder = NULL;
					}
					else
					{
						// check down ladders
						ladderUp = false;
						ladderList = area->GetLadders( CNavLadder::LADDER_DOWN );
						ladderConnIndex = 0;
					}
					searchIndex = 0;
					continue;
				}

				if ( ladderUp )
				{
					ladder = ladderList->Element( searchIndex ).ladder;

					// if index is 0, update the area vectors
					if (ladderConnIndex == 0)
					{
						ladderAreaList = &ladder->GetConnections();
					}

					if (ladderConnIndex >= ladderAreaList->size())
					{
						++searchIndex; // go to next ladder
						ladderConnIndex = 0; // reset index
						continue;
					}
					else
					{
						newArea = ladderAreaList->at(ladderConnIndex).GetConnectedArea();
						ladderConnIndex++; // increment index
					}
				}
				else
				{
					ladder = ladderList->Element(searchIndex).ladder;

					// if index is 0, update the area vectors
					if (ladderConnIndex == 0)
					{
						ladderAreaList = &ladder->GetConnections();
					}

					if (ladderConnIndex >= ladderAreaList->size())
					{
						++searchIndex; // go to next ladder
						ladderConnIndex = 0; // reset index
						continue;
					}
					else
					{
						newArea = ladderAreaList->at(ladderConnIndex).GetConnectedArea();
						ladderConnIndex++; // increment index
					}
				}

				// Skip self
				if (newArea == area)
					continue;

				if ( newArea == NULL )
					continue;

				/* Because ladder connections can now be anywhere within the ladder, determine direction based on the Z axis */
				how = newArea->GetCenter().z > area->GetCenter().z ? GO_LADDER_UP : GO_LADDER_DOWN;
				length = -1.0f;
/*
				{
					const Vector& c1 = area->GetCenter();
					const Vector& c2 = newArea->GetCenter();
					META_CONPRINTF("BuildPath Ladder #%i: area #%u <%g %g %g> newArea #%u <%g %g %g> how %s \n", ladder->GetID(), area->GetID(), c1.x, c1.y, c1.z,
						newArea->GetID(), c2.x, c2.y, c2.z, 
						how == GO_LADDER_UP ? "LADDER_UP" : "LADDER_DOWN");
				}
*/
			}
			else if ( searchWhere == SEARCH_ELEVATORS )
			{
				elevator = area->GetElevator();

				if (elevator == nullptr || elevFloorIndex >= elevator->GetFloors().size())
				{
					// done searching connected areas
					elevator = nullptr;
					searchWhere = SEARCH_LINKS;
					elevFloorIndex = 0;
					continue;
				}

				newArea = elevator->GetFloors()[elevFloorIndex].GetArea();

				if (newArea == area)
				{
					elevFloorIndex++; // skip self
					continue;
				}

				how = newArea->GetCenter().z > area->GetCenter().z ? GO_ELEVATOR_UP : GO_ELEVATOR_DOWN;
				elevFloorIndex++;

				length = -1.0f;
			}
			else // if (searchWhere == SEARCH_LINKS)
			{
				if (maxlinks == 0)
				{
					// no link to search, get out
					break;
				}
				else
				{
					if (linkIndex >= maxlinks)
					{
						linkIndex = 0;
						break; // ALL links searched, break
					}

					currentlink = &linklist[linkIndex];
					newArea = currentlink->m_link.area;
					length = currentlink->GetConnectionLength();
					how = GO_OFF_MESH_CONNECTION;
					linkIndex++;
				}
			}

			// don't backtrack
			// Assert( newArea );
			if ( newArea == area->GetParent()
				|| newArea == area // self neighbor?
				// don't consider blocked areas
				|| newArea->IsBlocked( teamID, ignoreNavBlockers ) )
				continue;

			/* CNavArea *area, CNavArea *fromArea, const CNavLadder *ladder, const NavOffMeshConnection *link, const CNavElevator *elevator, float length */
			float newCostSoFar = costFunc( newArea, area, ladder, currentlink, elevator, length );

			// NaNs really mess this function up causing tough to track down hangs. If
			//  we get inf back, clamp it down to a really high number.
			// DebuggerBreakOnNaN_StagingOnly( newCostSoFar );
			if ( IS_NAN( newCostSoFar ) )
				newCostSoFar = 1e30f;

			// check if cost functor says this area is a dead-end
			if ( newCostSoFar < 0.0f )
				continue;

			// Safety check against a bogus functor.  The cost of the path
			// A...B, C should always be at least as big as the path A...B.
			// Assert( newCostSoFar >= area->GetCostSoFar() );

			// And now that we've asserted, let's be a bit more defensive.
			// Make sure that any jump to a new area incurs some pathfinsing
			// cost, to avoid us spinning our wheels over insignificant cost
			// benefit, floating point precision bug, or busted cost functor.
			newCostSoFar = std::max(newCostSoFar, area->GetCostSoFar() * 1.00001f + 0.00001f);
				
			// stop if path length limit reached
			if ( bHaveMaxPathLength )
			{
				// keep track of path length so far
				float newLengthSoFar = area->GetPathLengthSoFar() + ( newArea->GetCenter() - area->GetCenter() ).Length();
				if ( newLengthSoFar > maxPathLength )
					continue;
				
				newArea->SetPathLengthSoFar( newLengthSoFar );
			}

			if ( ( newArea->IsOpen() || newArea->IsClosed() ) && newArea->GetCostSoFar() <= newCostSoFar )
			{
				// this is a worse path - skip it
				continue;
			}
			// compute estimate of distance left to go
			float distSq = ( newArea->GetCenter() - actualGoalPos ).LengthSqr();
			float newCostRemaining = ( distSq > 0.0 ) ? FastSqrt( distSq ) : 0.0 ;

			// track closest area to goal in case path fails
			if ( closestArea && newCostRemaining < closestAreaDist )
			{
				*closestArea = newArea;
				closestAreaDist = newCostRemaining;
			}

			newArea->SetCostSoFar( newCostSoFar );
			newArea->SetTotalCost( newCostSoFar + newCostRemaining );

			if ( newArea->IsClosed() )
			{
				newArea->RemoveFromClosedList();
			}

			if ( newArea->IsOpen() )
			{
				// area already on open list, update the list order to keep costs sorted
				newArea->UpdateOnOpenList();
			}
			else
			{
				newArea->AddToOpenList();
			}

			newArea->SetParent( area, how );
		}

		// we have searched this area
		area->AddToClosedList();
	}

	return false;
}

/**
 * @brief Checks if the goal area is reachable from the start area.
 * @tparam CostFunctor A* cost function
 * @param start Start area
 * @param goal Goal/End area
 * @param costFunc A* cost function
 * @return true if the start area can reach the goal area. false otherwise.
 */
template<typename CostFunctor>
bool NavIsReachable(CNavArea* start, CNavArea* goal, CostFunctor& costFunc)
{
	if (start == nullptr || goal == nullptr)
		return false;

	if (start == goal)
		return true;

	return NavAreaBuildPath(start, goal, nullptr, costFunc, nullptr);
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Compute distance between two areas. Return -1 if can't reach 'endArea' from 'startArea'.
 */
template< typename CostFunctor >
float NavAreaTravelDistance( CNavArea *startArea, CNavArea *endArea, CostFunctor &costFunc, float maxPathLength = 0.0f )
{
	if (startArea == NULL || endArea == NULL)
		return -1.0f;

	if (startArea == endArea)
		return 0.0f;

	// compute path between areas using given cost heuristic
	if (NavAreaBuildPath( startArea, endArea, NULL, costFunc, NULL, maxPathLength ) == false)
		return -1.0f;

	// compute distance along path
	float distance = 0.0f;
	for( CNavArea *area = endArea; area->GetParent(); area = area->GetParent() )
	{
		distance += (area->GetCenter() - area->GetParent()->GetCenter()).Length();
	}

	return distance;
}



//--------------------------------------------------------------------------------------------------------------
/**
 * Do a breadth-first search, invoking functor on each area.
 * If functor returns 'true', continue searching from this area. 
 * If functor returns 'false', the area's adjacent areas are not explored (dead end).
 * If 'maxRange' is 0 or less, no range check is done (all areas will be examined).
 *
 * NOTE: Returns all areas that overlap range, even partially
 *
 * @todo Use ladder connections
 */

// helper function
inline void AddAreaToOpenList( CNavArea *area, CNavArea *parent, const Vector &startPos, float maxRange )
{
	if (area == NULL)
		return;

	if (!area->IsMarked())
	{
		area->Mark();
		area->SetTotalCost( 0.0f );
		area->SetParent( parent );

		if (maxRange > 0.0f)
		{
			// make sure this area overlaps range
			Vector closePos;
			area->GetClosestPointOnArea( startPos, &closePos );
			if ((closePos - startPos).AsVector2D().IsLengthLessThan( maxRange ))
			{
				// compute approximate distance along path to limit travel range, too
				float distAlong = parent->GetCostSoFar();
				distAlong += (area->GetCenter() - parent->GetCenter()).Length();
				area->SetCostSoFar( distAlong );

				// allow for some fudge due to large size areas
				if (distAlong <= 1.5f * maxRange)
					area->AddToOpenList();
			}
		}
		else
		{
			// infinite range
			area->AddToOpenList();
		}
	}
}


/****************************************************************
 * DEPRECATED: Use filter-based SearchSurroundingAreas below
 ****************************************************************/
#define INCLUDE_INCOMING_CONNECTIONS	0x1
#define INCLUDE_BLOCKED_AREAS			0x2
#define EXCLUDE_OUTGOING_CONNECTIONS	0x4
#define EXCLUDE_ELEVATORS				0x8
template < typename Functor >
void SearchSurroundingAreas( CNavArea *startArea, const Vector &startPos, Functor &func, float maxRange = -1.0f, unsigned int options = 0, int teamID = NAV_TEAM_ANY )
{
	if (startArea == NULL)
		return;

	CNavArea::MakeNewMarker();
	CNavArea::ClearSearchLists();

	startArea->AddToOpenList();
	startArea->SetTotalCost( 0.0f );
	startArea->SetCostSoFar( 0.0f );
	startArea->SetParent( NULL );
	startArea->Mark();

	while( !CNavArea::IsOpenListEmpty() )
	{
		// get next area to check
		CNavArea *area = CNavArea::PopOpenList();

		// don't use blocked areas
		if ( area->IsBlocked( teamID ) && !(options & INCLUDE_BLOCKED_AREAS) )
			continue;

		// invoke functor on area
		if (func( area ))
		{
			if (!(options & EXCLUDE_OUTGOING_CONNECTIONS)) {
				// explore adjacent floor areas
				for( int dir=0; dir<NUM_DIRECTIONS; ++dir )
				{
					int count = area->GetAdjacentCount( (NavDirType)dir );
					for( int i=0; i<count; ++i )
					{
						CNavArea *adjArea = area->GetAdjacentArea( (NavDirType)dir, i );
						if ( adjArea->IsConnected( area, NUM_DIRECTIONS ) )
						{
							AddAreaToOpenList( adjArea, area, startPos, maxRange );
						}
					}
				}

				auto& alllinks = area->GetOffMeshConnections();

				for (auto& link : alllinks)
				{
					CNavArea* other = link.m_link.area;
					AddAreaToOpenList(other, area, startPos, maxRange);
				}
			}
			// potentially include areas that connect TO this area via a one-way link
			if (options & INCLUDE_INCOMING_CONNECTIONS)
			{
				for( int dir=0; dir<NUM_DIRECTIONS; ++dir )
				{
					const NavConnectVector *list = area->GetIncomingConnections( (NavDirType)dir );
					FOR_EACH_VEC( (*list), it )
					{
						AddAreaToOpenList( (*list)[ it ].area, area, startPos, maxRange );
					}
				}
			}

			// explore adjacent areas connected by ladders

			// check up ladders
			const NavLadderConnectVector *ladderList = area->GetLadders( CNavLadder::LADDER_UP );
			if (ladderList)
			{
				FOR_EACH_VEC( (*ladderList), it )
				{
					const CNavLadder *ladder = (*ladderList)[ it ].ladder;

					for (auto& connect : ladder->GetConnections())
					{
						if (connect.IsConnectedToLadderTop())
						{
							AddAreaToOpenList(connect.GetConnectedArea(), area, startPos, maxRange);
						}
					}
				}
			}

			// check down ladders
			ladderList = area->GetLadders( CNavLadder::LADDER_DOWN );
			if (ladderList)
			{
				FOR_EACH_VEC( (*ladderList), it )
				{
					const CNavLadder* ladder = (*ladderList)[it].ladder;

					for (auto& connect : ladder->GetConnections())
					{
						if (connect.IsConnectedToLadderBottom())
						{
							AddAreaToOpenList(connect.GetConnectedArea(), area, startPos, maxRange);
						}
					}
				}
			}

			if ( (options & EXCLUDE_ELEVATORS) == 0 )
			{
				const CNavElevator* elevator = area->GetElevator();
				
				if (elevator != nullptr)
				{
					auto& floors = elevator->GetFloors();

					for (auto& floor : floors)
					{
						if (floor.GetArea() != area)
						{
							AddAreaToOpenList(floor.GetArea(), area, startPos, maxRange);
						}
					}
				}
			}
		}
	}
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Derive your own custom search functor from this interface method for use with SearchSurroundingAreas below.
 */
class ISearchSurroundingAreasFunctor
{
public:
	virtual ~ISearchSurroundingAreasFunctor() { }

	/**
	 * Perform user-defined action on area.
	 * Return 'false' to end the search (ie: you found what you were looking for)
	 */
	virtual bool operator() ( CNavArea *area, CNavArea *priorArea, float travelDistanceSoFar ) = 0;

	// return true if 'adjArea' should be included in the ongoing search
	virtual bool ShouldSearch( CNavArea *adjArea, CNavArea *currentArea, float travelDistanceSoFar ) 
	{
		return !adjArea->IsBlocked( NAV_TEAM_ANY );
	}

	/**
	 * Collect adjacent areas to continue the search by calling 'IncludeInSearch' on each
	 */
	virtual void IterateAdjacentAreas( CNavArea *area, CNavArea *priorArea, float travelDistanceSoFar ) 
	{
		// search adjacent outgoing connections
		for( int dir=0; dir<NUM_DIRECTIONS; ++dir )
		{
			int count = area->GetAdjacentCount( (NavDirType)dir );
			for( int i=0; i<count; ++i )
			{
				CNavArea *adjArea = area->GetAdjacentArea( (NavDirType)dir, i );

				if ( ShouldSearch( adjArea, area, travelDistanceSoFar ) )
				{
					IncludeInSearch( adjArea, area );
				}
			}
		}

		auto& links = area->GetOffMeshConnections();

		for (auto& speciallink : links)
		{
			CNavArea* adjArea = speciallink.m_link.area;

			if (ShouldSearch(adjArea, area, travelDistanceSoFar))
			{
				IncludeInSearch(adjArea, area, &speciallink);
			}
		}
	}

	// Invoked after the search has completed
	virtual void PostSearch( void ) { }

	// consider 'area' in upcoming search steps
	void IncludeInSearch(CNavArea *area, CNavArea *priorArea, const NavOffMeshConnection* link = nullptr)
	{
		if ( area == NULL )
			return;

		if ( !area->IsMarked() )
		{
			area->Mark();
			area->SetTotalCost( 0.0f );
			area->SetParent( priorArea );
			// compute approximate travel distance from start area of search
			if (link != nullptr)
			{
				area->SetCostSoFar(link->GetConnectionLength());
			}
			else
			{
				area->SetCostSoFar(priorArea ? priorArea->GetCostSoFar() + (area->GetCenter() - priorArea->GetCenter()).Length() : 0.0f);
			}
			
			// adding an area to the open list also marks it
			area->AddToOpenList();
		}
	}
};


/**
 * Do a breadth-first search starting from 'startArea' and continuing outward based on
 * adjacent areas that pass the given filter
 */
inline void SearchSurroundingAreas( CNavArea *startArea, ISearchSurroundingAreasFunctor &func, float travelDistanceLimit = -1.0f )
{
	if ( startArea )
	{
		CNavArea::MakeNewMarker();
		CNavArea::ClearSearchLists();

		startArea->AddToOpenList();
		startArea->SetTotalCost( 0.0f );
		startArea->SetCostSoFar( 0.0f );
		startArea->SetParent( NULL );
		startArea->Mark();

		CUtlVector< CNavArea * > adjVector;

		while( !CNavArea::IsOpenListEmpty() )
		{
			// get next area to check
			CNavArea *area = CNavArea::PopOpenList();

			if ( travelDistanceLimit > 0.0f && area->GetCostSoFar() > travelDistanceLimit )
				continue;

			if ( func( area, area->GetParent(), area->GetCostSoFar() ) )
			{
				func.IterateAdjacentAreas( area, area->GetParent(), area->GetCostSoFar() );
			}
			else
			{
				// search aborted
				break;
			}
		}
	}

	func.PostSearch();
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Starting from 'startArea', collect adjacent areas via a breadth-first search continuing outward until
 * 'travelDistanceLimit' is reached.
 * Areas in the collection will be "marked", returning true for IsMarked(). 
 * Each area in the collection's GetCostSoFar() will be approximate travel distance from 'startArea'.
 */
inline void CollectSurroundingAreas( CUtlVector< CNavArea * > *nearbyAreaVector, CNavArea *startArea, float travelDistanceLimit = 1500.0f, float maxStepUpLimit = navgenparams->step_height, float maxDropDownLimit = 100.0f )
{
	nearbyAreaVector->RemoveAll();

	if ( startArea )
	{
		CNavArea::MakeNewMarker();
		CNavArea::ClearSearchLists();

		startArea->AddToOpenList();
		startArea->SetTotalCost( 0.0f );
		startArea->SetCostSoFar( 0.0f );
		startArea->SetParent( NULL );
		startArea->Mark();

		CUtlVector< CNavArea * > adjVector;

		while( !CNavArea::IsOpenListEmpty() )
		{
			// get next area to check
			CNavArea *area = CNavArea::PopOpenList();

			if ( travelDistanceLimit > 0.0f && area->GetCostSoFar() > travelDistanceLimit )
				continue;

			if ( area->GetParent() )
			{
				float deltaZ = area->GetParent()->ComputeAdjacentConnectionHeightChange( area );

				if ( deltaZ > maxStepUpLimit
						|| deltaZ < -maxDropDownLimit )
					continue;
			}

			nearbyAreaVector->AddToTail( area );

			// mark here to ensure all marked areas are also valid areas that are in the collection
			area->Mark();

			// search adjacent outgoing connections
			for( int dir=0; dir<NUM_DIRECTIONS; ++dir )
			{
				int count = area->GetAdjacentCount( (NavDirType)dir );
				for( int i=0; i<count; ++i )
				{
					CNavArea *adjArea = area->GetAdjacentArea( (NavDirType)dir, i );

					if ( adjArea->IsBlocked( NAV_TEAM_ANY )
							|| adjArea->IsMarked() ) {
						continue;
					}
					adjArea->SetTotalCost( 0.0f );
					adjArea->SetParent( area );

					// compute approximate travel distance from start area of search
					adjArea->SetCostSoFar( area->GetCostSoFar()
							+ ( adjArea->GetCenter() - area->GetCenter() ).Length());
					adjArea->AddToOpenList();
				}
			}

			for (auto& links : area->GetOffMeshConnections())
			{
				CNavArea* adjArea = links.m_link.area;

				if (adjArea->IsBlocked(NAV_TEAM_ANY) || adjArea->IsMarked())
				{
					continue;
				}

				adjArea->SetTotalCost(0.0f);
				adjArea->SetParent(area, GO_OFF_MESH_CONNECTION);
				adjArea->SetCostSoFar(area->GetCostSoFar() + links.GetConnectionLength());
				adjArea->AddToOpenList();
			}
		}
	}
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Functor that returns lowest cost for farthest away areas
 * For use with FindMinimumCostArea()
 */
class FarAwayFunctor
{
public:
	float operator() ( CNavArea *area, CNavArea *fromArea, const CNavLadder *ladder )
	{
		return area == fromArea ? 9999999.9f
				: 1.0f/(fromArea->GetCenter() - area->GetCenter()).Length();
	}
};

/**
 * Functor that returns lowest cost for areas farthest from given position
 * For use with FindMinimumCostArea()
 */
class FarAwayFromPositionFunctor 
{
public:
	FarAwayFromPositionFunctor( const Vector &pos ) : m_pos( pos )
	{
	}

	float operator() ( CNavArea *area, CNavArea *fromArea, const CNavLadder *ladder )
	{
		return 1.0f/(m_pos - area->GetCenter()).Length();
	}

private:
	const Vector &m_pos;
};


/**
 * Pick a low-cost area of "decent" size
 */
template< typename CostFunctor >
CNavArea *FindMinimumCostArea( CNavArea *startArea, CostFunctor &costFunc )
{
	const float minSize = 150.0f;

	// collect N low-cost areas of a decent size
	enum { NUM_CHEAP_AREAS = 32 };
	struct 
	{
		CNavArea *area;
		float cost;
	}
	cheapAreaSet[ NUM_CHEAP_AREAS ] = {};
	int cheapAreaSetCount = 0;
	extern NavAreaVector TheNavAreas;
	FOR_EACH_VEC( TheNavAreas, iter )
	{
		CNavArea *area = TheNavAreas[iter];

		// skip the small areas
		if ( area->GetSizeX() < minSize || area->GetSizeY() < minSize)
			continue;

		// compute cost of this area

		// HPE_FIX[pfreese]: changed this to only pass three parameters, in accord with the two functors above
		float cost = costFunc( area, startArea, NULL );

		if (cheapAreaSetCount < NUM_CHEAP_AREAS)
		{
			cheapAreaSet[ cheapAreaSetCount ].area = area;
			cheapAreaSet[ cheapAreaSetCount++ ].cost = cost;
		}
		else
		{
			// replace most expensive cost if this is cheaper
			int expensive = 0;
			for( int i=1; i<NUM_CHEAP_AREAS; ++i )
				if (cheapAreaSet[i].cost > cheapAreaSet[expensive].cost)
					expensive = i;

			if (cheapAreaSet[expensive].cost > cost)
			{
				cheapAreaSet[expensive].area = area;
				cheapAreaSet[expensive].cost = cost;
			}
		}
	}

	if (cheapAreaSetCount)
	{
		// pick one of the areas at random
		return cheapAreaSet[randomgen->GetRandomInt<int>(0, cheapAreaSetCount - 1)].area;
	}
	else
	{
		// degenerate case - no decent sized areas - pick a random area
		int numAreas = TheNavAreas.Count();
		int which = randomgen->GetRandomInt<int>(0, numAreas - 1);

		FOR_EACH_VEC( TheNavAreas, iter )
		{
			if (which-- == 0)
				return TheNavAreas[iter];
		}

	}
	return cheapAreaSet[randomgen->GetRandomInt<int>(0, cheapAreaSetCount - 1)].area;
}


//--------------------------------------------------------------------------------------------------------
//
// Given a vector of CNavAreas (or derived types), 'inVector', populate 'outVector' with a randomly shuffled set
// of 'maxCount' areas that are at least 'minSeparation' travel distance apart from each other.
//
template< typename T >
void SelectSeparatedShuffleSet( int maxCount, float minSeparation, const CUtlVector< T * > &inVector, CUtlVector< T * > *outVector )
{
	if ( !outVector )
		return;

	outVector->RemoveAll();

	CUtlVector< T * > shuffledVector;

	int i, j;

	for( i=0; i<inVector.Count(); ++i )
	{
		shuffledVector.AddToTail( inVector[i] );
	}

	// randomly shuffle the order
	int n = shuffledVector.Count();
	while( n > 1 )
	{
		int k = randomgen->GetRandomInt<int>(0, n - 1);
		n--;

		T *tmp = shuffledVector[n];
		shuffledVector[n] = shuffledVector[k];
		shuffledVector[k] = tmp;
	}

	// enforce minSeparation between shuffled areas
	for( i=0; i<shuffledVector.Count(); ++i )
	{
		T *area = shuffledVector[i];

		CUtlVector< CNavArea * > nearVector;
		CollectSurroundingAreas( &nearVector, area, minSeparation, 2.0f * navgenparams->step_height, 2.0f * navgenparams->step_height );

		for( j=0; j<i; ++j )
		{
			if ( nearVector.HasElement( (CNavArea *)shuffledVector[j] ) )
			{
				// this area is too near an area earlier in the vector
				break;
			}
		}

		if ( j == i )
		{
			// separated from all prior areas
			outVector->AddToTail( area );

			if ( outVector->Count() >= maxCount )
				return;
		}
	}
}

template <typename T>
class INavSearchNode
{
public:
	INavSearchNode()
	{
		parent = nullptr;
		child = nullptr;
		me = nullptr;
		how = NUM_TRAVERSE_TYPES;
		cost = 0.0f;
		total = 0.0f;
	}

	INavSearchNode(T* myself)
	{
		parent = nullptr;
		child = nullptr;
		me = myself;
		how = NUM_TRAVERSE_TYPES;
		cost = 0.0f;
		total = 0.0f;
	}

	inline T* GetArea() const { return this->me; }
	// Returns the travel cost to reach this area from the parent area.
	inline const float GetTravelCostFromParent() const { return this->cost; }
	// Returns the total travel cost to reach this area from the start.
	inline const float GetTravelCostFromStart() const { return this->total; }

	T* parent;
	T* child;
	T* me;
	NavTraverseType how;
	float cost; // cost to reach this from parent area
	float total; // cost to reach this from start
};

template <typename T>
class NavSearchNodeSmallestCost
{
public:

	bool operator()(const INavSearchNode<T>* lhs, const INavSearchNode<T>* rhs)
	{
		return lhs->total > rhs->total;
	}

	bool operator()(const INavSearchNode<T>& lhs, const INavSearchNode<T>& rhs)
	{
		return lhs.total > rhs.total;
	}
};

template <typename T>
class INavAreaCollector
{
public:
	INavAreaCollector()
	{
		m_startArea = nullptr;
		m_travelLimit = 999999.0f;
		m_nodes.reserve(4096);
		m_collected.reserve(4096);
		m_searchLadders = true;
		m_searchLinks = true;
		m_searchElevators = true;
		m_endSearch = false;
	}

	INavAreaCollector(T* start, const float limit = 999999.0f)
	{
		m_startArea = start;
		m_travelLimit = limit;
		m_nodes.reserve(4096);
		m_collected.reserve(4096);
		m_searchLadders = true;
		m_searchLinks = true;
		m_searchElevators = true;
		m_endSearch = false;
	}

	INavAreaCollector(T* start, const float limit, const bool searchLadders, const bool searchLinks, const bool searchElevators)
	{
		m_startArea = start;
		m_travelLimit = limit;
		m_nodes.reserve(4096);
		m_collected.reserve(4096);
		m_searchLadders = searchLadders;
		m_searchLinks = searchLinks;
		m_searchElevators = searchElevators;
		m_endSearch = false;
	}

	virtual ~INavAreaCollector() {}

	void SetStartArea(T* start) { m_startArea = start; }
	void SetTravelLimit(float limit) { m_travelLimit = limit; }
	T* GetStartArea() const { return m_startArea; }
	float GetTravelLimit() const { return m_travelLimit; }

	void SetSearchLadders(const bool search) { m_searchLadders = search; }
	void SetSearchLinks(const bool search) { m_searchLinks = search; }
	void SetSearchElevators(const bool search) { m_searchElevators = search; }
	bool CanSearchLadders() const { return m_searchLadders; }
	bool CanSearchLinks() const { return m_searchLinks; }
	bool CanSearchElevators() const { return m_searchElevators; }

	// Execute the search
	void Execute();

	// Checks if the given area should be searched
	virtual bool ShouldSearch(T* area) { return true; }
	// Checks if the given area should be collected
	virtual bool ShouldCollect(T* area) { return true; }
	// Calcutes the cost to travel from prevArea to NextArea
	virtual float ComputeCostBetweenAreas(T* prevArea, T* nextArea);
	// Given an area, searches connected areas
	virtual void SearchAdjacentAreas(T* area);
	// Called when the collector has ended collecting areas
	virtual void OnDone() {}
	// Resets and cleans up for a new search
	void Reset();

	const std::vector<T*>& GetCollectedAreas() const { return m_collected; }
	bool IsCollectedAreasEmpty() const { return m_collected.empty(); }
	size_t GetCollectedAreasCount() const { return m_collected.size(); }
	// Returns a random collected area. No filtering.
	T* GetRandomCollectedArea() const { return m_collected[randomgen->GetRandomInt<size_t>(0, m_collected.size() - 1)]; }

	/**
	 * @brief Gets a random collected area
	 * @tparam F Filter function
	 * @param functor Filter function, params: (T* area). Return true to add the area to the output vector, false to don't add it.
	 * @return A random area from the output vector.
	 */
	template<typename F>
	T* GetRandomCollectedArea(F functor) const;
	// Gets the NavSearchNode for the given area. NULL if not found.
	const INavSearchNode<T>* GetNodeForArea(T* area) const;
	// Forces the search to end early
	void EndSearch() { m_endSearch = true; }

protected:

	// Adds an area to search list, pass the previous area
	void IncludeInSearch(T* prevArea, T* area);

	bool ShouldEndSearchEarly() const { return m_endSearch; }

private:
	T* m_startArea;
	float m_travelLimit;
	std::priority_queue<T*, std::vector<T*>, NavSearchNodeSmallestCost<T>> m_searchlist;
	std::unordered_map<unsigned int, INavSearchNode<T>> m_nodes;
	std::vector<T*> m_collected;
	bool m_searchLadders;
	bool m_searchLinks;
	bool m_searchElevators;
	bool m_endSearch;

	void InitSearch();

	T* PopNextArea()
	{
		T* next = m_searchlist.top();
		m_searchlist.pop();
		return next;
	}
};

template<typename T>
inline void INavAreaCollector<T>::Execute()
{
	if (m_startArea == nullptr)
	{
		return;
	}

	InitSearch();

	while (!m_searchlist.empty())
	{
		T* next = PopNextArea();

		// All areas at the searchlist should have a valid node!
		auto thisnode = &m_nodes.find(next->GetID())->second;

		if (m_travelLimit > 0.0f && thisnode->total > m_travelLimit)
		{
			// over travel limit, skip this one. Keep searching the list as it might still have areas within the search limit
			continue;
		}

		// Collect if allowed
		if (ShouldCollect(next))
		{
			m_collected.push_back(next);
		}

		// Ask if we should search this area
		if (ShouldSearch(next))
		{
			// Search adjacent areas and add them to the search list if we never searched them
			SearchAdjacentAreas(next);
		}

		if (ShouldEndSearchEarly())
		{
			break;
		}
	}

	OnDone();
}

template<typename T>
inline float INavAreaCollector<T>::ComputeCostBetweenAreas(T* prevArea, T* nextArea)
{
	if (prevArea == nullptr)
	{
		return 0.0f;
	}

	auto link = prevArea->GetOffMeshConnectionToArea(nextArea);

	if (link != nullptr)
	{
		// if connected via link, always use the precomputed link cost
		return link->GetConnectionLength();
	}

	auto ladder = prevArea->GetLadderConnectionToArea(nextArea);

	if (ladder != nullptr)
	{
		// connected by ladder
		return ladder->m_length;
	}

	// normal connection
	float length = (nextArea->GetCenter() - prevArea->GetCenter()).Length();

	return length;
}

template<typename T>
inline void INavAreaCollector<T>::SearchAdjacentAreas(T* area)
{
	// Loop standard connections
	for (int dir = 0; dir < static_cast<int>(NUM_DIRECTIONS); dir++)
	{
		const int count = area->GetAdjacentCount(static_cast<NavDirType>(dir));

		for (int i = 0; i < count; i++)
		{
			T* other = static_cast<T*>(area->GetAdjacentArea(static_cast<NavDirType>(dir), i));
			IncludeInSearch(area, other);
		}
	}

	// ladders
	
	if (CanSearchLadders())
	{
		auto upconns = area->GetLadders(CNavLadder::LADDER_UP);

		for (int it = 0; it < upconns->Count(); it++)
		{
			auto& connect = upconns->Element(it);
			const CNavLadder* ladder = connect.ladder;

			for (auto& ladderconn : ladder->GetConnections())
			{
				if (ladderconn.IsConnectedToLadderTop())
				{
					T* other = static_cast<T*>(ladderconn.GetConnectedArea());
					IncludeInSearch(area, other);
				}
			}
		}

		auto downconns = area->GetLadders(CNavLadder::LADDER_DOWN);

		for (int it = 0; it < downconns->Count(); it++)
		{
			auto& connect = downconns->Element(it);
			const CNavLadder* ladder = connect.ladder;

			for (auto& ladderconn : ladder->GetConnections())
			{
				if (ladderconn.IsConnectedToLadderBottom())
				{
					T* other = static_cast<T*>(ladderconn.GetConnectedArea());
					IncludeInSearch(area, other);
				}
			}
		}
	}

	// Special links

	if (CanSearchLinks())
	{
		auto& links = area->GetOffMeshConnections();
		for (auto& link : links)
		{
			T* other = static_cast<T*>(link.m_link.area);
			IncludeInSearch(area, other);
		}
	}

	// Elevators

	if (CanSearchElevators())
	{
		const CNavElevator* elevator = area->GetElevator();
		if (elevator)
		{
			auto& floors = elevator->GetFloors();

			for (auto& floor : floors)
			{
				T* other = static_cast<T*>(floor.GetArea());

				if (other == area)
				{
					continue;
				}

				IncludeInSearch(area, other);
			}
		}
	}
}

template<typename T>
inline void INavAreaCollector<T>::Reset()
{
	m_collected.clear();
	m_nodes.clear();

	while (!m_searchlist.empty())
	{
		m_searchlist.pop();
	}
}

template<typename T>
inline const INavSearchNode<T>* INavAreaCollector<T>::GetNodeForArea(T* area) const
{
	auto it = m_nodes.find(area->GetID());

	if (it == m_nodes.end())
	{
		return nullptr;
	}

	return &it->second;
}

template<typename T>
inline void INavAreaCollector<T>::IncludeInSearch(T* prevArea, T* area)
{
	if (m_nodes.find(area->GetID()) == m_nodes.end())
	{
		auto status = m_nodes.emplace(area->GetID(), INavSearchNode<T>(area));
		
		if (status.second == true)
		{
			auto node = &status.first->second;
			auto prevnode = &m_nodes.find(prevArea->GetID())->second;

			node->me = area;
			node->parent = prevArea;
			float distAlong = prevnode->total;
			float cost = ComputeCostBetweenAreas(prevArea, area);
			node->cost = cost; // store the cost to go from parent to me
			node->total = distAlong + cost; // store total cost along the path

			m_searchlist.push(area);
		}
	}
}

template<typename T>
inline void INavAreaCollector<T>::InitSearch()
{
	m_endSearch = false;
	auto status = m_nodes.emplace(m_startArea->GetID(), INavSearchNode<T>(m_startArea));

	if (status.second == true)
	{
		auto node = &status.first->second;

		node->parent = nullptr;
		node->me = m_startArea;
		node->cost = ComputeCostBetweenAreas(nullptr, m_startArea);
		node->total = node->cost;

		m_searchlist.push(m_startArea);
	}
}

template<typename T>
template<typename F>
inline T* INavAreaCollector<T>::GetRandomCollectedArea(F functor) const
{
	std::vector<T*> out;
	out.reserve(m_collected.size());

	for (auto area : m_collected)
	{
		if (functor(area))
		{
			out.push_back(area);
		}
	}

	return out[randomgen->GetRandomInt<size_t>(0, out.size() - 1)];
}

class NavAStarHeuristicCost
{
public:

	// Calculates a heuristic cost between two areas
	float operator()(CNavArea* from, CNavArea* goal) const
	{
		return (from->GetCenter() - goal->GetCenter()).Length();
	}

	// Calculates a heuristic cost between an area and a goal position
	float operator()(CNavArea* from, Vector goal) const
	{
		return (from->GetCenter() - goal).Length();
	}
};

class NavAStarPathCost
{
public:
	/**
	 * @brief This function calculates the travel cost to go from 'fromArea' to 'area'.
	 * @param area Destination area.
	 * @param fromArea Start area.
	 * @param ladder If the area is reached via ladder, this is the ladder.
	 * @param link If the area is reached via Off-Mesh connection, this is it.
	 * @param elevator If the are ais reached via elevator, this is the elevator.
	 * @return Travel cost
	 */
	float operator() (CNavArea* area, CNavArea* fromArea, const CNavLadder* ladder, const NavOffMeshConnection* link, const CNavElevator* elevator) const
	{
		if (fromArea == nullptr)
		{
			return 0.0f; // null from, this is the first area being searched
		}

		float distance = 0.0f;
		
		if (link != nullptr)
		{
			distance = link->GetConnectionLength();
		}
		else if (ladder != nullptr)
		{
			distance = ladder->m_length;
		}
		else if (elevator != nullptr)
		{
			distance = elevator->GetLengthBetweenFloors(fromArea, area);
		}
		else
		{
			distance = (area->GetCenter() - fromArea->GetCenter()).Length();
		}

		return distance;
	}
};

class NavAStarNode
{
public:
	enum class NodeStatus : std::uint8_t
	{
		NEW = 0, // new node
		OPEN,
		CLOSED,
	};

	NavAStarNode()
	{
		area = nullptr;
		g = 0.0f;
		h = 0.0f;
		status = NodeStatus::NEW;
		parent = nullptr;
	}

	template <typename T = CNavArea>
	NavAStarNode(T* pArea)
	{
		area = static_cast<CNavArea*>(pArea);
		g = 0.0f;
		h = 0.0f;
		status = NodeStatus::NEW;
		parent = nullptr;
	}

	void SetArea(CNavArea* pArea) { area = pArea; }
	bool IsOpen() const { return status == NodeStatus::OPEN; }
	bool IsClosed() const { return status == NodeStatus::CLOSED; }
	bool IsStatusUndefined() const { return status == NodeStatus::NEW; }
	void Close() { status = NodeStatus::CLOSED; }
	void Open() { status = NodeStatus::OPEN; }
	float GetGCost() const { return g; }
	float GetHCost() const { return h; }
	float GetFCost() const { return g + h; }

	CNavArea* area;
	float g; // g cost
	float h; // h cost
	NodeStatus status;
	NavAStarNode* parent; // parent node
};

class NavNodeSmallestCost
{
public:

	/* std::greater implementation, for searching the node with the smallest total (f) cost */

	bool operator()(const NavAStarNode* lhs, const NavAStarNode* rhs)
	{
		if (lhs->GetFCost() > rhs->GetFCost())
		{
			return true;
		}
		else if (lhs->GetFCost() == rhs->GetFCost())
		{
			return lhs->GetHCost() > rhs->GetHCost();
		}
		else
		{
			return false;
		}
	}

	bool operator()(const NavAStarNode& lhs, const NavAStarNode& rhs)
	{
		if (lhs.GetFCost() > rhs.GetFCost())
		{
			return true;
		}
		else if (lhs.GetFCost() == rhs.GetFCost())
		{
			return lhs.GetHCost() > rhs.GetHCost();
		}
		else
		{
			return false;
		}
	}
};

template <typename T = CNavArea>
class INavAStarSearch
{
public:
	INavAStarSearch();
	virtual ~INavAStarSearch();

	void SetStart(T* area) { startArea = area; }
	void SetGoalArea(T* area) { goalArea = area; }
	void SetGoalPosition(const Vector& pos) { goalPosition = pos; goalArea = nullptr; }

	template <typename CF, typename HF>
	void DoSearch(CF& gCostFunctor, HF& hCostFunctor);
	void Clear();
	bool FoundPath() const { return lastResult; }
	const std::vector<CNavArea*>& GetPath() const { return path; }

	virtual void OnSuccess() {}
	virtual void OnFailure() {}
	// Total path cost
	const float GetTotalCost() const { return totalCost; }

private:
	T* startArea;
	T* goalArea;
	Vector goalPosition;
	bool lastResult;
	float totalCost;

	NavAStarNode* GetNodeForArea(T* area);
	void BuildPath(NavAStarNode* endnode);

	std::unordered_map<unsigned int, NavAStarNode> nodes;
	std::vector<CNavArea*> path;
};

template<typename T>
inline INavAStarSearch<T>::INavAStarSearch()
{
	startArea = nullptr;
	goalArea = nullptr;
	goalPosition.Init(0.0f, 0.0f, 0.0f);
	lastResult = false;
	totalCost = 0.0f;
	nodes.reserve(1024);
}

template<typename T>
inline INavAStarSearch<T>::~INavAStarSearch()
{
}

template<typename T>
inline NavAStarNode* INavAStarSearch<T>::GetNodeForArea(T* area)
{
	unsigned int id = area->GetID();

	auto it = nodes.find(id);

	if (it == nodes.end())
	{
		auto node = nodes.emplace(id, area);

		if (node.second)
		{
			return &node.first->second;
		}

		return nullptr;
	}

	return &it->second;
}

template<typename T>
template<typename CF, typename HF>
inline void INavAStarSearch<T>::DoSearch(CF& gCostFunctor, HF& hCostFunctor)
{
	std::priority_queue<NavAStarNode*, std::vector<NavAStarNode*>, NavNodeSmallestCost> openList;
	Vector searchGoal = goalArea != nullptr ? goalArea->GetCenter() : goalPosition;
	std::vector<CNavArea*> neighborAreas;
	CNavArea* endArea = goalArea;
	NavAStarNode* closest = nullptr;

	if (endArea == nullptr)
	{
		endArea = TheNavMesh->GetNearestNavArea(searchGoal, 1024.0f);

		if (endArea == nullptr)
		{
			// fail
			this->lastResult = false;
			OnFailure();
			return;
		}
	}

	// initialize
	{
		NavAStarNode* startNode = GetNodeForArea(startArea);

		startNode->g = gCostFunctor(startNode->area, nullptr, nullptr, nullptr, nullptr);
		startNode->h = hCostFunctor(startNode->area, searchGoal);
		startNode->parent = nullptr;
		startNode->Open();

		if (startNode->GetGCost() < 0.0f)
		{
			// fail
			this->lastResult = false;
			OnFailure();
			return;
		}

		openList.push(startNode);
	}

	// search loop
	while (!openList.empty())
	{
		neighborAreas.clear();
		NavAStarNode* current = openList.top();
		closest = current;
		CNavArea* area = current->area;
		openList.pop(); // remove current from openList
		current->Close(); // mark as closed

		if (current->area == endArea)
		{
			// reached
			this->lastResult = true;
			this->totalCost = current->GetGCost();
			BuildPath(current);
			OnSuccess();
			return;
		}

		// Collect neighbors
		auto collectneighborsfunc = [&neighborAreas](CNavArea* other) {
			neighborAreas.push_back(other);
		};
		area->ForEachConnectedArea(collectneighborsfunc);

		for (auto neighbor : neighborAreas)
		{
			NavAStarNode* node = GetNodeForArea(neighbor);

			if (node->IsClosed())
			{
				continue; // skip closed nodes
			}

			const NavOffMeshConnection* offmeshlink = area->GetOffMeshConnectionToArea(neighbor);
			const CNavLadder* ladder = area->GetLadderConnectionToArea(neighbor);
			const CNavElevator* elevator = area->GetElevatorConnectionToArea(neighbor);

			float costf = gCostFunctor(neighbor, area, ladder, offmeshlink, elevator);

			if (costf < 0.0f)
			{
				// dead end
				node->Close();
				continue;
			}

			float newCost = current->GetGCost() + costf;

			if (newCost < node->GetGCost() || node->IsStatusUndefined())
			{
				node->g = newCost;
				node->h = hCostFunctor(neighbor, searchGoal);
				node->parent = current;

				if (node->IsStatusUndefined())
				{
					node->Open();
					openList.push(node);
				}
			}
		}
	}

	this->lastResult = false;

	if (closest != nullptr)
	{
		BuildPath(closest);
	}

	OnFailure();
}

template<typename T>
inline void INavAStarSearch<T>::Clear()
{
	this->path.clear();
	this->nodes.clear();
	this->lastResult = false;
}

template<typename T>
inline void INavAStarSearch<T>::BuildPath(NavAStarNode* endnode)
{
#ifdef EXT_DEBUG
	size_t i = 0;
#endif

	NavAStarNode* next = endnode;
	path.reserve(nodes.size());

	for (;;)
	{
		this->path.push_back(next->area);

		if (next->area == this->startArea)
		{
			return;
		}

		next = next->parent;

#ifdef EXT_DEBUG
		if (++i >= (nodes.size() + 10U))
		{
			throw std::runtime_error("Infinite Loop!");
		}
#endif

		if (next == nullptr)
		{
			return;
		}
	}
}

template <typename T = CNavArea>
class INavFloodFill
{
public:
	INavFloodFill()
	{
		m_startArea = nullptr;
		m_travelLimit = -1.0f;
		m_searchedAreas.reserve(4096);
		m_travelCost.reserve(4096);
		m_parents.reserve(4096);
		m_searchLadders = true;
		m_searchOffmeshLinks = true;
		m_searchElevators = true;
	}

	INavFloodFill(T* start)
	{
		m_startArea = start;
		m_travelLimit = -1.0f;
		m_searchedAreas.reserve(4096);
		m_travelCost.reserve(4096);
		m_parents.reserve(4096);
		m_searchLadders = true;
		m_searchOffmeshLinks = true;
		m_searchElevators = true;
	}

	INavFloodFill(T* start, float travelLimit)
	{
		m_startArea = start;
		m_travelLimit = travelLimit;
		m_searchedAreas.reserve(4096);
		m_travelCost.reserve(4096);
		m_parents.reserve(4096);
		m_searchLadders = true;
		m_searchOffmeshLinks = true;
		m_searchElevators = true;
	}

	INavFloodFill(T* start, float travelLimit, bool searchLadders, bool searchOffMeshLinks, bool searchElevators)
	{
		m_startArea = start;
		m_travelLimit = travelLimit;
		m_searchedAreas.reserve(4096);
		m_travelCost.reserve(4096);
		m_parents.reserve(4096);
		m_searchLadders = searchLadders;
		m_searchOffmeshLinks = searchOffMeshLinks;
		m_searchElevators = searchElevators;
	}

	virtual ~INavFloodFill() {}

	// Reset for a new search
	void Reset()
	{
		m_searchedAreas.clear();
		m_travelCost.clear();
		m_parents.clear();

		while (!m_searchAreas.empty())
		{
			m_searchAreas.pop();
		}
	}

	void SetStartArea(T* area) { m_startArea = area; }
	void SetTravelLimit(float limit) { m_travelLimit = limit; }
	void SetSearchLadders(bool search) { m_searchLadders = search; }
	void SetSearchOffmeshLinks(bool search) { m_searchOffmeshLinks = search; }
	void SetSearchElevators(bool search) { m_searchElevators = search; }

	void Execute();

	/**
	 * @brief Called to each area in the flood fill.
	 * @param area Current area.
	 * @param parent Parent to area. NULL if it's the first area.
	 * @param parentCost Travel cost of the parent area.
	 */
	virtual void operator()(T* area, T* parent, const float parentCost) {}

	/**
	 * @brief Called to calculate the travel cost between from and to areas.
	 * @param toArea Destination area.
	 * @param fromArea Parent area.
	 * @param fromCost Cost of parent area.
	 * @param link Off-Mesh link if any.
	 * @param ladder Ladder if any.
	 * @param elevator Elevator if any.
	 * @return cost to go from parent area to destination area.
	 */
	virtual float operator()(T* toArea, T* fromArea, const float fromCost, const NavOffMeshConnection* link, const CNavLadder* ladder, const CNavElevator* elevator);

private:
	T* m_startArea; // flood start area
	float m_travelLimit;
	std::unordered_set<unsigned int> m_searchedAreas;
	std::unordered_map<unsigned int, float> m_travelCost;
	std::unordered_map<unsigned int, T*> m_parents;
	std::queue<T*> m_searchAreas;
	bool m_searchLadders;
	bool m_searchOffmeshLinks;
	bool m_searchElevators;

	bool AlreadySearched(T* area)
	{
		return m_searchedAreas.find(area->GetID()) != m_searchedAreas.end();
	}

	T* GetParentForArea(T* area)
	{
		return m_parents[area->GetID()];
	}

	void OnAreaBeingSearched(T* area, T* parent, const float parentCost, const NavOffMeshConnection* offmeshlink = nullptr, const CNavLadder* ladder = nullptr, const CNavElevator* elevator = nullptr);
};

template<typename T>
inline void INavFloodFill<T>::Execute()
{
	if (m_startArea == nullptr)
	{
		return;
	}

	m_searchAreas.push(m_startArea);
	m_searchedAreas.insert(m_startArea->GetID());
	m_travelCost[m_startArea->GetID()] = this->operator()(m_startArea, nullptr, 0.0f, nullptr, nullptr, nullptr);
	m_parents[m_startArea->GetID()] = nullptr;


	while (!m_searchAreas.empty())
	{
		T* nextArea = m_searchAreas.front();
		unsigned int nextID = nextArea->GetID();
		m_searchAreas.pop();
		T* parentArea = GetParentForArea(nextArea);
		float currentCost = m_travelCost[nextArea->GetID()];

		if (m_travelLimit > 0.0f && currentCost > m_travelLimit)
		{
			continue; // do not search this area, over travel cost limit
		}

		if (parentArea == nullptr)
		{
			this->operator()(nextArea, nullptr, 0.0f);
		}
		else
		{
			float cost = m_travelCost[parentArea->GetID()];
			this->operator()(nextArea, parentArea, cost);
		}

		// search normal connections
		for (int dir = 0; dir < static_cast<int>(NUM_DIRECTIONS); dir++)
		{
			auto vec = nextArea->GetAdjacentAreas(static_cast<NavDirType>(dir));

			for (int i = 0; i < vec->Count(); i++)
			{
				T* other = static_cast<T*>(vec->Element(i).area);

				if (!AlreadySearched(other))
				{
					OnAreaBeingSearched(other, nextArea, currentCost);
				}
			}
		}

		if (m_searchLadders)
		{
			// search ladders
			for (int dir = 0; dir < static_cast<int>(CNavLadder::NUM_LADDER_DIRECTIONS); dir++)
			{
				auto vec = nextArea->GetLadders(static_cast<CNavLadder::LadderDirectionType>(dir));

				for (int i = 0; i < vec->Count(); i++)
				{
					CNavLadder* ladder = vec->Element(i).ladder;
					auto& conns = ladder->GetConnections();

					for (auto& connect : conns)
					{
						T* other = static_cast<T*>(connect.GetConnectedArea());

						if (other != nextArea && !AlreadySearched(other))
						{
							OnAreaBeingSearched(other, nextArea, currentCost, nullptr, ladder, nullptr);
						}
					}
				}
			}
		}

		if (m_searchOffmeshLinks)
		{
			// search offmesh links
			auto& offmeshlinks = nextArea->GetOffMeshConnections();

			for (auto& link : offmeshlinks)
			{
				T* other = static_cast<T*>(link.m_link.area);

				if (!AlreadySearched(other))
				{
					OnAreaBeingSearched(other, nextArea, currentCost, &link, nullptr, nullptr);
				}
			}
		}

		if (m_searchElevators)
		{
			const CNavElevator* elevator = nextArea->GetElevator();

			// search elevators
			if (elevator != nullptr)
			{
				auto& floors = elevator->GetFloors();

				for (auto& floor : floors)
				{
					T* other = static_cast<T*>(floor.GetArea());

					if (other != nextArea && !AlreadySearched(other))
					{
						OnAreaBeingSearched(other, nextArea, currentCost, nullptr, nullptr, elevator);
					}
				}
			}
		}
	}
}

template<typename T>
inline float INavFloodFill<T>::operator()(T* toArea, T* fromArea, const float fromCost, const NavOffMeshConnection* link, const CNavLadder* ladder, const CNavElevator* elevator)
{
	if (fromArea == nullptr)
	{
		return 0.0f;
	}
	else
	{
		float dist = 0.0f;

		if (link != nullptr)
		{
			dist = link->GetConnectionLength();
		}
		else if (ladder != nullptr)
		{
			dist = ladder->m_length;
		}
		else if (elevator != nullptr)
		{
			dist = elevator->GetLengthBetweenFloors(static_cast<CNavArea*>(fromArea), static_cast<CNavArea*>(toArea));
		}
		else
		{
			dist = (fromArea->GetCenter() - toArea->GetCenter()).Length();
		}

		return fromCost + dist;
	}
}

template<typename T>
inline void INavFloodFill<T>::OnAreaBeingSearched(T* area, T* parent, const float parentCost, const NavOffMeshConnection* offmeshlink, const CNavLadder* ladder, const CNavElevator* elevator)
{
	m_searchedAreas.insert(area->GetID());
	m_travelCost[area->GetID()] = this->operator()(area, parent, parentCost, offmeshlink, ladder, elevator);
	m_parents[area->GetID()] = parent;
	m_searchAreas.push(area);
}

#endif // _NAV_PATHFIND_H_