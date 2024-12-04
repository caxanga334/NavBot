//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
// nav_area.h
// Navigation areas
// Author: Michael S. Booth (mike@turtlerockstudios.com), January 2003

#ifndef _NAV_AREA_H_
#define _NAV_AREA_H_

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <string>
#include <cstdint>
#include <vector>
#include <array>
#include <cmath>

#include "nav_ladder.h"
#include <sdkports/sdk_timers.h>
#include <shareddefs.h>
#include <networkvar.h>

// BOTPORT: Clean up relationship between team index and danger storage in nav areas
constexpr auto MAX_NAV_TEAMS = 2;


#ifdef STAGING_ONLY
inline void DebuggerBreakOnNaN_StagingOnly( float val )
{
	if ( IS_NAN( val ) )
		DebuggerBreak();
}
#else
#define DebuggerBreakOnNaN_StagingOnly( _val )
#endif

class CFuncElevator;
class CFuncNavCost;
// class KeyValues;

inline bool FStrEq(const char *sz1, const char *sz2)
{
	return ( sz1 == sz2 || V_stricmp(sz1, sz2) == 0 );
}

template < typename Functor >
bool ForEachActor( Functor &func );

bool UTIL_IsCommandIssuedByServerAdmin();

const char *UTIL_VarArgs( const char *format, ... );

//-------------------------------------------------------------------------------------------------------------------
/**
 * Functor interface for iteration
 */
class IForEachNavArea
{
public:
	virtual bool Inspect( const CNavArea *area ) = 0;				// Invoked once on each area of the iterated set. Return false to stop iterating.
	virtual void PostIteration( bool wasCompleteIteration ) { }		// Invoked after the iteration has ended. 'wasCompleteIteration' will be true if the entire set was iterated (ie: Inspect() never returned false)
};


//-------------------------------------------------------------------------------------------------------------------
/**
 * The NavConnect union is used to refer to connections to areas
 */
struct NavConnect
{
	NavConnect()
	{
		id = 0;
		length = -1;
	}

	union
	{
		unsigned int id;
		CNavArea *area;
	};

	mutable float length;

	bool operator==( const NavConnect &other ) const
	{
		return area == other.area;
	}
};

//-------------------------------------------------------------------------------------------------------------------
/**
 * The NavLadderConnect union is used to refer to connections to ladders
 */
union NavLadderConnect
{
	unsigned int id;
	CNavLadder *ladder;

	bool operator==( const NavLadderConnect &other ) const
	{
		return ladder == other.ladder;
	}
};

//--------------------------------------------------------------------------------------------------------------
/**
 * A HidingSpot is a good place for a bot to crouch and wait for enemies
 */
class HidingSpot
{
public:
	virtual ~HidingSpot()	{ }

	enum 
	{ 
		IN_COVER			= 0x01,							// in a corner with good hard cover nearby
		GOOD_SNIPER_SPOT	= 0x02,							// had at least one decent sniping corridor
		IDEAL_SNIPER_SPOT	= 0x04,							// can see either very far, or a large area, or both
		EXPOSED				= 0x08							// spot in the open, usually on a ledge or cliff
	};

	bool HasGoodCover( void ) const			{ return (m_flags & IN_COVER) ? true : false; }	// return true if hiding spot in in cover
	bool IsGoodSniperSpot( void ) const		{ return (m_flags & GOOD_SNIPER_SPOT) ? true : false; }
	bool IsIdealSniperSpot( void ) const	{ return (m_flags & IDEAL_SNIPER_SPOT) ? true : false; }
	bool IsExposed( void ) const			{ return (m_flags & EXPOSED) ? true : false; }	

	int GetFlags( void ) const		{ return m_flags; }

	void Save(std::fstream& filestream, uint32_t version);
	void Load(std::fstream& filestream, uint32_t version);
	NavErrorType PostLoad( void );

	const Vector &GetPosition( void ) const		{ return m_pos; }	// get the position of the hiding spot
	unsigned int GetID( void ) const			{ return m_id; }
	const CNavArea *GetArea( void ) const		{ return m_area; }	// return nav area this hiding spot is within

	void Mark( void )							{ m_marker = m_masterMarker; }
	bool IsMarked( void ) const					{ return (m_marker == m_masterMarker) ? true : false; }
	static void ChangeMasterMarker( void )		{ ++m_masterMarker; }


public:
	void SetFlags( int flags )				{ m_flags |= flags; }	// FOR INTERNAL USE ONLY
	void SetPosition( const Vector &pos )	{ m_pos = pos; }		// FOR INTERNAL USE ONLY

private:
	friend class CNavMesh;
	friend void ClassifySniperSpot( HidingSpot *spot );

	HidingSpot( void );										// must use factory to create

	Vector m_pos;											// world coordinates of the spot
	unsigned int m_id;										// this spot's unique ID
	unsigned int m_marker;									// this spot's unique marker
	CNavArea *m_area;										// the nav area containing this hiding spot

	unsigned char m_flags;									// bit flags

	static unsigned int m_nextID;							// used when allocating spot ID's
	static unsigned int m_masterMarker;						// used to mark spots
};
typedef CUtlVector< HidingSpot * > HidingSpotVector;


//--------------------------------------------------------------------------------------------------------------
/**
 * Stores a pointer to an interesting "spot", and a parametric distance along a path
 */
struct SpotOrder
{
	float t;						// parametric distance along ray where this spot first has LOS to our path
	union
	{
		HidingSpot *spot;			// the spot to look at
		unsigned int id;			// spot ID for save/load
	};
};
typedef CUtlVector< SpotOrder > SpotOrderVector;

/**
 * This struct stores possible path segments thru a CNavArea, and the dangerous spots
 * to look at as we traverse that path segment.
 */
struct SpotEncounter
{
	NavConnect from;
	NavDirType fromDir;
	NavConnect to;
	NavDirType toDir;
	Ray path;									// the path segment
	SpotOrderVector spots;						// list of spots to look at, in order of occurrence
};
typedef CUtlVector< SpotEncounter * > SpotEncounterVector;

enum class NavLinkType : uint32_t
{
	LINK_INVALID = 0,
	LINK_GROUND, // solid ground
	LINK_TELEPORTER, // map based teleporter (trigger_teleport)
	LINK_BLAST_JUMP, // Blast/Rocket Jump
	LINK_DOUBLE_JUMP,

	MAX_LINK_TYPES // max known link types
};

/**
 * @brief Link connections between two Nav Areas. "Off-Mesh" connections.
 */
class NavSpecialLink
{
public:
	NavSpecialLink()
	{
		m_type = NavLinkType::LINK_INVALID;
	}

	NavSpecialLink(NavLinkType type, CNavArea* other, const Vector& start, const Vector& end)
	{
		m_type = type;
		m_link.area = other;
		m_start = start;
		m_end = end;
	}

	NavSpecialLink(NavLinkType type, unsigned int id, const Vector& start, const Vector& end)
	{
		m_type = type;
		m_link.id = id;
		m_start = start;
		m_end = end;
	}

	static const char* LinkTypeToString(NavLinkType type);

	inline bool IsOfType(NavLinkType type) const { return m_type == type; }
	inline bool IsConnectedTo(CNavArea* area) const { return m_link.area == area; }
	inline NavLinkType GetType() const { return m_type; }
	inline const Vector& GetStart() const { return m_start; }
	inline const Vector& GetEnd() const { return m_end; }
	inline const CNavArea* GetConnectedArea() const { return m_link.area; }
	inline void SetArea(CNavArea* area) { m_link.area = area; }
	inline float GetConnectionLength() const { return m_link.length; }

	bool operator==(const NavSpecialLink& other) const
	{
		return other.IsOfType(m_type) && other.m_link == m_link;
	}

	NavLinkType m_type; // Link type
	NavConnect m_link; // Link connection
	Vector m_start; // Link start
	Vector m_end; // Link end
};

//-------------------------------------------------------------------------------------------------------------------
/**
 * A CNavArea is a rectangular region defining a walkable area in the environment
 */

class CNavAreaCriticalData
{
protected:
	// --- Begin critical data, which is heavily hit during pathing operations and carefully arranged for cache performance [7/24/2008 tom] ---

	Vector m_nwCorner;											// north-west corner position (2D mins)
	Vector m_seCorner;											// south-east corner position (2D maxs)
	float m_invDxCorners;
	float m_invDyCorners;
	float m_neZ;												// height of the implicit corner defined by (m_seCorner.x, m_nwCorner.y, m_neZ)
	float m_swZ;												// height of the implicit corner defined by (m_nwCorner.x, m_seCorner.y, m_neZ)
	Vector m_center;											// centroid of area
	unsigned int m_marker;										// used to flag the area as visited
	float m_totalCost;											// the distance so far plus an estimate of the distance left
	float m_costSoFar;											// distance travelled so far
	std::array<bool, NAV_TEAMS_ARRAY_SIZE> m_isBlocked;					// Blocked status for each team

	CNavArea *m_nextOpen, *m_prevOpen;							// only valid if m_openMarker == m_masterMarker
	unsigned int m_openMarker;									// if this equals the current marker value, we are on the open list
	int	m_attributeFlags;										// set of attribute bit flags (see NavAttributeType)

	//- connections to adjacent areas -------------------------------------------------------------------
	NavConnectVector m_connect[ NUM_DIRECTIONS ];				// a list of adjacent areas for each direction
	NavLadderConnectVector m_ladder[ CNavLadder::NUM_LADDER_DIRECTIONS ];	// list of ladders leading up and down from this area
	NavConnectVector m_elevatorAreas;							// a list of areas reachable via elevator from this area

	unsigned int m_nearNavSearchMarker;							// used in GetNearestNavArea()

	CNavArea *m_parent;											// the area just prior to this on in the search path
	NavTraverseType m_parentHow;								// how we get from parent to us
	float m_pathLengthSoFar;									// length of path so far, needed for limiting pathfind max path length
	CFuncElevator *m_elevator;									// if non-NULL, this area is in an elevator's path. The elevator can transport us vertically to another area.

	std::vector<NavSpecialLink> m_speciallinks;					// Special 'link' connections
	// --- End critical data --- 
};

class CNavArea : protected CNavAreaCriticalData
{
public:
	DECLARE_CLASS_NOBASE( CNavArea )

	CNavArea(unsigned int place);
	virtual ~CNavArea();
	
	virtual void OnServerActivate( void );						// (EXTEND) invoked when map is initially loaded
	virtual void OnRoundRestart( void );						// (EXTEND) invoked for each area when the round restarts
	virtual void OnRoundRestartPreEntity( void ) { }			// invoked for each area when the round restarts, but before entities are deleted and recreated
	virtual void OnEnter( edict_t *who, CNavArea *areaJustLeft ) { }	// invoked when player enters this area
	virtual void OnExit( edict_t *who, CNavArea *areaJustEntered ) { }	// invoked when player exits this area
	virtual void OnFrame() {} // invoked every server frame
	virtual void OnUpdate() {} // invoked at intervals

	virtual void OnDestroyNotify( CNavArea *dead );				// invoked when given area is going away
	virtual void OnDestroyNotify( CNavLadder *dead );			// invoked when given ladder is going away

	virtual void OnEditCreateNotify( CNavArea *newArea ) { }		// invoked when given area has just been added to the mesh in edit mode
	virtual void OnEditDestroyNotify( CNavArea *deadArea ) { }		// invoked when given area has just been deleted from the mesh in edit mode
	virtual void OnEditDestroyNotify( CNavLadder *deadLadder ) { }	// invoked when given ladder has just been deleted from the mesh in edit mode

	virtual void Save(std::fstream& filestream, uint32_t version);	// (EXTEND)
	virtual NavErrorType Load(std::fstream& filestream, uint32_t version, uint32_t subVersion);		// (EXTEND)
	virtual NavErrorType PostLoad( void );								// (EXTEND) invoked after all areas have been loaded - for pointer binding, etc

	// virtual void SaveToSelectedSet( KeyValues *areaKey ) const;		// (EXTEND) saves attributes for the area to a KeyValues
	// virtual void RestoreFromSelectedSet( KeyValues *areaKey );		// (EXTEND) restores attributes from a KeyValues

	// for interactively building or generating nav areas
	void Build( CNavNode *nwNode, CNavNode *neNode, CNavNode *seNode, CNavNode *swNode );
	void Build( const Vector &corner, const Vector &otherCorner );
	void Build( const Vector &nwCorner, const Vector &neCorner, const Vector &seCorner, const Vector &swCorner );

	void ConnectTo( CNavArea *area, NavDirType dir );			// connect this area to given area in given direction
	void Disconnect( CNavArea *area );							// disconnect this area from given area

	void ConnectTo( CNavLadder *ladder );						// connect this area to given ladder
	void Disconnect( CNavLadder *ladder );						// disconnect this area from given ladder

	bool ConnectTo(CNavArea* area, NavLinkType linktype, const Vector& origin, const Vector& end); // connect via special link
	void Disconnect(CNavArea* area, NavLinkType linktype); // remove special link connection

	unsigned int GetID( void ) const	{ return m_id; }		// return this area's unique ID
	static void CompressIDs( CNavMesh* TheNavMesh );							// re-orders area ID's so they are continuous
	unsigned int GetDebugID( void ) const { return m_debugid; }

	size_t GetSpecialLinkCount() const { return m_speciallinks.size(); }
	const std::vector<NavSpecialLink>& GetSpecialLinks() const { return m_speciallinks; }

	void SetAttributes( int bits )			{ m_attributeFlags = bits; }
	int GetAttributes( void ) const			{ return m_attributeFlags; }
	bool HasAttributes( int bits ) const	{ return ( m_attributeFlags & bits ) ? true : false; }
	void RemoveAttributes( int bits )		{ m_attributeFlags &= ( ~bits ); }

	void SetPlace( Place place )		{ m_place = place; }	// set place descriptor
	Place GetPlace( void ) const		{ return m_place; }		// get place descriptor

	void MarkAsBlocked( int teamID, edict_t *blocker, bool bGenerateEvent = true );	// An entity can force a nav area to be blocked
	virtual void UpdateBlocked( bool force = false, int teamID = NAV_TEAM_ANY );		// Updates the (un)blocked status of the nav area (throttled)
	virtual bool IsBlocked( int teamID, bool ignoreNavBlockers = false ) const;
	void UnblockArea( int teamID = NAV_TEAM_ANY );					// clear blocked status for the given team(s)

protected:
	inline void SetBlocked(bool blocked, int teamID = NAV_TEAM_ANY)
	{
		if (teamID < 0 || teamID >= static_cast<int>(m_isBlocked.size()))
		{
			for (auto& b : m_isBlocked)
			{
				b = blocked;
			}
		}
		else
		{
			m_isBlocked[teamID] = blocked;
		}
	}
public:

	void CheckFloor( edict_t *ignore );						// Checks if there is a floor under the nav area, in case a breakable floor is gone
	// Checks if there is a solid floor on this nav area
	bool HasSolidFloor() const;
	bool HasSolidObstruction() const;

	void MarkObstacleToAvoid( float obstructionHeight );
	void UpdateAvoidanceObstacles( void );
	bool HasAvoidanceObstacle( float maxObstructionHeight = navgenparams->step_height ) const; // is there a large, immobile object obstructing this area
	float GetAvoidanceObstacleHeight( void ) const; // returns the maximum height of the obstruction above the ground

#ifdef NEXT_BOT
	bool HasPrerequisite( CBaseCombatCharacter *actor = NULL ) const;							// return true if this area has a prerequisite that applies to the given actor
	const CUtlVector< CHandle< CFuncNavPrerequisite > > &GetPrerequisiteVector( void ) const;	// return vector of prerequisites that must be met before this area can be traversed
	void RemoveAllPrerequisites( void );
	void AddPrerequisite( CFuncNavPrerequisite *prereq );
#endif

	void ClearAllNavCostEntities( void );							// clear set of func_nav_cost entities that affect this area
	void AddFuncNavCostEntity( CFuncNavCost *cost );				// add the given func_nav_cost entity to the cost of this area
	float ComputeFuncNavCost( edict_t *who ) const;	// return the cost multiplier of this area's func_nav_cost entities for the given actor
	bool HasFuncNavAvoid( void ) const;
	bool HasFuncNavPrefer( void ) const;

	void CheckWaterLevel( void );
	bool IsUnderwater( void ) const		{ return m_isUnderwater; }

	bool IsOverlapping( const Vector &pos, float tolerance = 0.0f ) const;	// return true if 'pos' is within 2D extents of area.
	bool IsOverlapping( const CNavArea *area ) const;			// return true if 'area' overlaps our 2D extents
	bool IsOverlapping( const Extent &extent ) const;			// return true if 'extent' overlaps our 2D extents
	bool IsOverlappingX( const CNavArea *area ) const;			// return true if 'area' overlaps our X extent
	bool IsOverlappingY( const CNavArea *area ) const;			// return true if 'area' overlaps our Y extent
	inline float GetZ( const Vector * RESTRICT pPos ) const ;			// return Z of area at (x,y) of 'pos'
	inline float GetZ( const Vector &pos ) const;						// return Z of area at (x,y) of 'pos'
	float GetZ( float x, float y ) const RESTRICT;				// return Z of area at (x,y) of 'pos'
	bool Contains( const Vector &pos ) const;					// return true if given point is on or above this area, but no others
	bool Contains( const CNavArea *area ) const;	
	bool IsCoplanar( const CNavArea *area ) const;				// return true if this area and given area are approximately co-planar
	void GetClosestPointOnArea( const Vector * RESTRICT pPos, Vector *close ) const RESTRICT;	// return closest point to 'pos' on this area - returned point in 'close'
	void GetClosestPointOnArea( const Vector &pos, Vector *close ) const { return GetClosestPointOnArea( &pos, close ); }
	float GetDistanceSquaredToPoint( const Vector &pos ) const;	// return shortest distance between point and this area
	bool IsDegenerate( void ) const;							// return true if this area is badly formed
	bool IsRoughlySquare( void ) const;							// return true if this area is approximately square
	bool IsFlat( void ) const;									// return true if this area is approximately flat
	bool HasNodes( void ) const;
	void GetNodes( NavDirType dir, CUtlVector< CNavNode * > *nodes ) const;	// build a vector of nodes along the given direction
	CNavNode *FindClosestNode( const Vector &pos, NavDirType dir ) const;	// returns the closest node along the given edge to the given point

	bool IsContiguous( const CNavArea *other ) const;			// return true if the given area and 'other' share a colinear edge (ie: no drop-down or step/jump/climb)
	float ComputeAdjacentConnectionHeightChange( const CNavArea *destinationArea ) const;			// return height change between edges of adjacent nav areas (not actual underlying ground)
	float ComputeAdjacentConnectionGapDistance(const CNavArea* destinationArea) const; // return the 'gap' distance between edges of adjacent nav areas

	bool IsEdge( NavDirType dir ) const;						// return true if there are no bi-directional links on the given side

	bool IsDamaging( void ) const;								// Return true if continuous damage (ie: fire) is in this area
	void MarkAsDamaging( float duration );						// Mark this area is damaging for the next 'duration' seconds

	bool IsVisible( const Vector &eye, Vector *visSpot = NULL ) const;	// return true if area is visible from the given eyepoint, return visible spot

	int GetAdjacentCount( NavDirType dir ) const	{ return m_connect[ dir ].Count(); }	// return number of connected areas in given direction
	CNavArea *GetAdjacentArea( NavDirType dir, int i ) const;	// return the i'th adjacent area in the given direction
	CNavArea *GetRandomAdjacentArea( NavDirType dir ) const;
	void CollectAdjacentAreas( CUtlVector< CNavArea * > *adjVector ) const;	// build a vector of all adjacent areas

	const NavConnectVector *GetAdjacentAreas( NavDirType dir ) const	{ return &m_connect[dir]; }
	bool IsConnected( const CNavArea *area, NavDirType dir ) const;	// return true if given area is connected in given direction
	bool IsConnected( const CNavLadder *ladder, CNavLadder::LadderDirectionType dir ) const;	// return true if given ladder is connected in given direction
	bool IsConnected(const CNavArea* area, NavLinkType linktype) const; // returns true if the given area is connected via the given link typ
	// true if there is at least 1 form of connection to the other area
	inline bool HasAnyConnectionsTo(const CNavArea* other) const
	{
		for (int dir = static_cast<int>(NORTH); dir < static_cast<int>(NUM_DIRECTIONS); dir++)
		{
			if (IsConnected(other, static_cast<NavDirType>(dir)))
			{
				return true;
			}
		}

		if (IsConnectedToBySpecialLink(other))
		{
			return true;
		}

		return false;
	}

	float ComputeGroundHeightChange( const CNavArea *area ) const;			// compute change in actual ground height from this area to given area

	const NavConnectVector *GetIncomingConnections( NavDirType dir ) const	{ return &m_incomingConnect[dir]; }	// get areas connected TO this area by a ONE-WAY link (ie: we have no connection back to them)
	void AddIncomingConnection( CNavArea *source, NavDirType incomingEdgeDir );

	const NavLadderConnectVector *GetLadders( CNavLadder::LadderDirectionType dir ) const	{ return &m_ladder[dir]; }
	CFuncElevator *GetElevator( void ) const												{ Assert( !( m_attributeFlags & NAV_MESH_HAS_ELEVATOR ) == (m_elevator == NULL) ); return ( m_attributeFlags & NAV_MESH_HAS_ELEVATOR ) ? m_elevator : NULL; }
	const NavConnectVector &GetElevatorAreas( void ) const									{ return m_elevatorAreas; }	// return collection of areas reachable via elevator from this area

	void ComputePortal( const CNavArea *to, NavDirType dir, Vector *center, float *halfWidth ) const;		// compute portal to adjacent area
	NavDirType ComputeLargestPortal( const CNavArea *to, Vector *center, float *halfWidth ) const;		// compute largest portal to adjacent area, returning direction
	void ComputeClosestPointInPortal( const CNavArea *to, NavDirType dir, const Vector &fromPos, Vector *closePos ) const; // compute closest point within the "portal" between to adjacent areas
	NavDirType ComputeDirection( Vector *point ) const;			// return direction from this area to the given point

	//- for hunting algorithm ---------------------------------------------------------------------------
	void SetClearedTimestamp( int teamID );						// set this area's "clear" timestamp to now
	float GetClearedTimestamp( int teamID ) const;				// get time this area was marked "clear"

	//- hiding spots ------------------------------------------------------------------------------------
	const HidingSpotVector *GetHidingSpots( void ) const	{ return &m_hidingSpots; }

	//- "danger" ----------------------------------------------------------------------------------------
	void IncreaseDanger( int teamID, float amount );			// increase the danger of this area for the given team
	float GetDanger( int teamID );								// return the danger of this area (decays over time)
	virtual float GetDangerDecayRate( void ) const;				// return danger decay rate per second

	//- extents -----------------------------------------------------------------------------------------
	float GetSizeX( void ) const			{ return m_seCorner.x - m_nwCorner.x; }
	float GetSizeY( void ) const			{ return m_seCorner.y - m_nwCorner.y; }
	void GetExtent( Extent *extent ) const;						// return a computed extent (XY is in m_nwCorner and m_seCorner, Z is computed)
	const Vector &GetCenter( void ) const	{ return m_center; }
	Vector GetRandomPoint( void ) const;
	Vector GetCorner( NavCornerType corner ) const;
	void SetCorner( NavCornerType corner, const Vector& newPosition );
	void ComputeNormal( Vector *normal, bool alternate = false ) const;	// Computes the area's normal based on m_nwCorner.  If 'alternate' is specified, m_seCorner is used instead.
	void RemoveOrthogonalConnections( NavDirType dir );

	//- occupy time ------------------------------------------------------------------------------------
	float GetEarliestOccupyTime( int teamID ) const;			// returns the minimum time for someone of the given team to reach this spot from their spawn
	bool IsBattlefront( void ) const	{ return m_isBattlefront; }	// true if this area is a "battlefront" - where rushing teams initially meet

	//- lighting ----------------------------------------------------------------------------------------
	float GetLightIntensity( const Vector &pos ) const;			// returns a 0..1 light intensity for the given point
	float GetLightIntensity( float x, float y ) const;			// returns a 0..1 light intensity for the given point
	float GetLightIntensity( void ) const;						// returns a 0..1 light intensity averaged over the whole area

	//- A* pathfinding algorithm ------------------------------------------------------------------------
	static void MakeNewMarker( void )	{ ++m_masterMarker; if (m_masterMarker == 0) m_masterMarker = 1; }
	void Mark( void )					{ m_marker = m_masterMarker; }
	bool IsMarked( void ) const			{ return (m_marker == m_masterMarker) ? true : false; }
	
	void SetParent( CNavArea *parent, NavTraverseType how = NUM_TRAVERSE_TYPES )	{ m_parent = parent; m_parentHow = how; }
	CNavArea *GetParent( void ) const	{ return m_parent; }
	NavTraverseType GetParentHow( void ) const	{ return m_parentHow; }

	bool IsOpen( void ) const;									// true if on "open list"
	void AddToOpenList( void );									// add to open list in decreasing value order
	void AddToOpenListTail( void );								// add to tail of the open list
	void UpdateOnOpenList( void );								// a smaller value has been found, update this area on the open list
	void RemoveFromOpenList( void );
	static bool IsOpenListEmpty( void );
	static CNavArea *PopOpenList( void );						// remove and return the first element of the open list													

	bool IsClosed( void ) const;								// true if on "closed list"
	void AddToClosedList( void );								// add to the closed list
	void RemoveFromClosedList( void );

	static void ClearSearchLists( void );						// clears the open and closed lists for a new search

	void SetTotalCost( float value )	{ DebuggerBreakOnNaN_StagingOnly( value ); Assert( !IS_NAN(value) ); m_totalCost = value; }
	float GetTotalCost( void ) const	{ DebuggerBreakOnNaN_StagingOnly( m_totalCost ); return m_totalCost; }

	void SetCostSoFar( float value )	{ DebuggerBreakOnNaN_StagingOnly( value ); Assert( IS_NAN(value) ); m_costSoFar = value; }
	float GetCostSoFar( void ) const	{ DebuggerBreakOnNaN_StagingOnly( m_costSoFar ); return m_costSoFar; }

	void SetPathLengthSoFar( float value )	{ DebuggerBreakOnNaN_StagingOnly( value ); Assert( !IS_NAN(value) ); m_pathLengthSoFar = value; }
	float GetPathLengthSoFar( void ) const	{ DebuggerBreakOnNaN_StagingOnly( m_pathLengthSoFar ); return m_pathLengthSoFar; }

	//- editing -----------------------------------------------------------------------------------------
	virtual void Draw( void ) const;							// draw area for debugging & editing
	virtual void DrawFilled( int r, int g, int b, int a, float deltaT = 0.1f, bool noDepthTest = true, float margin = 5.0f ) const;	// draw area as a filled rect of the given color
	virtual void DrawSelectedSet( const Vector &shift ) const;	// draw this area as part of a selected set
	void DrawDragSelectionSet( Color &dragSelectionSetColor ) const;
	void DrawConnectedAreas(  CNavMesh* TheNavMesh ) const;
	void DrawHidingSpots( void ) const;
	bool SplitEdit( bool splitAlongX, float splitEdge, CNavArea **outAlpha = NULL, CNavArea **outBeta = NULL );	// split this area into two areas at the given edge
	bool MergeEdit( CNavMesh* TheNavMesh, CNavArea *adj );							// merge this area and given adjacent area
	bool SpliceEdit( CNavArea *other );							// create a new area between this area and given area 
	void RaiseCorner( NavCornerType corner, int amount, bool raiseAdjacentCorners = true );	// raise/lower a corner (or all corners if corner == NUM_CORNERS)
	void PlaceOnGround( NavCornerType corner, float inset = 0.0f );	// places a corner (or all corners if corner == NUM_CORNERS) on the ground
	NavCornerType GetCornerUnderCursor( void ) const;
	bool GetCornerHotspot( NavCornerType corner, Vector hotspot[NUM_CORNERS] ) const;	// returns true if the corner is under the cursor
	void Shift( const Vector &shift );							// shift the nav area

	//- ladders -----------------------------------------------------------------------------------------
	void AddLadderUp( CNavLadder *ladder );
	void AddLadderDown( CNavLadder *ladder );

	//- generation and analysis -------------------------------------------------------------------------
	virtual void ComputeHidingSpots( void );					// analyze local area neighborhood to find "hiding spots" in this area - for map learning
	virtual void ComputeSniperSpots( void );					// analyze local area neighborhood to find "sniper spots" in this area - for map learning
	virtual void ComputeEarliestOccupyTimes( void );
	virtual void CustomAnalysis( bool isIncremental = false ) { }	// for game-specific analysis
	virtual bool ComputeLighting( void );						// compute 0..1 light intensity at corners and center (requires client via listenserver)
	bool TestStairs( void );									// Test an area for being on stairs
	virtual bool IsAbleToMergeWith( CNavArea *other ) const;

	virtual void InheritAttributes( CNavArea *first, CNavArea *second = NULL );

	inline bool IsConnectedToBySpecialLink(const CNavArea* other) const
	{
		for (auto& link : m_speciallinks)
		{
			if (link.GetConnectedArea() == other)
			{
				return true;
			}
		}

		return false;
	}

	const NavSpecialLink* GetSpecialLinkConnectionToArea(const CNavArea* other) const
	{
		for (auto& link : m_speciallinks)
		{
			if (link.GetConnectedArea() == other)
			{
				return &link;
			}
		}

		return nullptr;
	}

	const CNavLadder* GetLadderConnectionToArea(const CNavArea* other) const
	{
		for (int dir = 0; dir < static_cast<int>(CNavLadder::NUM_LADDER_DIRECTIONS); dir++)
		{
			FOR_EACH_VEC(m_ladder[dir], it)
			{
				auto& connect = m_ladder[dir].Element(it);
				auto ladder = connect.ladder;

				if (ladder != nullptr)
				{
					if (ladder->IsConnected(this))
					{
						return ladder;
					}
				}
			}
		}

		return nullptr;
	}

	/**
	 * @brief Runs a function on each nav area connected to this area
	 * @tparam T A class with operator() overload with 1 parameter (CNavArea* connectedArea)
	 * @param functor function to run on each connected area to this area
	 */
	template <typename T>
	inline void ForEachConnectedArea(T functor)
	{
		for (int dir = 0; dir < static_cast<int>(NUM_DIRECTIONS); dir++)
		{
			auto conn = GetAdjacentAreas(static_cast<NavDirType>(dir));

			for (int i = 0; i < conn->Count(); i++)
			{
				CNavArea* connectedArea = conn->Element(i).area;
				functor(connectedArea);
			}
		}

		auto laddersup = GetLadders(CNavLadder::LADDER_UP);

		if (laddersup)
		{
			for (int i = 0; i < laddersup->Count(); i++)
			{
				auto& conn = laddersup->Element(i);

				for (auto& lc : conn.ladder->GetConnections())
				{
					if (lc.IsConnectedToLadderTop())
					{
						functor(lc.GetConnectedArea());
					}
				}				
			}
		}

		auto laddersdown = GetLadders(CNavLadder::LADDER_DOWN);

		if (laddersdown)
		{
			for (int i = 0; i < laddersdown->Count(); i++)
			{
				auto& conn = laddersdown->Element(i);

				for (auto& lc : conn.ladder->GetConnections())
				{
					if (lc.IsConnectedToLadderBottom())
					{
						functor(lc.GetConnectedArea());
					}
				}
			}
		}

		for (auto& link : m_speciallinks)
		{
			CNavArea* connectedArea = link.m_link.area;
			functor(connectedArea);
		}
	}

private:
	friend class CNavMesh;
	friend class CNavLadder;
	friend class CCSNavArea;									// allow CS load code to complete replace our default load behavior

	static bool m_isReset;										// if true, don't bother cleaning up in destructor since everything is going away

	/*
	m_nwCorner
		nw           ne
		 +-----------+
		 | +-->x     |
		 | |         |
		 | v         |
		 | y         |
		 |           |
		 +-----------+
		sw           se
					m_seCorner
	*/

	static unsigned int m_nextID;								// used to allocate unique IDs
	unsigned int m_id;											// unique area ID
	unsigned int m_debugid;

	Place m_place;												// place descriptor

	CountdownTimer m_blockedTimer;								// Throttle checks on our blocked state while blocked
	void UpdateBlockedFromNavBlockers( void );					// checks if nav blockers are still blocking the area

	bool m_isUnderwater;										// true if the center of the area is underwater

	bool m_isBattlefront;

	float m_avoidanceObstacleHeight;							// if nonzero, a prop is obstructing movement through this nav area
	CountdownTimer m_avoidanceObstacleTimer;					// Throttle checks on our obstructed state while obstructed

	//- for hunting -------------------------------------------------------------------------------------
	float m_clearedTimestamp[ MAX_NAV_TEAMS ];					// time this area was last "cleared" of enemies

	//- "danger" ----------------------------------------------------------------------------------------
	float m_danger[ MAX_NAV_TEAMS ];							// danger of this area, allowing bots to avoid areas where they died in the past - zero is no danger
	float m_dangerTimestamp[ MAX_NAV_TEAMS ];					// time when danger value was set - used for decaying
	void DecayDanger( void );

	//- hiding spots ------------------------------------------------------------------------------------
	HidingSpotVector m_hidingSpots;
	bool IsHidingSpotCollision( const Vector &pos ) const;		// returns true if an existing hiding spot is too close to given position

	//- encounter spots ---------------------------------------------------------------------------------
	// SpotEncounterVector m_spotEncounters;						// list of possible ways to move thru this area, and the spots to look at as we do
	// void AddSpotEncounters( const CNavArea *from, NavDirType fromDir, const CNavArea *to, NavDirType toDir );	// add spot encounter data when moving from area to area

	float m_earliestOccupyTime[ MAX_NAV_TEAMS ];				// min time to reach this spot from spawn

#ifdef DEBUG_AREA_PLAYERCOUNTS
	CUtlVector< int > m_playerEntIndices[ MAX_NAV_TEAMS ];
#endif

	//- lighting ----------------------------------------------------------------------------------------
	float m_lightIntensity[ NUM_CORNERS ];						// 0..1 light intensity at corners

	//- A* pathfinding algorithm ------------------------------------------------------------------------
	static unsigned int m_masterMarker;

	static CNavArea *m_openList;
	static CNavArea *m_openListTail;

	//- connections to adjacent areas -------------------------------------------------------------------
	NavConnectVector m_incomingConnect[ NUM_DIRECTIONS ];		// a list of adjacent areas for each direction that connect TO us, but we have no connection back to them

	//---------------------------------------------------------------------------------------------------
	CNavNode *m_node[ NUM_CORNERS ];							// nav nodes at each corner of the area

	void ResetNodes( void );									// nodes are going away as part of an incremental nav generation
	void Strip( void );											// remove "analyzed" data from nav area

	void FinishMerge( CNavMesh *TheNavMesh, CNavArea *adjArea );						// recompute internal data once nodes have been adjusted during merge
	void MergeAdjacentConnections( CNavMesh *TheNavMesh, CNavArea *adjArea );			// for merging with "adjArea" - pick up all of "adjArea"s connections
	void AssignNodes( CNavArea *area );							// assign internal nodes to the given area

	void FinishSplitEdit( CNavArea *newArea, NavDirType ignoreEdge );	// given the portion of the original area, update its internal data

	void CalcDebugID();

#ifdef NEXT_BOT
	CUtlVector< CHandle< CFuncNavPrerequisite > > m_prerequisiteVector;		// list of prerequisites that must be met before this area can be traversed
#endif

	CNavArea *m_prevHash, *m_nextHash;							// for hash table in CNavMesh

	void ConnectElevators( void );								// find elevator connections between areas

	int m_damagingTickCount;									// this area is damaging through this tick count
	

	uint32 m_nVisTestCounter;
	static uint32 s_nCurrVisTestCounter;

	CUtlVector< CHandle< CFuncNavCost > > m_funcNavCostVector;	// active, overlapping cost entities

	const CNavVolume* m_volume; // Nav volume this area is inside

public:
	void SetNavVolume(const CNavVolume* volume) { m_volume = volume; }
	const CNavVolume* GetNavVolume() const { return m_volume; }
	void NotifyNavVolumeDestruction(const CNavVolume* volume)
	{
		if (volume == m_volume)
		{
			m_volume = nullptr;
		}
	}


};

typedef CUtlVector< CNavArea * > NavAreaVector;


//--------------------------------------------------------------------------------------------------------------
class COverlapCheck
{
public:
	COverlapCheck(const CNavArea *me, const Vector &pos) :
		m_me(me), m_myZ(me->GetZ(pos)), m_pos(pos) {
	}

	bool operator() ( CNavArea *area )
	{
		// skip self
		if ( area == m_me
				// check 2D overlap
			|| !area->IsOverlapping( m_pos ) )
			return true;

		float theirZ = area->GetZ( m_pos );
		return theirZ > m_pos.z
			// they are above the point
				|| theirZ <= m_myZ;
			// we are below an area that is beneath the given position
	}

	const CNavArea *m_me;
	float m_myZ;
	const Vector &m_pos;
};

//--------------------------------------------------------------------------------------------------------------
class ForgetArea
{
public:
	ForgetArea( CNavArea *area )
	{
		m_area = area;
	}

	bool operator() ( edict_t *player )
	{
		// TODO: IMPELMENT
		return true;
	}

	CNavArea *m_area;
};

//--------------------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------------------
//
// Inlines
//

#ifdef NEXT_BOT

//--------------------------------------------------------------------------------------------------------------
inline bool CNavArea::HasPrerequisite( CBaseCombatCharacter *actor ) const
{
	return m_prerequisiteVector.Count() > 0;
}

//--------------------------------------------------------------------------------------------------------------
inline const CUtlVector< CHandle< CFuncNavPrerequisite > > &CNavArea::GetPrerequisiteVector( void ) const
{
	return m_prerequisiteVector;
}

//--------------------------------------------------------------------------------------------------------------
inline void CNavArea::RemoveAllPrerequisites( void )
{
	m_prerequisiteVector.RemoveAll();
}

//--------------------------------------------------------------------------------------------------------------
inline void CNavArea::AddPrerequisite( CFuncNavPrerequisite *prereq )
{
	if ( m_prerequisiteVector.Find( prereq ) == m_prerequisiteVector.InvalidIndex() )
	{
		m_prerequisiteVector.AddToTail( prereq );
	}
}
#endif

//--------------------------------------------------------------------------------------------------------------
inline float CNavArea::GetDangerDecayRate( void ) const
{
	// one kill == 1.0, which we will forget about in two minutes
	return 1.0f / 120.0f;
}

//--------------------------------------------------------------------------------------------------------------
inline bool CNavArea::IsDegenerate( void ) const
{
	return (m_nwCorner.x >= m_seCorner.x || m_nwCorner.y >= m_seCorner.y);
}

//--------------------------------------------------------------------------------------------------------------
inline CNavArea *CNavArea::GetAdjacentArea( NavDirType dir, int i ) const
{
	if ( ( i < 0 ) || ( i >= m_connect[dir].Count() ) )
		return NULL;
	return m_connect[dir][i].area;
}

//--------------------------------------------------------------------------------------------------------------
inline bool CNavArea::IsOpen( void ) const
{
	return (m_openMarker == m_masterMarker) ? true : false;
}

//--------------------------------------------------------------------------------------------------------------
inline bool CNavArea::IsOpenListEmpty( void )
{
	Assert( (m_openList && m_openList->m_prevOpen == NULL) || m_openList == NULL );
	return (m_openList) ? false : true;
}

//--------------------------------------------------------------------------------------------------------------
inline CNavArea *CNavArea::PopOpenList( void )
{
	Assert( (m_openList && m_openList->m_prevOpen == NULL) || m_openList == NULL );

	if ( m_openList )
	{
		CNavArea *area = m_openList;
	
		// disconnect from list
		area->RemoveFromOpenList();
		area->m_prevOpen = NULL;
		area->m_nextOpen = NULL;

		Assert( (m_openList && m_openList->m_prevOpen == NULL) || m_openList == NULL );

		return area;
	}

	Assert( (m_openList && m_openList->m_prevOpen == NULL) || m_openList == NULL );

	return NULL;
}

//--------------------------------------------------------------------------------------------------------------
inline bool CNavArea::IsClosed( void ) const
{
	return IsMarked() && !IsOpen();
}

//--------------------------------------------------------------------------------------------------------------
inline void CNavArea::AddToClosedList( void )
{
	Mark();
}

//--------------------------------------------------------------------------------------------------------------
inline void CNavArea::RemoveFromClosedList( void )
{
	// since "closed" is defined as visited (marked) and not on open list, do nothing
}

//--------------------------------------------------------------------------------------------------------------
inline float CNavArea::GetClearedTimestamp( int teamID ) const
{ 
	return m_clearedTimestamp[ teamID % MAX_NAV_TEAMS ];
}

//--------------------------------------------------------------------------------------------------------------
inline float CNavArea::GetEarliestOccupyTime( int teamID ) const
{
	return m_earliestOccupyTime[ teamID % MAX_NAV_TEAMS ];
}

//--------------------------------------------------------------------------------------------------------------
inline bool CNavArea::HasAvoidanceObstacle( float maxObstructionHeight ) const
{
	return m_avoidanceObstacleHeight > maxObstructionHeight;
}


//--------------------------------------------------------------------------------------------------------------
inline float CNavArea::GetAvoidanceObstacleHeight( void ) const
{
	return m_avoidanceObstacleHeight;
}

//--------------------------------------------------------------------------------------------------------------
/**
* Return Z of area at (x,y) of 'pos'
* Trilinear interpolation of Z values at quad edges.
* NOTE: pos->z is not used.
*/
inline float CNavArea::GetZ( const Vector * pos ) const
{
	return GetZ( pos->x, pos->y );
}

inline float CNavArea::GetZ( const Vector & pos ) const
{
	return GetZ( pos.x, pos.y );
}

//--------------------------------------------------------------------------------------------------------------
/**
* Return the coordinates of the area's corner.
*/
inline Vector CNavArea::GetCorner( NavCornerType corner ) const
{
	// @TODO: Confirm compiler does the "right thing" in release builds, or change this function to to take a pointer [2/4/2009 tom]
	Vector pos;

	switch( corner )
	{
	default:
		return Vector(0.0f, 0.0f, 0.0f);
	case NORTH_WEST:
		return m_nwCorner;

	case NORTH_EAST:
		pos.x = m_seCorner.x;
		pos.y = m_nwCorner.y;
		pos.z = m_neZ;
		return pos;

	case SOUTH_WEST:
		pos.x = m_nwCorner.x;
		pos.y = m_seCorner.y;
		pos.z = m_swZ;
		return pos;

	case SOUTH_EAST:
		return m_seCorner;
	}
}


#endif // _NAV_AREA_H_
