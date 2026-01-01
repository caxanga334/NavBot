//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
// nav_area.cpp
// AI Navigation areas
// Author: Michael S. Booth (mike@turtlerockstudios.com), January 2003

#include NAVBOT_PCH_FILE
#include <unordered_set>
#include <string_view>

#include <extension.h>
#include <manager.h>
#include <extplayer.h>
#include <util/librandom.h>
#include <sdkports/debugoverlay_shared.h>
#include <sdkports/sdk_traces.h>
#include <sdkports/sdk_utils.h>
#include "nav_trace.h"
#include "nav_area.h"
#include "nav_mesh.h"
#include "nav_node.h"
#include "nav_volume.h"
#include "nav_prereq.h"
#include "nav_entities.h"
#include "nav_colors.h"
#include "nav_blocker.h"
#include "nav_pathcost_mod.h"
#include <Color.h>
#include <sdkports/sdk_collisionutils.h>
#include <tier1/checksum_crc.h>

#if SOURCE_ENGINE <= SE_DARKMESSIAH
#include <sdkports/sdk_convarref_ep1.h>
#endif // SOURCE_ENGINE <= SE_DARKMESSIAH


// #include <tslist.h>
#include <utlhash.h>

#ifdef EXT_VPROF_ENABLED
#include <tier0/vprof.h>
#endif // EXT_VPROF_ENABLED

#undef min
#undef max
#undef clamp // mathlib compat hack

extern ConVar sm_nav_quicksave;
unsigned int CNavArea::m_nextID = 1;
NavAreaVector TheNavAreas;

unsigned int CNavArea::m_masterMarker = 1;
CNavArea *CNavArea::m_openList = NULL;
CNavArea *CNavArea::m_openListTail = NULL;

bool CNavArea::m_isReset = false;
uint32 CNavArea::s_nCurrVisTestCounter = 0;

ConVar sm_nav_coplanar_slope_limit( "sm_nav_coplanar_slope_limit", "0.99", FCVAR_CHEAT );
ConVar sm_nav_coplanar_slope_limit_displacement( "sm_nav_coplanar_slope_limit_displacement", "0.7", FCVAR_CHEAT );
ConVar sm_nav_split_place_on_ground( "sm_nav_split_place_on_ground", "0", FCVAR_CHEAT, "If true, nav areas will be placed flush with the ground when split." );
ConVar sm_nav_area_bgcolor( "sm_nav_area_bgcolor", "0 0 0 30", FCVAR_CHEAT, "RGBA color to draw as the background color for nav areas while editing." );
ConVar sm_nav_corner_adjust_adjacent( "sm_nav_corner_adjust_adjacent", "18", FCVAR_CHEAT, "radius used to raise/lower corners in nearby areas when raising/lowering corners." );
#ifdef NAVMESH_REMOVED_FEATURES
ConVar sm_nav_show_light_intensity( "sm_nav_show_light_intensity", "0", FCVAR_CHEAT );
#endif // NAVMESH_REMOVED_FEATURES
ConVar sm_nav_debug_blocked( "sm_nav_debug_blocked", "0", FCVAR_CHEAT );
ConVar sm_nav_show_contiguous( "sm_nav_show_continguous", "0", FCVAR_CHEAT, "Highlight non-contiguous connections" );

constexpr float DEF_NAV_VIEW_DISTANCE = 1500.0;
// ConVar sm_nav_max_view_distance( "sm_nav_max_view_distance", "6000", FCVAR_CHEAT, "Maximum range for precomputed nav mesh visibility (0 = default 1500 units)" );
// ConVar sm_nav_update_visibility_on_edit( "sm_nav_update_visibility_on_edit", "0", FCVAR_CHEAT, "If nonzero editing the mesh will incrementally recompue visibility" );
// ConVar sm_nav_potentially_visible_dot_tolerance( "sm_nav_potentially_visible_dot_tolerance", "0.98", FCVAR_CHEAT );
// ConVar sm_nav_show_potentially_visible( "sm_nav_show_potentially_visible", "0", FCVAR_CHEAT, "Show areas that are potentially visible from the current nav area" );
static ConVar sm_nav_danger_decay_rate("sm_nav_danger_decay_rate", "20", FCVAR_GAMEDLL, "How much area danger is decayed per second.", true, 1.0f, true, 100.0f);

Color s_selectedSetColor( 255, 255, 200, 96 );
Color s_selectedSetBorderColor( 100, 100, 0, 255 );
Color s_dragSelectionSetBorderColor( 50, 50, 50, 255 );

bool UTIL_IsCommandIssuedByServerAdmin() 
{
	if (engine->IsDedicatedServer()) 
	{
		return false;
	}

	for (int i = 2; i <= gpGlobals->maxClients; i++) 
	{
		edict_t* player = gamehelpers->EdictOfIndex(i);
		IPlayerInfo* info = playerinfomanager->GetPlayerInfo(player);

		if (player != nullptr && !player->IsFree() && player->GetNetworkable() != nullptr && info != nullptr && !info->IsFakeClient()) 
		{
			return false;
		}
	}

	return true;
}

#if SOURCE_ENGINE <= SE_DARKMESSIAH
static void SelectedSetColorChaged(ConVar* var, char const* pOldString)
#else
static void SelectedSetColorChaged(IConVar* var, const char* pOldValue, float flOldValue)
#endif // SOURCE_ENGINE <= SE_DARKMESSIAH
{
	ConVarRef colorVar(var->GetName());

	Color *color = &s_selectedSetColor;
	if (strcmp(var->GetName(), "nav_selected_set_border_color") == 0) {
		color = &s_selectedSetBorderColor;
	}

	// Xbox compiler needs these to be in this explicit form
	// likely due to sscanf expecting word aligned boundaries
	int r = color->r();
	int g = color->r();
	int b = color->b();
	int a = color->a();
	int numFound = sscanf(colorVar.GetString(), "%d %d %d %d", &r, &g, &b, &a);

	(*color)[0] = r;
	(*color)[1] = g;
	(*color)[2] = b;

	if (numFound > 3) 
	{
		(*color)[3] = a;
	}
}

ConVar sm_nav_selected_set_color( "sm_nav_selected_set_color", "255 255 200 96", FCVAR_CHEAT, "Color used to draw the selected set background while editing.", false, 0.0f, false, 0.0f, SelectedSetColorChaged );

ConVar sm_nav_selected_set_border_color( "sm_nav_selected_set_border_color", "100 100 0 255", FCVAR_CHEAT, "Color used to draw the selected set borders while editing.", false, 0.0f, false, 0.0f, SelectedSetColorChaged );

const char* NavOffMeshConnection::OffMeshConnectionTypeToString(OffMeshConnectionType type)
{
	using namespace std::literals::string_view_literals;

	if (type >= OffMeshConnectionType::MAX_OFFMESH_CONNECTION_TYPES)
	{
		return "ERROR";
	}

	return NavOffMeshConnection::s_linknames.at(static_cast<size_t>(type)).data();
}

OffMeshConnectionType NavOffMeshConnection::GetOffMeshConnectionTypeFromString(const char* input)
{
	int i = 0;

	for (auto& sv : NavOffMeshConnection::s_linknames)
	{
		if (std::strcmp(input, sv.data()) == 0)
		{
			return static_cast<OffMeshConnectionType>(i);
		}

		i++;
	}

	return OffMeshConnectionType::OFFMESH_INVALID;
}

Extent::Extent(CBaseEntity* entity)
{
	Init(entity);
}

void Extent::Init(edict_t *entity)
{
	entity->GetCollideable()->WorldSpaceSurroundingBounds(&lo, &hi);
}

void Extent::Init(CBaseEntity* pEntity)
{
	reinterpret_cast<IServerEntity*>(pEntity)->GetCollideable()->WorldSpaceSurroundingBounds(&lo, &hi);
}

//--------------------------------------------------------------------------------------------------------------
void CNavArea::CompressIDs( CNavMesh *TheNavMesh  )
{
	m_nextID = 1;

	FOR_EACH_VEC( TheNavAreas, id )
	{
		CNavArea *area = TheNavAreas[id];
		area->m_id = m_nextID++;

		// remove and re-add the area from the nav mesh to update the hashed ID
		TheNavMesh->RemoveNavArea( area );
		TheNavMesh->AddNavArea( area );
	}
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Constructor used during normal runtime.
 */
CNavArea::CNavArea(unsigned int place)
{
	m_marker = 0;
	m_nearNavSearchMarker = 0;
	m_damagingTickCount = 0;
	m_openMarker = 0;

	m_parent = NULL;
	m_parentHow = GO_NORTH;
	m_attributeFlags = 0;
	m_place = place;
	m_isUnderwater = false;
	m_avoidanceObstacleHeight = 0.0f;

	m_totalCost = 0.0f;
	m_costSoFar = 0.0f;
	m_pathLengthSoFar = 0.0f;

	ResetNodes();

	for (auto& b : m_isBlocked)
	{
		b = false;
	}

	int i;
	for ( i=0; i<MAX_NAV_TEAMS; ++i )
	{
		m_earliestOccupyTime[i] = 0.0f;
	}

	std::fill(m_danger.begin(), m_danger.end(), 0.0f);

	// set an ID for splitting and other interactive editing - loads will overwrite this
	m_id = m_nextID++;
	m_debugid = 0;

	m_prevHash = NULL;
	m_nextHash = NULL;

	m_isBattlefront = false;

	for( i = 0; i<NUM_DIRECTIONS; ++i )
	{
		m_connect[i].RemoveAll();
	}

	for( i=0; i<CNavLadder::NUM_LADDER_DIRECTIONS; ++i )
	{
		m_ladder[i].RemoveAll();
	}

	for ( i=0; i<NUM_CORNERS; ++i )
	{
		m_lightIntensity[i] = 1.0f;
	}

	m_elevator = nullptr;
	m_elevfloor = nullptr;

	m_invDxCorners = 0;
	m_invDyCorners = 0;

	m_funcNavCostVector.RemoveAll();

	m_nVisTestCounter = (uint32)-1;

	m_offmeshconnections.reserve(4);
	m_volume = nullptr;
	m_prerequisite = nullptr;
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Assumes Z is flat
 */
void CNavArea::Build( const Vector &corner, const Vector &otherCorner )
{
	if (corner.x < otherCorner.x)
	{
		m_nwCorner.x = corner.x;
		m_seCorner.x = otherCorner.x;
	}
	else
	{
		m_seCorner.x = corner.x;
		m_nwCorner.x = otherCorner.x;
	}

	if (corner.y < otherCorner.y)
	{
		m_nwCorner.y = corner.y;
		m_seCorner.y = otherCorner.y;
	}
	else
	{
		m_seCorner.y = corner.y;
		m_nwCorner.y = otherCorner.y;
	}

	m_nwCorner.z = corner.z;
	m_seCorner.z = corner.z;

	m_center.x = (m_nwCorner.x + m_seCorner.x)/2.0f;
	m_center.y = (m_nwCorner.y + m_seCorner.y)/2.0f;
	m_center.z = (m_nwCorner.z + m_seCorner.z)/2.0f;

	if ( ( m_seCorner.x - m_nwCorner.x ) > 0.0f && ( m_seCorner.y - m_nwCorner.y ) > 0.0f )
	{
		m_invDxCorners = 1.0f / ( m_seCorner.x - m_nwCorner.x );
		m_invDyCorners = 1.0f / ( m_seCorner.y - m_nwCorner.y );
	}
	else
	{
		m_invDxCorners = m_invDyCorners = 0;
	}

	m_neZ = corner.z;
	m_swZ = otherCorner.z;

	CalcDebugID();
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Build a nav area given the positions of its four corners.
 */
void CNavArea::Build( const Vector &nwCorner, const Vector &neCorner, const Vector &seCorner, const Vector &swCorner )
{
	m_nwCorner = nwCorner;
	m_seCorner = seCorner;

	m_center.x = (m_nwCorner.x + m_seCorner.x)/2.0f;
	m_center.y = (m_nwCorner.y + m_seCorner.y)/2.0f;
	m_center.z = (m_nwCorner.z + m_seCorner.z)/2.0f;

	m_neZ = neCorner.z;
	m_swZ = swCorner.z;

	if ( ( m_seCorner.x - m_nwCorner.x ) > 0.0f && ( m_seCorner.y - m_nwCorner.y ) > 0.0f )
	{
		m_invDxCorners = 1.0f / ( m_seCorner.x - m_nwCorner.x );
		m_invDyCorners = 1.0f / ( m_seCorner.y - m_nwCorner.y );
	}
	else
	{
		m_invDxCorners = m_invDyCorners = 0;
	}

	CalcDebugID();
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Used during generation phase to build nav areas from sampled nodes.
 */
void CNavArea::Build( CNavNode *nwNode, CNavNode *neNode, CNavNode *seNode, CNavNode *swNode )
{
	m_nwCorner = *nwNode->GetPosition();
	m_seCorner = *seNode->GetPosition();

	m_center.x = (m_nwCorner.x + m_seCorner.x)/2.0f;
	m_center.y = (m_nwCorner.y + m_seCorner.y)/2.0f;
	m_center.z = (m_nwCorner.z + m_seCorner.z)/2.0f;

	m_neZ = neNode->GetPosition()->z;
	m_swZ = swNode->GetPosition()->z;

	m_node[ NORTH_WEST ] = nwNode;
	m_node[ NORTH_EAST ] = neNode;
	m_node[ SOUTH_EAST ] = seNode;
	m_node[ SOUTH_WEST ] = swNode;

	if ( ( m_seCorner.x - m_nwCorner.x ) > 0.0f && ( m_seCorner.y - m_nwCorner.y ) > 0.0f )
	{
		m_invDxCorners = 1.0f / ( m_seCorner.x - m_nwCorner.x );
		m_invDyCorners = 1.0f / ( m_seCorner.y - m_nwCorner.y );
	}
	else
	{
		m_invDxCorners = m_invDyCorners = 0;
	}

	// mark internal nodes as part of this area
	AssignNodes( this );

	CalcDebugID();
}

//--------------------------------------------------------------------------------------------------------------
// Return a computed extent (XY is in m_nwCorner and m_seCorner, Z is computed)
void CNavArea::GetExtent( Extent *extent ) const
{
	extent->lo = m_nwCorner;
	extent->hi = m_seCorner;

	extent->lo.z = MIN( extent->lo.z, m_nwCorner.z );
	extent->lo.z = MIN( extent->lo.z, m_seCorner.z );
	extent->lo.z = MIN( extent->lo.z, m_neZ );
	extent->lo.z = MIN( extent->lo.z, m_swZ );

	extent->hi.z = MAX( extent->hi.z, m_nwCorner.z );
	extent->hi.z = MAX( extent->hi.z, m_seCorner.z );
	extent->hi.z = MAX( extent->hi.z, m_neZ );
	extent->hi.z = MAX( extent->hi.z, m_swZ );
}

//--------------------------------------------------------------------------------------------------------------
// returns the closest node along the given edge to the given point
CNavNode *CNavArea::FindClosestNode( const Vector &pos, NavDirType dir ) const
{
	if ( !HasNodes() )
		return NULL;

	CUtlVector< CNavNode * > nodes;
	GetNodes( dir, &nodes );

	CNavNode *bestNode = NULL;
	float bestDistanceSq = FLT_MAX;

	for ( int i=0; i<nodes.Count(); ++i )
	{
		float distSq = pos.DistToSqr( *nodes[i]->GetPosition() );
		if ( distSq < bestDistanceSq )
		{
			bestDistanceSq = distSq;
			bestNode = nodes[i];
		}
	}

	return bestNode;
}

//--------------------------------------------------------------------------------------------------------------
// build a vector of nodes along the given direction
void CNavArea::GetNodes( NavDirType dir, CUtlVector< CNavNode * > *nodes ) const
{
	if ( !nodes )
		return;

	nodes->RemoveAll();

	NavCornerType startCorner;
	NavCornerType endCorner;
	NavDirType traversalDirection;

	switch ( dir )
	{
	case NORTH:
		startCorner = NORTH_WEST;
		endCorner = NORTH_EAST;
		traversalDirection = EAST;
		break;

	case SOUTH:
		startCorner = SOUTH_WEST;
		endCorner = SOUTH_EAST;
		traversalDirection = EAST;
		break;

	case EAST:
		startCorner = NORTH_EAST;
		endCorner = SOUTH_EAST;
		traversalDirection = SOUTH;
		break;

	case WEST:
		startCorner = NORTH_WEST;
		endCorner = SOUTH_WEST;
		traversalDirection = SOUTH;
		break;

	default:
		return;
	}

	CNavNode *node;
	for ( node = m_node[ startCorner ]; node && node != m_node[ endCorner ]; node = node->GetConnectedNode( traversalDirection ) )
	{
		nodes->AddToTail( node );
	}
	if ( node && node == m_node[ endCorner ] )
	{
		nodes->AddToTail( node );
	}
}

//--------------------------------------------------------------------------------------------------------------
class AreaDestroyNotification
{
	CNavArea *m_area;

public:
	AreaDestroyNotification( CNavArea *area )
	{
		m_area = area;
	}

	bool operator()( CNavLadder *ladder )
	{
		ladder->OnDestroyNotify( m_area );
		return true;
	}

	bool operator()( CNavArea *area )
	{
		if ( area != m_area )
		{
			area->OnDestroyNotify( m_area );
		}
		return true;
	}
};

//--------------------------------------------------------------------------------------------------------------
/**
 * Destructor
 */
CNavArea::~CNavArea()
{
	m_offmeshconnections.clear();
	m_navblockers.clear();

	// if we are resetting the system, don't bother cleaning up - all areas are being destroyed
	if (m_isReset)
		return;

	// tell the other areas and ladders we are going away
	AreaDestroyNotification notification( this );
	TheNavMesh->ForAllAreas( notification );
	TheNavMesh->ForAllLadders( notification );

	// remove the area from the grid
	TheNavMesh->RemoveNavArea( this );
	
	// make sure no players keep a pointer to this area
	// ForgetArea forget( this );
	// ForEachActor( forget );
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Invoked when map is initially loaded
 */
void CNavArea::OnServerActivate( void )
{
	m_damagingTickCount = 0;
	ClearAllNavCostEntities();
	ClearDanger(NAV_TEAM_ANY);
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Invoked for each area when the round restarts
 */
void CNavArea::OnRoundRestart( void )
{
	m_damagingTickCount = 0;
	ClearAllNavCostEntities();
	ClearDanger(NAV_TEAM_ANY);
}

#ifdef DEBUG_AREA_PLAYERCOUNTS
//--------------------------------------------------------------------------------------------------------------
void CNavArea::IncrementPlayerCount( int teamID, int entIndex )
{
	ConColorMsg( Color( 128, 255, 128, 255 ), "%f: Adding ent %d (team %d) to area %d\n", gpGlobals->curtime, entIndex, teamID, GetID() );
	teamID = teamID % MAX_NAV_TEAMS;
	Assert( !m_playerEntIndices[teamID].HasElement( entIndex ) );
	if ( !m_playerEntIndices[teamID].HasElement( entIndex ) )
	{
		m_playerEntIndices[teamID].AddToTail( entIndex );
	}

	if (m_playerCount[ teamID ] == 255)
	{
		Warning( "CNavArea::IncrementPlayerCount: Overflow\n" );
		return;
	}

	++m_playerCount[ teamID ];
}

//--------------------------------------------------------------------------------------------------------------
void CNavArea::DecrementPlayerCount( int teamID, int entIndex )
{
	ConColorMsg( Color( 128, 128, 255, 255 ), "%f: Removing ent %d (team %d) from area %d\n", gpGlobals->curtime, entIndex, teamID, GetID() );
	teamID = teamID % MAX_NAV_TEAMS;
	Assert( m_playerEntIndices[teamID].HasElement( entIndex ) );
	m_playerEntIndices[teamID].FindAndFastRemove( entIndex );

	if (m_playerCount[ teamID ] == 0)
	{
		Warning( "CNavArea::IncrementPlayerCount: Underflow\n" );
		return;
	}

	--m_playerCount[ teamID ];
}
#endif // DEBUG_AREA_PLAYERCOUNTS

//--------------------------------------------------------------------------------------------------------------
/**
 * This is invoked at the start of an incremental nav generation on pre-existing areas.
 */
void CNavArea::ResetNodes( void )
{
	for ( int i=0; i<NUM_CORNERS; ++i )
	{
		m_node[i] = NULL;
	}
}

//--------------------------------------------------------------------------------------------------------------
bool CNavArea::HasNodes( void ) const
{
	for ( int i=0; i<NUM_CORNERS; ++i )
	{
		if ( m_node[i] )
		{
			return true;
		}
	}

	return false;
}

void CNavArea::OnUpdate()
{
	DecayDanger();
}

//--------------------------------------------------------------------------------------------------------------
/**
 * This is invoked when an area is going away.
 * Remove any references we have to it.
 */
void CNavArea::OnDestroyNotify( CNavArea *dead )
{
	NavConnect con;
	con.area = dead;
	for( int d=0; d<NUM_DIRECTIONS; ++d )
	{
		m_connect[ d ].FindAndRemove( con );
		m_incomingConnect[ d ].FindAndRemove( con );
	}

	m_offmeshconnections.erase(std::remove_if(m_offmeshconnections.begin(), m_offmeshconnections.end(),
		[dead](const NavOffMeshConnection& link) { return link.IsConnectedTo(dead); }), m_offmeshconnections.end());
}


//--------------------------------------------------------------------------------------------------------------
/**
 * This is invoked when a ladder is going away.
 * Remove any references we have to it.
 */
void CNavArea::OnDestroyNotify( CNavLadder *dead )
{
	Disconnect( dead );
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Connect this area to given area in given direction
 */
void CNavArea::ConnectTo( CNavArea *area, NavDirType dir )
{
	// don't allow self-referential connections
	if ( area == this )
		return;

	// check if already connected
	FOR_EACH_VEC( m_connect[ dir ], it )
	{
		if (m_connect[ dir ][ it ].area == area)
			return;
	}

	NavConnect con;
	con.area = area;
	con.length = ( area->GetCenter() - GetCenter() ).Length();
	m_connect[ dir ].AddToTail( con );
	m_incomingConnect[ dir ].FindAndRemove( con );

	NavDirType dirOpposite = OppositeDirection( dir );
	con.area = this;
	if ( area->m_connect[ dirOpposite ].Find( con ) == area->m_connect[ dirOpposite ].InvalidIndex() )
	{
		area->AddIncomingConnection( this, dirOpposite );
	}	
	
	//static char *dirName[] = { "NORTH", "EAST", "SOUTH", "WEST" };
	//CONSOLE_ECHO( "  Connected area #%d to #%d, %s\n", m_id, area->m_id, dirName[ dir ] );
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Connect this area to given ladder
 */
void CNavArea::ConnectTo( CNavLadder *ladder )
{
	Disconnect( ladder ); // just in case

	if ( GetCenter().z > (ladder->m_top.z + ladder->m_bottom.z) * 0.5f )
	{
		AddLadderDown( ladder );
	}
	else
	{
		AddLadderUp( ladder );
	}
}

bool CNavArea::ConnectTo(CNavArea* area, OffMeshConnectionType linktype, const Vector& start, const Vector& end)
{
	if (area == this)
	{
		return false;
	}

	Vector pos;

	if (IsConnectedToBySpecialLink(area))
	{
		Warning("Only one off-mesh link per area connection! \n");
		return false;
	}

	const float range = (GetCenter() - start).Length();

	if (range >= 512.0f)
	{
		Warning("Distance between start area center and start position is %g! Is this correct? \n", range);
	}

	pos.x = start.x;
	pos.y = start.y;
	pos.z = start.z;

	m_offmeshconnections.emplace_back(linktype, area, pos, end);
	Msg("Added off-mesh connection between area #%i and #%i \n", GetID(), area->GetID());
	NDebugOverlay::HorzArrow(pos + Vector(0.0f, 0.0f, 72.0f), pos, 4.0f, 0, 255, 255, 255, true, 10.0f);

	return true;
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Disconnect this area from given area
 */
void CNavArea::Disconnect( CNavArea *area )
{
	NavConnect connect;
	connect.area = area;

	for( int i = 0; i<NUM_DIRECTIONS; i++ )
	{
		NavDirType dir = (NavDirType) i;
		NavDirType dirOpposite = OppositeDirection( dir );
		int index = m_connect[ dir ].Find( connect );
		if ( index != m_connect[ dir ].InvalidIndex() )
		{
			m_connect[ dir ].Remove( index );
			if ( area->IsConnected( this, dirOpposite ) )
			{
				AddIncomingConnection( area, dir );
			}
			else
			{
				connect.area = this;
				area->m_incomingConnect[ dirOpposite ].FindAndRemove( connect );
			}
		}		
	}
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Disconnect this area from given ladder
 */
void CNavArea::Disconnect( CNavLadder *ladder )
{
	NavLadderConnect con;
	con.ladder = ladder;

	for( int i=0; i<CNavLadder::NUM_LADDER_DIRECTIONS; ++i )
	{
		m_ladder[i].FindAndRemove( con );
	}
}

void CNavArea::Disconnect(CNavArea* area, OffMeshConnectionType linktype)
{
	m_offmeshconnections.erase(std::remove_if(m_offmeshconnections.begin(), m_offmeshconnections.end(), [&area, &linktype](const NavOffMeshConnection& connection) {
		return connection.GetConnectedArea() == area && connection.m_type == linktype;
	}), m_offmeshconnections.end());

	Msg("Removing special link of type %i between areas #%i and #%i. \n", static_cast<int>(linktype), GetID(), area->GetID());
}

//--------------------------------------------------------------------------------------------------------------
void CNavArea::AddLadderUp( CNavLadder *ladder )
{
	Disconnect( ladder ); // just in case

	NavLadderConnect tmp;
	tmp.ladder = ladder;
	m_ladder[ CNavLadder::LADDER_UP ].AddToTail( tmp );
}


//--------------------------------------------------------------------------------------------------------------
void CNavArea::AddLadderDown( CNavLadder *ladder )
{
	Disconnect( ladder ); // just in case

	NavLadderConnect tmp;
	tmp.ladder = ladder;
	m_ladder[ CNavLadder::LADDER_DOWN ].AddToTail( tmp );
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Recompute internal data once nodes have been adjusted during merge
 * Destroy adjArea.
 */
void CNavArea::FinishMerge( CNavMesh* TheNavMesh, CNavArea *adjArea )
{
	// update extent
	m_nwCorner = *m_node[ NORTH_WEST ]->GetPosition();
	m_seCorner = *m_node[ SOUTH_EAST ]->GetPosition();

	m_center.x = (m_nwCorner.x + m_seCorner.x)/2.0f;
	m_center.y = (m_nwCorner.y + m_seCorner.y)/2.0f;
	m_center.z = (m_nwCorner.z + m_seCorner.z)/2.0f;

	m_neZ = m_node[ NORTH_EAST ]->GetPosition()->z;
	m_swZ = m_node[ SOUTH_WEST ]->GetPosition()->z;

	if ( ( m_seCorner.x - m_nwCorner.x ) > 0.0f && ( m_seCorner.y - m_nwCorner.y ) > 0.0f )
	{
		m_invDxCorners = 1.0f / ( m_seCorner.x - m_nwCorner.x );
		m_invDyCorners = 1.0f / ( m_seCorner.y - m_nwCorner.y );
	}
	else
	{
		m_invDxCorners = m_invDyCorners = 0;
	}

	// reassign the adjacent area's internal nodes to the final area
	adjArea->AssignNodes( this );

	// merge adjacency links - we gain all the connections that adjArea had
	MergeAdjacentConnections(TheNavMesh, adjArea );

	// remove subsumed adjacent area
	TheNavAreas.FindAndRemove( adjArea );
	TheNavMesh->OnEditDestroyNotify( adjArea );
	TheNavMesh->DestroyArea( adjArea );
}


//--------------------------------------------------------------------------------------------------------------
class LadderConnectionReplacement
{
	CNavArea *m_originalArea;
	CNavArea *m_replacementArea;

public:
	LadderConnectionReplacement( CNavArea *originalArea, CNavArea *replacementArea )
	{
		m_originalArea = originalArea;
		m_replacementArea = replacementArea;
	}

	bool operator()( CNavLadder *ladder )
	{
		if (ladder->IsConnected(m_originalArea))
		{
			ladder->Disconnect(m_originalArea);
			ladder->ConnectTo(m_replacementArea);
		}

		return true;
	}
};


//--------------------------------------------------------------------------------------------------------------
/**
 * For merging with "adjArea" - pick up all of "adjArea"s connections
 */
void CNavArea::MergeAdjacentConnections(CNavMesh *TheNavNesh, CNavArea *adjArea )
{
	// merge adjacency links - we gain all the connections that adjArea had
	int dir;
	for( dir = 0; dir<NUM_DIRECTIONS; dir++ )
	{
		FOR_EACH_VEC( adjArea->m_connect[ dir ], it )
		{
			NavConnect connect = adjArea->m_connect[ dir ][ it ];

			if (connect.area != adjArea && connect.area != this)
				ConnectTo( connect.area, (NavDirType)dir );
		}
	}

	// remove any references from this area to the adjacent area, since it is now part of us
	Disconnect( adjArea );
	
	// Change other references to adjArea to refer instead to us
	// We can't just replace existing connections, as several adjacent areas may have been merged into one,
	// resulting in a large area adjacent to all of them ending up with multiple redunandant connections
	// into the merged area, one for each of the adjacent subsumed smaller ones.
	// If an area has a connection to the merged area, we must remove all references to adjArea, and add
	// a single connection to us.
	FOR_EACH_VEC( TheNavAreas, it )
	{
		CNavArea *area = TheNavAreas[ it ];

		if (area == this || area == adjArea)
			continue;

		for( dir = 0; dir<NUM_DIRECTIONS; dir++ )
		{
			// check if there are any references to adjArea in this direction
			bool connected = false;
			FOR_EACH_VEC( area->m_connect[ dir ], cit )
			{
				NavConnect connect = area->m_connect[ dir ][ cit ];

				if (connect.area == adjArea)
				{
					connected = true;
					break;
				}
			}

			if (connected)
			{
				// remove all references to adjArea
				area->Disconnect( adjArea );

				// remove all references to the new area
				area->Disconnect( this );

				// add a single connection to the new area
				area->ConnectTo( this, (NavDirType) dir );
			}
		}
	}

	// We gain all ladder connections adjArea had
	for( dir=0; dir<CNavLadder::NUM_LADDER_DIRECTIONS; ++dir )
	{
		FOR_EACH_VEC( adjArea->m_ladder[ dir ], it )
		{
			ConnectTo( adjArea->m_ladder[ dir ][ it ].ladder );
		}
	}

	// All ladders that point to adjArea should point to us now
	LadderConnectionReplacement replacement( adjArea, this );
	TheNavMesh->ForAllLadders( replacement );
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Assign internal nodes to the given area
 * NOTE: "internal" nodes do not include the east or south border nodes
 */
void CNavArea::AssignNodes( CNavArea *area )
{
	CNavNode *horizLast = m_node[ NORTH_EAST ];

	for( CNavNode *vertNode = m_node[ NORTH_WEST ]; vertNode != m_node[ SOUTH_WEST ]; vertNode = vertNode->GetConnectedNode( SOUTH ) )
	{
		for( CNavNode *horizNode = vertNode; horizNode != horizLast; horizNode = horizNode->GetConnectedNode( EAST ) )
		{
			horizNode->AssignArea( area );
		}

		horizLast = horizLast->GetConnectedNode( SOUTH );
	}
}


//--------------------------------------------------------------------------------------------------------------
class SplitNotification
{
	CNavArea *m_originalArea;
	CNavArea *m_alphaArea;
	CNavArea *m_betaArea;

public:
	SplitNotification( CNavArea *originalArea, CNavArea *alphaArea, CNavArea *betaArea )
	{
		m_originalArea = originalArea;
		m_alphaArea = alphaArea;
		m_betaArea = betaArea;
	}

	bool operator()( CNavLadder *ladder )
	{
		ladder->OnSplit( m_originalArea, m_alphaArea, m_betaArea );
		return true;
	}
};


//--------------------------------------------------------------------------------------------------------------
/**
 * Split this area into two areas at the given edge.
 * Preserve all adjacency connections.
 * NOTE: This does not update node connections, only areas.
 */
bool CNavArea::SplitEdit( bool splitAlongX, float splitEdge, CNavArea **outAlpha, CNavArea **outBeta )
{
	CNavArea *alpha = NULL;
	CNavArea *beta = NULL;

	if (splitAlongX)
	{
		// +-----+->X
		// |  A  |
		// +-----+
		// |  B  |
		// +-----+
		// |
		// Y

		// don't do split if at edge of area
		if (splitEdge <= m_nwCorner.y + 1.0f)
			return false;

		if (splitEdge >= m_seCorner.y - 1.0f)
			return false;

		alpha = TheNavMesh->CreateArea();
		alpha->m_nwCorner = m_nwCorner;

		alpha->m_seCorner.x = m_seCorner.x;
		alpha->m_seCorner.y = splitEdge;
		alpha->m_seCorner.z = GetZ( alpha->m_seCorner );

		beta = TheNavMesh->CreateArea();
		beta->m_nwCorner.x = m_nwCorner.x;
		beta->m_nwCorner.y = splitEdge;
		beta->m_nwCorner.z = GetZ( beta->m_nwCorner );

		beta->m_seCorner = m_seCorner;

		alpha->ConnectTo( beta, SOUTH );
		beta->ConnectTo( alpha, NORTH );

		FinishSplitEdit( alpha, SOUTH );
		FinishSplitEdit( beta, NORTH );
	}
	else
	{
		// +--+--+->X
		// |  |  |
		// | A|B |
		// |  |  |
		// +--+--+
		// |
		// Y

		// don't do split if at edge of area
		if (splitEdge <= m_nwCorner.x + 1.0f)
			return false;

		if (splitEdge >= m_seCorner.x - 1.0f)
			return false;

		alpha = TheNavMesh->CreateArea();
		alpha->m_nwCorner = m_nwCorner;

		alpha->m_seCorner.x = splitEdge;
		alpha->m_seCorner.y = m_seCorner.y;
		alpha->m_seCorner.z = GetZ( alpha->m_seCorner );

		beta = TheNavMesh->CreateArea();
		beta->m_nwCorner.x = splitEdge;
		beta->m_nwCorner.y = m_nwCorner.y;
		beta->m_nwCorner.z = GetZ( beta->m_nwCorner );

		beta->m_seCorner = m_seCorner;

		alpha->ConnectTo( beta, EAST );
		beta->ConnectTo( alpha, WEST );

		FinishSplitEdit( alpha, EAST );
		FinishSplitEdit( beta, WEST );
	}

	if ( !TheNavMesh->IsGenerating() && sm_nav_split_place_on_ground.GetBool() )
	{
		alpha->PlaceOnGround( NUM_CORNERS );
		beta->PlaceOnGround( NUM_CORNERS );
	}

	// For every ladder we pointed to, alpha or beta should point to it, based on
	// their distance to the ladder
	int dir;
	for( dir=0; dir<CNavLadder::NUM_LADDER_DIRECTIONS; ++dir )
	{
		FOR_EACH_VEC( m_ladder[ dir ], it )
		{
			CNavLadder *ladder = m_ladder[ dir ][ it ].ladder;
			Vector ladderPos = ladder->m_top; // doesn't matter if we choose top or bottom

			float alphaDistance = alpha->GetDistanceSquaredToPoint( ladderPos );
			float betaDistance = beta->GetDistanceSquaredToPoint( ladderPos );

			if ( alphaDistance < betaDistance )
			{
				alpha->ConnectTo( ladder );
			}
			else
			{
				beta->ConnectTo( ladder );
			}
		}
	}

	// For every ladder that pointed to us, connect that ladder to the closer of alpha and beta
	SplitNotification notify( this, alpha, beta );
	TheNavMesh->ForAllLadders( notify );

	// return new areas
	if (outAlpha)
		*outAlpha = alpha;

	if (outBeta)
		*outBeta = beta;

	TheNavMesh->OnEditCreateNotify( alpha );
	TheNavMesh->OnEditCreateNotify( beta );
	if ( TheNavMesh->IsInSelectedSet( this ) )
	{
		TheNavMesh->AddToSelectedSet( alpha );
		TheNavMesh->AddToSelectedSet( beta );
	}

	// remove original area
	TheNavMesh->OnEditDestroyNotify( this );
	TheNavAreas.FindAndRemove( this );
	TheNavMesh->RemoveFromSelectedSet( this );
	TheNavMesh->DestroyArea( this );

	return true;
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Return true if given ladder is connected in given direction
 * @todo Formalize "asymmetric" flag on connections
 */
bool CNavArea::IsConnected( const CNavLadder *ladder, CNavLadder::LadderDirectionType dir ) const
{
	FOR_EACH_VEC( m_ladder[ dir ], it )
	{
		if ( ladder == m_ladder[ dir ][ it ].ladder )
		{
			return true;
		}
	}

	return false;
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Return true if given area is connected in given direction
 * if dir == NUM_DIRECTIONS, check all directions (direction is unknown)
 * @todo Formalize "asymmetric" flag on connections
 */
bool CNavArea::IsConnected( const CNavArea *area, NavDirType dir ) const
{
	// we are connected to ourself
	if (area == this)
		return true;

	if (dir == NUM_DIRECTIONS)
	{
		// search all directions
		for( int d=0; d<NUM_DIRECTIONS; ++d )
		{
			FOR_EACH_VEC( m_connect[ d ], it )
			{
				if (area == m_connect[ d ][ it ].area)
					return true;
			}
		}

		// check ladder connections
		FOR_EACH_VEC( m_ladder[ CNavLadder::LADDER_UP ], it )
		{
			CNavLadder *ladder = m_ladder[ CNavLadder::LADDER_UP ][ it ].ladder;

			for (auto& connect : ladder->GetConnections())
			{
				if (connect.IsConnectedToLadderTop() && connect.GetConnectedArea() == area)
				{
					return true;
				}
			}
		}

		FOR_EACH_VEC( m_ladder[ CNavLadder::LADDER_DOWN ], dit )
		{
			CNavLadder *ladder = m_ladder[ CNavLadder::LADDER_DOWN ][ dit ].ladder;

			for (auto& connect : ladder->GetConnections())
			{
				if (connect.IsConnectedToLadderBottom() && connect.GetConnectedArea() == area)
				{
					return true;
				}
			}
		}
	}
	else
	{
		// check specific direction
		FOR_EACH_VEC( m_connect[ dir ], it )
		{
			if (area == m_connect[ dir ][ it ].area)
				return true;
		}
	}

	return false;
}

bool CNavArea::IsConnected(const CNavArea* area, OffMeshConnectionType linktype) const
{
	for (auto& link : m_offmeshconnections)
	{
		if (link.IsOfType(linktype))
		{
			if (link.m_link.area == area)
			{
				return true;
			}
		}
	}

	return false;
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Compute change in actual ground height from this area to given area
 */
float CNavArea::ComputeGroundHeightChange( const CNavArea *area ) const
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("[NavBot] CNavArea::ComputeGroundHeightChange", "NavBot");
#endif // EXT_VPROF_ENABLED

	Vector closeFrom, closeTo;
	area->GetClosestPointOnArea( GetCenter(), &closeTo );
	GetClosestPointOnArea( area->GetCenter(), &closeFrom );

	// find actual ground height at each point in case 
	// areas are below/above actual terrain
	float toZ, fromZ;
	if ( TheNavMesh->GetSimpleGroundHeight( closeTo + Vector( 0, 0, navgenparams->step_height ), &toZ ) == false )
	{
		return 0.0f;
	}

	if ( TheNavMesh->GetSimpleGroundHeight( closeFrom + Vector( 0, 0, navgenparams->step_height ), &fromZ ) == false )
	{
		return 0.0f;
	}

	return toZ - fromZ;
}


//--------------------------------------------------------------------------------------------------------------
/**
 * The area 'source' is connected to us along our 'incomingEdgeDir' edge
 */
void CNavArea::AddIncomingConnection( CNavArea *source, NavDirType incomingEdgeDir )
{
	NavConnect con;
	con.area = source;
	if ( m_incomingConnect[ incomingEdgeDir ].Find( con ) == m_incomingConnect[ incomingEdgeDir ].InvalidIndex() )
	{
		con.length = ( source->GetCenter() - GetCenter() ).Length();
		m_incomingConnect[ incomingEdgeDir ].AddToTail( con );
	}	
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Given the portion of the original area, update its internal data
 * The "ignoreEdge" direction defines the side of the original area that the new area does not include
 */
void CNavArea::FinishSplitEdit( CNavArea *newArea, NavDirType ignoreEdge )
{
	newArea->InheritAttributes( this );

	newArea->m_center.x = (newArea->m_nwCorner.x + newArea->m_seCorner.x)/2.0f;
	newArea->m_center.y = (newArea->m_nwCorner.y + newArea->m_seCorner.y)/2.0f;
	newArea->m_center.z = (newArea->m_nwCorner.z + newArea->m_seCorner.z)/2.0f;

	newArea->m_neZ = GetZ( newArea->m_seCorner.x, newArea->m_nwCorner.y );
	newArea->m_swZ = GetZ( newArea->m_nwCorner.x, newArea->m_seCorner.y );

	if ( ( m_seCorner.x - m_nwCorner.x ) > 0.0f && ( m_seCorner.y - m_nwCorner.y ) > 0.0f )
	{
		newArea->m_invDxCorners = 1.0f / ( newArea->m_seCorner.x - newArea->m_nwCorner.x );
		newArea->m_invDyCorners = 1.0f / ( newArea->m_seCorner.y - newArea->m_nwCorner.y );
	}
	else
	{
		newArea->m_invDxCorners = newArea->m_invDyCorners = 0;
	}

	// connect to adjacent areas
	for( int d=0; d<NUM_DIRECTIONS; ++d )
	{
		if (d == ignoreEdge)
			continue;

		int count = GetAdjacentCount( (NavDirType)d );

		for( int a=0; a<count; ++a )
		{
			CNavArea *adj = GetAdjacentArea( (NavDirType)d, a );

			switch( d )
			{
				case NORTH:
				case SOUTH:
					if (newArea->IsOverlappingX( adj ))
					{
						newArea->ConnectTo( adj, (NavDirType)d );			

						// add reciprocal connection if needed
						if (adj->IsConnected( this, OppositeDirection( (NavDirType)d )))
							adj->ConnectTo( newArea, OppositeDirection( (NavDirType)d ) );
					}
					break;

				case EAST:
				case WEST:
					if (newArea->IsOverlappingY( adj ))
					{
						newArea->ConnectTo( adj, (NavDirType)d );			

						// add reciprocal connection if needed
						if (adj->IsConnected( this, OppositeDirection( (NavDirType)d )))
							adj->ConnectTo( newArea, OppositeDirection( (NavDirType)d ) );
					}
					break;
			}

			for ( int a = 0; a < m_incomingConnect[d].Count(); a++ )
			{			
				CNavArea *adj = m_incomingConnect[d][a].area;

				switch( d )
				{
				case NORTH:
				case SOUTH:
					if (newArea->IsOverlappingX( adj ))
					{
						adj->ConnectTo( newArea, OppositeDirection( (NavDirType)d ) );
					}
					break;

				case EAST:
				case WEST:
					if (newArea->IsOverlappingY( adj ))
					{
						adj->ConnectTo( newArea, OppositeDirection( (NavDirType)d ) );
					}
					break;
				}
			}			
		}
	}

	TheNavAreas.AddToTail( newArea );
	TheNavMesh->AddNavArea( newArea );

	// Assign nodes
	if ( HasNodes() )
	{
		// first give it all our nodes...
		newArea->m_node[ NORTH_WEST ] = m_node[ NORTH_WEST ];
		newArea->m_node[ NORTH_EAST ] = m_node[ NORTH_EAST ];
		newArea->m_node[ SOUTH_EAST ] = m_node[ SOUTH_EAST ];
		newArea->m_node[ SOUTH_WEST ] = m_node[ SOUTH_WEST ];

		// ... then pull in one edge...
		NavDirType dir = NUM_DIRECTIONS;
		NavCornerType corner[2] = { NUM_CORNERS, NUM_CORNERS };

		switch ( ignoreEdge )
		{
		case NORTH:
			dir = SOUTH;
			corner[0] = NORTH_WEST;
			corner[1] = NORTH_EAST;
			break;
		case SOUTH:
			dir = NORTH;
			corner[0] = SOUTH_WEST;
			corner[1] = SOUTH_EAST;
			break;
		case EAST:
			dir = WEST;
			corner[0] = NORTH_EAST;
			corner[1] = SOUTH_EAST;
			break;
		case WEST:
			dir = EAST;
			corner[0] = NORTH_WEST;
			corner[1] = SOUTH_WEST;
			break;
		}

		while ( !newArea->IsOverlapping( *newArea->m_node[ corner[0] ]->GetPosition(), navgenparams->generation_step_size/2 ) )
		{
			for ( int i=0; i<2; ++i )
			{
				Assert( newArea->m_node[ corner[i] ] );
				Assert( newArea->m_node[ corner[i] ]->GetConnectedNode( dir ) );
				newArea->m_node[ corner[i] ] = newArea->m_node[ corner[i] ]->GetConnectedNode( dir );
			}
		}

		// assign internal nodes...
		newArea->AssignNodes( newArea );

		// ... and grab the node heights for our corner heights.
		newArea->m_neZ			= newArea->m_node[ NORTH_EAST ]->GetPosition()->z;
		newArea->m_nwCorner.z	= newArea->m_node[ NORTH_WEST ]->GetPosition()->z;
		newArea->m_swZ			= newArea->m_node[ SOUTH_WEST ]->GetPosition()->z;
		newArea->m_seCorner.z	= newArea->m_node[ SOUTH_EAST ]->GetPosition()->z;
	}
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Create a new area between this area and given area 
 */
bool CNavArea::SpliceEdit( CNavArea *other )
{
	CNavArea *newArea = NULL;
	Vector nw, ne, se, sw;

	if (m_nwCorner.x > other->m_seCorner.x)
	{
		// 'this' is east of 'other'
		float top = MAX( m_nwCorner.y, other->m_nwCorner.y );
		float bottom = MIN( m_seCorner.y, other->m_seCorner.y );

		nw.x = other->m_seCorner.x;
		nw.y = top;
		nw.z = other->GetZ( nw );

		se.x = m_nwCorner.x;
		se.y = bottom;
		se.z = GetZ( se );

		ne.x = se.x;
		ne.y = nw.y;
		ne.z = GetZ( ne );

		sw.x = nw.x;
		sw.y = se.y;
		sw.z = other->GetZ( sw );

		newArea = TheNavMesh->CreateArea();
		if (newArea == NULL)
		{
			Warning( "SpliceEdit: Out of memory.\n" );
			return false;
		}
		
		newArea->Build( nw, ne, se, sw );

		this->ConnectTo( newArea, WEST );
		newArea->ConnectTo( this, EAST );

		other->ConnectTo( newArea, EAST );
		newArea->ConnectTo( other, WEST );
	}
	else if (m_seCorner.x < other->m_nwCorner.x)
	{
		// 'this' is west of 'other'
		float top = MAX( m_nwCorner.y, other->m_nwCorner.y );
		float bottom = MIN( m_seCorner.y, other->m_seCorner.y );

		nw.x = m_seCorner.x;
		nw.y = top;
		nw.z = GetZ( nw );

		se.x = other->m_nwCorner.x;
		se.y = bottom;
		se.z = other->GetZ( se );

		ne.x = se.x;
		ne.y = nw.y;
		ne.z = other->GetZ( ne );

		sw.x = nw.x;
		sw.y = se.y;
		sw.z = GetZ( sw );

		newArea = TheNavMesh->CreateArea();
		if (newArea == NULL)
		{
			Warning( "SpliceEdit: Out of memory.\n" );
			return false;
		}
		
		newArea->Build( nw, ne, se, sw );

		this->ConnectTo( newArea, EAST );
		newArea->ConnectTo( this, WEST );

		other->ConnectTo( newArea, WEST );
		newArea->ConnectTo( other, EAST );
	}
	else	// 'this' overlaps in X
	{
		if (m_nwCorner.y > other->m_seCorner.y)
		{
			// 'this' is south of 'other'
			float left = MAX( m_nwCorner.x, other->m_nwCorner.x );
			float right = MIN( m_seCorner.x, other->m_seCorner.x );

			nw.x = left;
			nw.y = other->m_seCorner.y;
			nw.z = other->GetZ( nw );

			se.x = right;
			se.y = m_nwCorner.y;
			se.z = GetZ( se );

			ne.x = se.x;
			ne.y = nw.y;
			ne.z = other->GetZ( ne );

			sw.x = nw.x;
			sw.y = se.y;
			sw.z = GetZ( sw );

			newArea = TheNavMesh->CreateArea();
			if (newArea == NULL)
			{
				Warning( "SpliceEdit: Out of memory.\n" );
				return false;
			}
			
			newArea->Build( nw, ne, se, sw );

			this->ConnectTo( newArea, NORTH );
			newArea->ConnectTo( this, SOUTH );

			other->ConnectTo( newArea, SOUTH );
			newArea->ConnectTo( other, NORTH );
		}
		else if (m_seCorner.y < other->m_nwCorner.y)
		{
			// 'this' is north of 'other'
			float left = MAX( m_nwCorner.x, other->m_nwCorner.x );
			float right = MIN( m_seCorner.x, other->m_seCorner.x );

			nw.x = left;
			nw.y = m_seCorner.y;
			nw.z = GetZ( nw );

			se.x = right;
			se.y = other->m_nwCorner.y;
			se.z = other->GetZ( se );

			ne.x = se.x;
			ne.y = nw.y;
			ne.z = GetZ( ne );

			sw.x = nw.x;
			sw.y = se.y;
			sw.z = other->GetZ( sw );

			newArea = TheNavMesh->CreateArea();
			if (newArea == NULL)
			{
				Warning( "SpliceEdit: Out of memory.\n" );
				return false;
			}
			
			newArea->Build( nw, ne, se, sw );

			this->ConnectTo( newArea, SOUTH );
			newArea->ConnectTo( this, NORTH );

			other->ConnectTo( newArea, NORTH );
			newArea->ConnectTo( other, SOUTH );
		}
		else
		{
			// areas overlap
			return false;
		}
	}

	newArea->InheritAttributes( this, other );

	TheNavAreas.AddToTail( newArea );
	TheNavMesh->AddNavArea( newArea );
	
	TheNavMesh->OnEditCreateNotify( newArea );

	return true;
}

//--------------------------------------------------------------------------------------------------------------
/**
* Calculates a constant ID for an area at this location, for debugging
*/
void CNavArea::CalcDebugID()
{
	if ( m_debugid == 0 )
	{
		// calculate a debug ID which will be constant for this nav area across generation runs
		int coord[6] = { (int) m_nwCorner.x, (int) m_nwCorner.x, (int) m_nwCorner.z, (int) m_seCorner.x, (int) m_seCorner.y, (int) m_seCorner.z };
		m_debugid = CRC32_ProcessSingleBuffer( &coord, sizeof( coord ) );
	}
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Merge this area and given adjacent area 
 */
bool CNavArea::MergeEdit(CNavMesh* TheNavMesh, CNavArea *adj )
{
	// can only merge if attributes of both areas match


	// check that these areas can be merged
	const float tolerance = 1.0f;
	bool merge = false;
	if (fabs( m_nwCorner.x - adj->m_nwCorner.x ) < tolerance && 
		fabs( m_seCorner.x - adj->m_seCorner.x ) < tolerance)
		merge = true;

	if (fabs( m_nwCorner.y - adj->m_nwCorner.y ) < tolerance && 
		fabs( m_seCorner.y - adj->m_seCorner.y ) < tolerance)
		merge = true;

	if (merge == false)
		return false;

	Vector originalNWCorner = m_nwCorner;
	Vector originalSECorner = m_seCorner;
	
	// update extent
	if (m_nwCorner.x > adj->m_nwCorner.x || m_nwCorner.y > adj->m_nwCorner.y)
		m_nwCorner = adj->m_nwCorner;

	if (m_seCorner.x < adj->m_seCorner.x || m_seCorner.y < adj->m_seCorner.y)
		m_seCorner = adj->m_seCorner;

	m_center.x = (m_nwCorner.x + m_seCorner.x)/2.0f;
	m_center.y = (m_nwCorner.y + m_seCorner.y)/2.0f;
	m_center.z = (m_nwCorner.z + m_seCorner.z)/2.0f;

	if ( ( m_seCorner.x - m_nwCorner.x ) > 0.0f && ( m_seCorner.y - m_nwCorner.y ) > 0.0f )
	{
		m_invDxCorners = 1.0f / ( m_seCorner.x - m_nwCorner.x );
		m_invDyCorners = 1.0f / ( m_seCorner.y - m_nwCorner.y );
	}
	else
	{
		m_invDxCorners = m_invDyCorners = 0;
	}

	if (m_seCorner.x > originalSECorner.x || m_nwCorner.y < originalNWCorner.y)
		m_neZ = adj->GetZ( m_seCorner.x, m_nwCorner.y );
	else
		m_neZ = GetZ( m_seCorner.x, m_nwCorner.y );

	if (m_nwCorner.x < originalNWCorner.x || m_seCorner.y > originalSECorner.y)
		m_swZ = adj->GetZ( m_nwCorner.x, m_seCorner.y );
	else
		m_swZ = GetZ( m_nwCorner.x, m_seCorner.y );

	// merge adjacency links - we gain all the connections that adjArea had
	MergeAdjacentConnections( TheNavMesh, adj );

	InheritAttributes( adj );

	// remove subsumed adjacent area
	TheNavAreas.FindAndRemove( adj );
	TheNavMesh->OnEditDestroyNotify( adj );
	TheNavMesh->DestroyArea( adj );
	
	TheNavMesh->OnEditCreateNotify( this );

	return true;
}

//--------------------------------------------------------------------------------------------------------------
void CNavArea::InheritAttributes( CNavArea *first, CNavArea *second )
{
	if ( first && second )
	{
		SetAttributes( first->GetAttributes() | second->GetAttributes() );

		// if both areas have the same place, the new area inherits it
		if ( first->GetPlace() == second->GetPlace() )
		{
			SetPlace( first->GetPlace() );
		}
		else if ( first->GetPlace() == UNDEFINED_PLACE )
		{
			SetPlace( second->GetPlace() );
		}
		else if ( second->GetPlace() == UNDEFINED_PLACE )
		{
			SetPlace( first->GetPlace() );
		}
		else
		{
			// both have valid, but different places - pick on at random
			if (randomgen->GetRandomInt<int>(0, 1) == 1)
				SetPlace( first->GetPlace() );
			else
				SetPlace( second->GetPlace() );
		}
	}
	else if ( first )
	{
		SetAttributes( GetAttributes() | first->GetAttributes() );
		if ( GetPlace() == UNDEFINED_PLACE )
		{
			SetPlace( first->GetPlace() );
		}
	}
}

//--------------------------------------------------------------------------------------------------------------
void ApproachAreaAnalysisPrep( void )
{
}

//--------------------------------------------------------------------------------------------------------------
void CleanupApproachAreaAnalysisPrep( void )
{
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Remove "analyzed" data from nav area
 */
void CNavArea::Strip( void )
{
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Return true if area is more or less square.
 * This is used when merging to prevent long, thin, areas being created.
 */
bool CNavArea::IsRoughlySquare( void ) const
{
	float aspect = GetSizeX() / GetSizeY();

	const float maxAspect = 3.01;
	const float minAspect = 1.0f / maxAspect;
	if (aspect < minAspect || aspect > maxAspect)
		return false;

	return true;
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Return true if 'pos' is within 2D extents of area.
 */
bool CNavArea::IsOverlapping( const Vector &pos, float tolerance ) const
{
	if (pos.x + tolerance >= m_nwCorner.x && pos.x - tolerance <= m_seCorner.x &&
		pos.y + tolerance >= m_nwCorner.y && pos.y - tolerance <= m_seCorner.y)
		return true;

	return false;
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Return true if 'area' overlaps our 2D extents
 */
bool CNavArea::IsOverlapping( const CNavArea *area ) const
{
	if (area->m_nwCorner.x < m_seCorner.x && area->m_seCorner.x > m_nwCorner.x && 
		area->m_nwCorner.y < m_seCorner.y && area->m_seCorner.y > m_nwCorner.y)
		return true;

	return false;
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Return true if 'extent' overlaps our 2D extents
 */
bool CNavArea::IsOverlapping( const Extent &extent ) const
{
	return ( extent.lo.x < m_seCorner.x && extent.hi.x > m_nwCorner.x && 
			 extent.lo.y < m_seCorner.y && extent.hi.y > m_nwCorner.y );
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Return true if 'area' overlaps our X extent
 */
bool CNavArea::IsOverlappingX( const CNavArea *area ) const
{
	if (area->m_nwCorner.x < m_seCorner.x && area->m_seCorner.x > m_nwCorner.x)
		return true;

	return false;
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Return true if 'area' overlaps our Y extent
 */
bool CNavArea::IsOverlappingY( const CNavArea *area ) const
{
	if (area->m_nwCorner.y < m_seCorner.y && area->m_seCorner.y > m_nwCorner.y)
		return true;

	return false;
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Return true if given point is on or above this area, but no others
 */
bool CNavArea::Contains( const Vector &pos ) const
{
	// check 2D overlap
	if (!IsOverlapping( pos ))
		return false;

	// the point overlaps us, check that it is above us, but not above any areas that overlap us
	float myZ = GetZ( pos );

	// if the nav area is above the given position, fail
	// allow nav area to be as much as a step height above the given position
	if (myZ - navgenparams->step_height > pos.z)
		return false;

	Extent areaExtent;
	GetExtent( &areaExtent );

	COverlapCheck overlap( this, pos );
	return TheNavMesh->ForAllAreasOverlappingExtent( overlap, areaExtent );
}


//--------------------------------------------------------------------------------------------------------------
/**
* Returns true if area completely contains other area
*/
bool CNavArea::Contains( const CNavArea *area ) const
{
	return ( ( m_nwCorner.x <= area->m_nwCorner.x ) && ( m_seCorner.x >= area->m_seCorner.x ) &&
		( m_nwCorner.y <= area->m_nwCorner.y ) && ( m_seCorner.y >= area->m_seCorner.y ) &&
		( m_nwCorner.z <= area->m_nwCorner.z ) && ( m_seCorner.z >= area->m_seCorner.z ) );
}


//--------------------------------------------------------------------------------------------------------------
void CNavArea::ComputeNormal( Vector *normal, bool alternate ) const
{
	if ( !normal )
		return;

	Vector u, v;

	if ( !alternate )
	{
		u.x = m_seCorner.x - m_nwCorner.x;
		u.y = 0.0f;
		u.z = m_neZ - m_nwCorner.z;

		v.x = 0.0f;
		v.y = m_seCorner.y - m_nwCorner.y;
		v.z = m_swZ - m_nwCorner.z;
	}
	else
	{
		u.x = m_nwCorner.x - m_seCorner.x;
		u.y = 0.0f;
		u.z = m_swZ - m_seCorner.z;

		v.x = 0.0f;
		v.y = m_nwCorner.y - m_seCorner.y;
		v.z = m_neZ - m_seCorner.z;
	}

	*normal = CrossProduct( u, v );
	normal->NormalizeInPlace();
}


//--------------------------------------------------------------------------------------------------------------
/**
* Removes all connections in directions to left and right of specified direction
*/
void CNavArea::RemoveOrthogonalConnections( NavDirType dir )
{
	NavDirType dirToRemove[2];
	dirToRemove[0] = DirectionLeft( dir );
	dirToRemove[1] = DirectionRight( dir );
	for ( int i = 0; i < 2; i++ )
	{
		dir = dirToRemove[i];
		while ( GetAdjacentCount( dir ) > 0 )
		{
			CNavArea *adj = GetAdjacentArea( dir, 0 );
			Disconnect( adj );
			adj->Disconnect( this );
		}
	}
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Return true if the area is approximately flat, using normals computed from opposite corners
 */
bool CNavArea::IsFlat( void ) const
{
	Vector normal, otherNormal;
	ComputeNormal( &normal );
	ComputeNormal( &otherNormal, true );

	float tolerance = sm_nav_coplanar_slope_limit.GetFloat();
	if ( ( m_node[ NORTH_WEST ] && m_node[ NORTH_WEST ]->IsOnDisplacement() ) ||
		( m_node[ NORTH_EAST ] && m_node[ NORTH_EAST ]->IsOnDisplacement() ) ||
		( m_node[ SOUTH_EAST ] && m_node[ SOUTH_EAST ]->IsOnDisplacement() ) ||
		( m_node[ SOUTH_WEST ] && m_node[ SOUTH_WEST ]->IsOnDisplacement() ) )
	{
		tolerance = sm_nav_coplanar_slope_limit_displacement.GetFloat();
	}

	if (DotProduct( normal, otherNormal ) > tolerance)
		return true;

	return false;
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Return true if this area and given area are approximately co-planar
 */
bool CNavArea::IsCoplanar( const CNavArea *area ) const
{
	// Vector u, v;

	bool isOnDisplacement = ( m_node[ NORTH_WEST ] && m_node[ NORTH_WEST ]->IsOnDisplacement() ) ||
		( m_node[ NORTH_EAST ] && m_node[ NORTH_EAST ]->IsOnDisplacement() ) ||
		( m_node[ SOUTH_EAST ] && m_node[ SOUTH_EAST ]->IsOnDisplacement() ) ||
		( m_node[ SOUTH_WEST ] && m_node[ SOUTH_WEST ]->IsOnDisplacement() );

	if ( !isOnDisplacement && !IsFlat() )
		return false;

	bool areaIsOnDisplacement = ( area->m_node[ NORTH_WEST ] && area->m_node[ NORTH_WEST ]->IsOnDisplacement() ) ||
		( area->m_node[ NORTH_EAST ] && area->m_node[ NORTH_EAST ]->IsOnDisplacement() ) ||
		( area->m_node[ SOUTH_EAST ] && area->m_node[ SOUTH_EAST ]->IsOnDisplacement() ) ||
		( area->m_node[ SOUTH_WEST ] && area->m_node[ SOUTH_WEST ]->IsOnDisplacement() );

	if ( !areaIsOnDisplacement && !area->IsFlat() )
		return false;

	// compute our unit surface normal
	Vector normal, otherNormal;
	ComputeNormal( &normal );
	area->ComputeNormal( &otherNormal );

	// can only merge areas that are nearly planar, to ensure areas do not differ from underlying geometry much
	float tolerance = sm_nav_coplanar_slope_limit.GetFloat();
	if ( ( m_node[ NORTH_WEST ] && m_node[ NORTH_WEST ]->IsOnDisplacement() ) ||
		( m_node[ NORTH_EAST ] && m_node[ NORTH_EAST ]->IsOnDisplacement() ) ||
		( m_node[ SOUTH_EAST ] && m_node[ SOUTH_EAST ]->IsOnDisplacement() ) ||
		( m_node[ SOUTH_WEST ] && m_node[ SOUTH_WEST ]->IsOnDisplacement() ) )
	{
		tolerance = sm_nav_coplanar_slope_limit_displacement.GetFloat();
	}

	if (DotProduct( normal, otherNormal ) > tolerance)
		return true;

	return false;
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Return Z of area at (x,y) of 'pos'
 * Trilinear interpolation of Z values at quad edges.
 * NOTE: pos->z is not used.
 */

float CNavArea::GetZ( float x, float y ) const
{
	// guard against division by zero due to degenerate areas
#ifdef _X360 
	// do the compare-against-zero on the integer unit to avoid a fcmp
	// IEEE754 float positive zero is simply 0x00. There is also a 
	// floating-point negative zero (-0.0f == 0x80000000), but given 
	// how m_inv is computed earlier, that's not a possible value for
	// it here, so we don't have to check for that.
	//
	// oddly, the compiler isn't smart enough to do this on its own
	if ( *reinterpret_cast<const unsigned *>(&m_invDxCorners) == 0 ||
		 *reinterpret_cast<const unsigned *>(&m_invDyCorners) == 0 )
		return m_neZ;
#else
	if (m_invDxCorners == 0.0f || m_invDyCorners == 0.0f)
		return m_neZ;
#endif

	float u = (x - m_nwCorner.x) * m_invDxCorners;
	float v = (y - m_nwCorner.y) * m_invDyCorners;

	// clamp Z values to (x,y) volume
	
	u = fsel( u, u, 0.0f );			// u >= 0 ? u : 0
	u = fsel( u - 1.0f, 1.0f, u );	// u >= 1 ? 1 : u

	v = fsel( v, v, 0.0f );			// v >= 0 ? v : 0
	v = fsel( v - 1.0f, 1.0f, v );	// v >= 1 ? 1 : v

	float northZ = m_nwCorner.z + u * (m_neZ - m_nwCorner.z);
	float southZ = m_swZ + u * (m_seCorner.z - m_swZ);

	return northZ + v * (southZ - northZ);
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Return closest point to 'pos' on 'area'.
 * Returned point is in 'close'.
 */
void CNavArea::GetClosestPointOnArea( const Vector* pPos, Vector *close ) const
{
	float x, y, z;

	// Using fsel rather than compares, as much faster on 360 [7/28/2008 tom]
	x = fsel( pPos->x - m_nwCorner.x, pPos->x, m_nwCorner.x );
	x = fsel( x - m_seCorner.x, m_seCorner.x, x );

	y = fsel( pPos->y - m_nwCorner.y, pPos->y, m_nwCorner.y );
	y = fsel( y - m_seCorner.y, m_seCorner.y, y );

	z = GetZ( x, y );

	close->Init( x, y, z );
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Return shortest distance squared between point and this area
 */
float CNavArea::GetDistanceSquaredToPoint( const Vector &pos ) const
{
	if (pos.x < m_nwCorner.x)
	{
		if (pos.y < m_nwCorner.y)
		{
			// position is north-west of area
			return (m_nwCorner - pos).LengthSqr();
		}
		else if (pos.y > m_seCorner.y)
		{
			// position is south-west of area
			Vector d;
			d.x = m_nwCorner.x - pos.x;
			d.y = m_seCorner.y - pos.y;
			d.z = m_swZ - pos.z;
			return d.LengthSqr();
		}
		else
		{
			// position is west of area
			float d = m_nwCorner.x - pos.x;
			return d * d;
		}
	}
	else if (pos.x > m_seCorner.x)
	{
		if (pos.y < m_nwCorner.y)
		{
			// position is north-east of area
			Vector d;
			d.x = m_seCorner.x - pos.x;
			d.y = m_nwCorner.y - pos.y;
			d.z = m_neZ - pos.z;
			return d.LengthSqr();
		}
		else if (pos.y > m_seCorner.y)
		{
			// position is south-east of area
			return (m_seCorner - pos).LengthSqr();
		}
		else
		{
			// position is east of area
			float d = pos.x - m_seCorner.x;
			return d * d;
		}
	}
	else if (pos.y < m_nwCorner.y)
	{
		// position is north of area
		float d = m_nwCorner.y - pos.y;
		return d * d;
	}
	else if (pos.y > m_seCorner.y)
	{
		// position is south of area
		float d = pos.y - m_seCorner.y;
		return d * d;
	}
	else
	{
		// position is inside of 2D extent of area - find delta Z
		float z = GetZ( pos );
		float d = z - pos.z;
		return d * d;
	}
}



//--------------------------------------------------------------------------------------------------------------
CNavArea *CNavArea::GetRandomAdjacentArea( NavDirType dir ) const
{
	int count = m_connect[ dir ].Count();
	int which = randomgen->GetRandomInt<int>(0, count - 1);

	int i = 0;
	FOR_EACH_VEC( m_connect[ dir ], it )
	{
		if (i == which)
			return m_connect[ dir ][ it ].area;

		++i;
	}

	return NULL;
}


//--------------------------------------------------------------------------------------------------------------
// Build a vector of all adjacent areas
void CNavArea::CollectAdjacentAreas( CUtlVector< CNavArea * > *adjVector ) const
{
	for( int d=0; d<NUM_DIRECTIONS; ++d )
	{
		for( int i=0; i<m_connect[d].Count(); ++i )
		{
			adjVector->AddToTail( m_connect[d].Element(i).area );
		}
	}
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Compute "portal" between two adjacent areas. 
 * Return center of portal opening, and half-width defining sides of portal from center.
 * NOTE: center->z is unset.
 */
void CNavArea::ComputePortal( const CNavArea *to, NavDirType dir, Vector *center, float *halfWidth ) const
{
	if ( dir == NORTH || dir == SOUTH )
	{
		if ( dir == NORTH )
		{
			center->y = m_nwCorner.y;
		}
		else
		{
			center->y = m_seCorner.y;
		}

		float left = MAX( m_nwCorner.x, to->m_nwCorner.x );
		float right = MIN( m_seCorner.x, to->m_seCorner.x );

		// clamp to our extent in case areas are disjoint
		if ( left < m_nwCorner.x )
		{
			left = m_nwCorner.x;
		}
		else if ( left > m_seCorner.x )
		{
			left = m_seCorner.x;
		}

		if ( right < m_nwCorner.x )
		{
			right = m_nwCorner.x;
		}
		else if ( right > m_seCorner.x )
		{
			right = m_seCorner.x;
		}

		center->x = ( left + right )/2.0f;
		*halfWidth = ( right - left )/2.0f;
	}
	else	// EAST or WEST
	{
		if ( dir == WEST )
		{
			center->x = m_nwCorner.x;
		}
		else
		{
			center->x = m_seCorner.x;
		}

		float top = MAX( m_nwCorner.y, to->m_nwCorner.y );
		float bottom = MIN( m_seCorner.y, to->m_seCorner.y );

		// clamp to our extent in case areas are disjoint
		if ( top < m_nwCorner.y )
		{
			top = m_nwCorner.y;
		}
		else if ( top > m_seCorner.y )
		{
			top = m_seCorner.y;
		}

		if ( bottom < m_nwCorner.y )
		{
			bottom = m_nwCorner.y;
		}
		else if ( bottom > m_seCorner.y )
		{
			bottom = m_seCorner.y;
		}

		center->y = (top + bottom)/2.0f;
		*halfWidth = (bottom - top)/2.0f;
	}

	center->z = GetZ( center->x, center->y );
}


//--------------------------------------------------------------------------------------------------------------
// compute largest portal to adjacent area, returning direction
NavDirType CNavArea::ComputeLargestPortal( const CNavArea *to, Vector *center, float *halfWidth ) const
{
	NavDirType bestDir = NUM_DIRECTIONS;
	Vector bestCenter( vec3_origin );
	float bestHalfWidth = 0.0f;

	Vector centerDir = to->GetCenter() - GetCenter();

	for ( int i=0; i<NUM_DIRECTIONS; ++i )
	{
		NavDirType testDir = (NavDirType)i;
		Vector testCenter;
		float testHalfWidth;

		// Make sure we're not picking the opposite direction
		switch ( testDir )
		{
		case NORTH:	// -y
			if ( centerDir.y >= 0.0f )
				continue;
			break;
		case SOUTH:	// +y
			if ( centerDir.y <= 0.0f )
				continue;
			break;
		case WEST:	// -x
			if ( centerDir.x >= 0.0f )
				continue;
			break;
		case EAST:	// +x
			if ( centerDir.x <= 0.0f )
				continue;
			break;
		}

		ComputePortal( to, testDir, &testCenter, &testHalfWidth );
		if ( testHalfWidth > bestHalfWidth )
		{
			bestDir = testDir;
			bestCenter = testCenter;
			bestHalfWidth = testHalfWidth;
		}
	}

	*center = bestCenter;
	*halfWidth = bestHalfWidth;
	return bestDir;
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Compute closest point within the "portal" between to adjacent areas. 
 */
void CNavArea::ComputeClosestPointInPortal( const CNavArea *to, NavDirType dir, const Vector &fromPos, Vector *closePos ) const
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("[NavBot] CNavArea::ComputeClosestPointInPortal", "NavBot");
#endif // EXT_VPROF_ENABLED

//	const float margin = 0.0f; //navgenparams->generation_step_size/2.0f;  // causes trouble with very small/narrow nav areas
	const float margin = navgenparams->generation_step_size;

	if ( dir == NORTH || dir == SOUTH )
	{
		if ( dir == NORTH )
		{
			closePos->y = m_nwCorner.y;
		}
		else
		{
			closePos->y = m_seCorner.y;
		}

		float left = MAX( m_nwCorner.x, to->m_nwCorner.x );
		float right = MIN( m_seCorner.x, to->m_seCorner.x );

		// clamp to our extent in case areas are disjoint
		// no good - need to push into to area for margins
		/*
		if (left < m_nwCorner.x)
			left = m_nwCorner.x;
		else if (left > m_seCorner.x)
			left = m_seCorner.x;

		if (right < m_nwCorner.x)
			right = m_nwCorner.x;
		else if (right > m_seCorner.x)
			right = m_seCorner.x;
			*/

		// keep margin if against edge
		/// @todo Need better check whether edge is outer edge or not - partial overlap is missed
		float leftMargin = ( to->IsEdge( WEST ) ) ? ( left + margin ) : left;
		float rightMargin = ( to->IsEdge( EAST ) ) ? ( right - margin ) : right;
		
		// if area is narrow, margins may have crossed
		if ( leftMargin > rightMargin )
		{
			// use midline
			float mid = ( left + right )/2.0f;
			leftMargin = mid;
			rightMargin = mid;
		}

		// limit x to within portal
		if ( fromPos.x < leftMargin )
		{
			closePos->x = leftMargin;
		}
		else if ( fromPos.x > rightMargin )
		{
			closePos->x = rightMargin;
		}
		else
		{
			closePos->x = fromPos.x;
		}
	}
	else	// EAST or WEST
	{
		if ( dir == WEST )
		{
			closePos->x = m_nwCorner.x;
		}
		else
		{
			closePos->x = m_seCorner.x;
		}

		float top = MAX( m_nwCorner.y, to->m_nwCorner.y );
		float bottom = MIN( m_seCorner.y, to->m_seCorner.y );

		// clamp to our extent in case areas are disjoint
		// no good - need to push into to area for margins
		/*
		if (top < m_nwCorner.y)
			top = m_nwCorner.y;
		else if (top > m_seCorner.y)
			top = m_seCorner.y;

		if (bottom < m_nwCorner.y)
			bottom = m_nwCorner.y;
		else if (bottom > m_seCorner.y)
			bottom = m_seCorner.y;
		*/
		
		// keep margin if against edge
		float topMargin = ( to->IsEdge( NORTH ) ) ? ( top + margin ) : top;
		float bottomMargin = ( to->IsEdge( SOUTH ) ) ? ( bottom - margin ) : bottom;

		// if area is narrow, margins may have crossed
		if ( topMargin > bottomMargin )
		{
			// use midline
			float mid = ( top + bottom )/2.0f;
			topMargin = mid;
			bottomMargin = mid;
		}

		// limit y to within portal
		if ( fromPos.y < topMargin )
		{
			closePos->y = topMargin;
		}
		else if ( fromPos.y > bottomMargin )
		{
			closePos->y = bottomMargin;
		}
		else
		{
			closePos->y = fromPos.y;
		}
	}

	closePos->z = GetZ( closePos->x, closePos->y );
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Return true if the given area and 'other' share a colinear edge (ie: no drop-down or step/jump/climb)
 */
bool CNavArea::IsContiguous( const CNavArea *other ) const
{
	// find which side it is connected on
	int dir;
	for( dir=0; dir<NUM_DIRECTIONS; ++dir )
	{
		if ( IsConnected( other, (NavDirType)dir ) )
			break;
	}

	if ( dir == NUM_DIRECTIONS )
		return false;

	Vector myEdge;
	float halfWidth;
	ComputePortal( other, (NavDirType)dir, &myEdge, &halfWidth );

	Vector otherEdge;
	other->ComputePortal( this, OppositeDirection( (NavDirType)dir ), &otherEdge, &halfWidth );

	// must use stepheight because rough terrain can have gaps/cracks between adjacent nav areas
	return ( myEdge - otherEdge ).IsLengthLessThan( navgenparams->step_height );
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Return height change between edges of adjacent nav areas (not actual underlying ground)
 */
float CNavArea::ComputeAdjacentConnectionHeightChange( const CNavArea *destinationArea ) const
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("[NavBot] CNavArea::ComputeAdjacentConnectionHeightChange", "NavBot");
#endif // EXT_VPROF_ENABLED

	// find which side it is connected on
	int dir;
	for( dir=0; dir<NUM_DIRECTIONS; ++dir )
	{
		if ( IsConnected( destinationArea, (NavDirType)dir ) )
			break;
	}

	if (dir == NUM_DIRECTIONS)
	{
		// no direction, check special links
		if (IsConnectedToBySpecialLink(destinationArea))
		{
			return GetOffMeshConnectionToArea(destinationArea)->GetConnectionLength();
		}

		return FLT_MAX;
	}

	Vector myEdge;
	float halfWidth;
	ComputePortal( destinationArea, (NavDirType)dir, &myEdge, &halfWidth );

	Vector otherEdge;
	destinationArea->ComputePortal( this, OppositeDirection( (NavDirType)dir ), &otherEdge, &halfWidth );

	return otherEdge.z - myEdge.z;
}

float CNavArea::ComputeAdjacentConnectionGapDistance(const CNavArea* destinationArea) const
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("[NavBot] CNavArea::ComputeAdjacentConnectionGapDistance", "NavBot");
#endif // EXT_VPROF_ENABLED

	// find which side it is connected on
	int dir;
	for (dir = 0; dir < NUM_DIRECTIONS; ++dir)
	{
		if (IsConnected(destinationArea, (NavDirType)dir))
			break;
	}

	if (dir == NUM_DIRECTIONS)
	{
		// no direction, check special links
		if (IsConnectedToBySpecialLink(destinationArea))
		{
			return GetOffMeshConnectionToArea(destinationArea)->GetConnectionLength();
		}

		if (m_elevator != nullptr)
		{
			return m_elevator->GetLengthBetweenFloors(this, destinationArea);
		}

		return FLT_MAX;
	}

	Vector myEdge;
	float halfWidth;
	ComputePortal(destinationArea, (NavDirType)dir, &myEdge, &halfWidth);

	Vector otherEdge;
	destinationArea->ComputePortal(this, OppositeDirection((NavDirType)dir), &otherEdge, &halfWidth);

	return (otherEdge - myEdge).AsVector2D().Length();
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Return true if there are no bi-directional links on the given side
 */
bool CNavArea::IsEdge( NavDirType dir ) const
{
	FOR_EACH_VEC( m_connect[ dir ], it )
	{
		const NavConnect connect = m_connect[ dir ][ it ];

		if (connect.area->IsConnected( this, OppositeDirection( dir ) ))
			return false;
	}

	return true;
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Return direction from this area to the given point
 */
NavDirType CNavArea::ComputeDirection( Vector *point ) const
{
	if (point->x >= m_nwCorner.x && point->x <= m_seCorner.x)
	{
		if (point->y < m_nwCorner.y)
			return NORTH;
		else if (point->y > m_seCorner.y)
			return SOUTH;
	}
	else if (point->y >= m_nwCorner.y && point->y <= m_seCorner.y)
	{
		if (point->x < m_nwCorner.x)
			return WEST;
		else if (point->x > m_seCorner.x)
			return EAST;
	}

	// find closest direction
	Vector to = *point - m_center;

	if (fabs(to.x) > fabs(to.y))
	{
		if (to.x > 0.0f)
			return EAST;
		return WEST;
	}
	else
	{
		if (to.y > 0.0f)
			return SOUTH;
		return NORTH;
	}

	return NUM_DIRECTIONS;
}

const HidingSpot* CNavArea::GetRandomHidingSpot() const
{
	if (m_hidingSpots.Count() == 0) { return nullptr; }
	if (m_hidingSpots.Count() == 1) { return m_hidingSpots[0]; }
	return m_hidingSpots[randomgen->GetRandomInt<int>(0, m_hidingSpots.Count() - 1)];
}

//--------------------------------------------------------------------------------------------------------------
bool CNavArea::GetCornerHotspot( NavCornerType corner, Vector hotspot[NUM_CORNERS] ) const
{
	Vector nw = GetCorner( NORTH_WEST );
	Vector ne = GetCorner( NORTH_EAST );
	Vector sw = GetCorner( SOUTH_WEST );
	Vector se = GetCorner( SOUTH_EAST );

	float size = 9.0f;
	size = MIN( size, GetSizeX()/3 );	// make sure the hotspot doesn't extend outside small areas
	size = MIN( size, GetSizeY()/3 );

	switch ( corner )
	{
	case NORTH_WEST:
		hotspot[0] = nw;
		hotspot[1] = hotspot[0] + Vector( size, 0, 0 );
		hotspot[2] = hotspot[0] + Vector( size, size, 0 );
		hotspot[3] = hotspot[0] + Vector( 0, size, 0 );
		break;
	case NORTH_EAST:
		hotspot[0] = ne;
		hotspot[1] = hotspot[0] + Vector( -size, 0, 0 );
		hotspot[2] = hotspot[0] + Vector( -size, size, 0 );
		hotspot[3] = hotspot[0] + Vector( 0, size, 0 );
		break;
	case SOUTH_WEST:
		hotspot[0] = sw;
		hotspot[1] = hotspot[0] + Vector( size, 0, 0 );
		hotspot[2] = hotspot[0] + Vector( size, -size, 0 );
		hotspot[3] = hotspot[0] + Vector( 0, -size, 0 );
		break;
	case SOUTH_EAST:
		hotspot[0] = se;
		hotspot[1] = hotspot[0] + Vector( -size, 0, 0 );
		hotspot[2] = hotspot[0] + Vector( -size, -size, 0 );
		hotspot[3] = hotspot[0] + Vector( 0, -size, 0 );
		break;
	default:
		return false;
	}

	for ( int i=1; i<NUM_CORNERS; ++i )
	{
		hotspot[i].z = GetZ( hotspot[i] );
	}

	Vector eyePos, eyeForward;
	TheNavMesh->GetEditVectors( &eyePos, &eyeForward );

	Ray_t ray;
	ray.Init( eyePos, eyePos + 10000.0f * eyeForward, vec3_origin, vec3_origin );

	float dist = IntersectRayWithTriangle( ray, hotspot[0], hotspot[1], hotspot[2], false );
	if ( dist > 0 )
	{
		return true;
	}

	dist = IntersectRayWithTriangle( ray, hotspot[2], hotspot[3], hotspot[0], false );
	if ( dist > 0 )
	{
		return true;
	}

	return false;
}


//--------------------------------------------------------------------------------------------------------------
NavCornerType CNavArea::GetCornerUnderCursor( void ) const
{
	Vector eyePos, eyeForward;
	TheNavMesh->GetEditVectors( &eyePos, &eyeForward );

	for ( int i=0; i<NUM_CORNERS; ++i )
	{
		Vector hotspot[NUM_CORNERS];
		if ( GetCornerHotspot( (NavCornerType)i, hotspot ) )
		{
			return (NavCornerType)i;
		}
	}

	return NUM_CORNERS;
}

const char *UTIL_VarArgs( const char *format, ... )
{
	va_list		argptr;
	static char		string[1024];

	va_start (argptr, format);
	Q_vsnprintf(string, sizeof(string), format,argptr);
	va_end (argptr);

	return string;
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Draw area for debugging
 */
void CNavArea::Draw( void ) const
{
	NavEditColor color;
	bool useAttributeColors = true;

	const float DebugDuration = NDEBUG_PERSIST_FOR_ONE_TICK;

	if ( TheNavMesh->IsEditMode( CNavMesh::PLACE_PAINTING ) )
	{
		useAttributeColors = false;

		if ( m_place == UNDEFINED_PLACE )
		{
			color = NavNoPlaceColor;
		}
		else if ( TheNavMesh->GetNavPlace() == m_place )
		{
			color = NavSamePlaceColor;
		}
		else
		{
			color = NavDifferentPlaceColor;
		}
	}
	else
	{
		// normal edit mode
		if ( this == TheNavMesh->GetMarkedArea() )
		{
			useAttributeColors = false;
			color = NavMarkedColor;
		}
		else if ( this == TheNavMesh->GetSelectedArea() )
		{
			color = NavSelectedColor;
		}
		else
		{
			color = NavNormalColor;
		}
	}

	if ( IsDegenerate() )
	{
		static IntervalTimer blink;
		static bool blinkOn = false;

		if (blink.GetElapsedTime() > 1.0f)
		{
			blink.Reset();
			blinkOn = !blinkOn;
		}

		useAttributeColors = false;

		if (blinkOn)
			color = NavDegenerateFirstColor;
		else
			color = NavDegenerateSecondColor;

		NDebugOverlay::Text(GetCenter(), true, DebugDuration, "Degenerate area %u", GetID());
	}

	Vector nw, ne, sw, se;

	nw = m_nwCorner;
	se = m_seCorner;
	ne.x = se.x;
	ne.y = nw.y;
	ne.z = m_neZ;
	sw.x = nw.x;
	sw.y = se.y;
	sw.z = m_swZ;

#ifdef NAVMESH_REMOVED_FEATURES
	if (sm_nav_show_light_intensity.GetBool())
	{
		for (int i = 0; i < NUM_CORNERS; ++i)
		{
			Vector pos = GetCorner((NavCornerType)i);
			Vector end = pos;
			float lightIntensity = GetLightIntensity(pos);
			end.z += navgenparams->human_height * lightIntensity;
			lightIntensity *= 255; // for color
			debugoverlay->AddLineOverlay(end, pos, lightIntensity, lightIntensity, MAX(192, lightIntensity), true, DebugDuration);
		}
	}
#endif // NAVMESH_REMOVED_FEATURES

	int bgcolor[4];
	if ( 4 == sscanf(sm_nav_area_bgcolor.GetString(), "%d %d %d %d", &(bgcolor[0]), &(bgcolor[1]), &(bgcolor[2]), &(bgcolor[3]) ) )
	{
		for ( int i=0; i<4; ++i )
			bgcolor[i] = clamp( bgcolor[i], 0, 255 );

		if ( bgcolor[3] > 0 )
		{
			const Vector offset( 0, 0, 0.8f );
			debugoverlay->AddTriangleOverlay( nw+offset, se+offset, ne+offset, bgcolor[0], bgcolor[1], bgcolor[2], bgcolor[3], true, DebugDuration );
			debugoverlay->AddTriangleOverlay( se+offset, nw+offset, sw+offset, bgcolor[0], bgcolor[1], bgcolor[2], bgcolor[3], true, DebugDuration );
		}
	}

	const float inset = 0.2f;
	nw.x += inset;
	nw.y += inset;
	ne.x -= inset;
	ne.y += inset;
	sw.x += inset;
	sw.y -= inset;
	se.x -= inset;
	se.y -= inset;

	if ( GetAttributes() & NAV_MESH_TRANSIENT )
	{
		NavDrawDashedLine( nw, ne, color );
		NavDrawDashedLine( ne, se, color );
		NavDrawDashedLine( se, sw, color );
		NavDrawDashedLine( sw, nw, color );
	}
	else
	{
		NavDrawLine( nw, ne, color );
		NavDrawLine( ne, se, color );
		NavDrawLine( se, sw, color );
		NavDrawLine( sw, nw, color );
	}

	if ( this == TheNavMesh->GetMarkedArea() && TheNavMesh->m_markedCorner != NUM_CORNERS )
	{
		Vector p[NUM_CORNERS];
		GetCornerHotspot( TheNavMesh->m_markedCorner, p );

		NavDrawLine( p[1], p[2], NavMarkedColor );
		NavDrawLine( p[2], p[3], NavMarkedColor );
	}
	if ( this != TheNavMesh->GetMarkedArea() && this == TheNavMesh->GetSelectedArea() && TheNavMesh->IsEditMode( CNavMesh::NORMAL ) )
	{
		NavCornerType bestCorner = GetCornerUnderCursor();

		Vector p[NUM_CORNERS];
		if ( GetCornerHotspot( bestCorner, p ) )
		{
			NavDrawLine( p[1], p[2], NavSelectedColor );
			NavDrawLine( p[2], p[3], NavSelectedColor );
		}
	}

	if (GetAttributes() & NAV_MESH_CROUCH)
	{
		if ( useAttributeColors )
			color = NavAttributeCrouchColor;

		NavDrawLine( nw, se, color );
	}

	if (GetAttributes() & NAV_MESH_JUMP)
	{
		if ( useAttributeColors )
			color = NavAttributeJumpColor;

		if ( !(GetAttributes() & NAV_MESH_CROUCH) )
		{
			NavDrawLine( nw, se, color );
		}
		NavDrawLine( ne, sw, color );
	}

	if (GetAttributes() & NAV_MESH_PRECISE)
	{
		if ( useAttributeColors )
			color = NavAttributePreciseColor;

		float size = 8.0f;
		Vector up( m_center.x, m_center.y - size, m_center.z );
		Vector down( m_center.x, m_center.y + size, m_center.z );
		NavDrawLine( up, down, color );

		Vector left( m_center.x - size, m_center.y, m_center.z );
		Vector right( m_center.x + size, m_center.y, m_center.z );
		NavDrawLine( left, right, color );
	}

	if (GetAttributes() & NAV_MESH_NO_JUMP)
	{
		if ( useAttributeColors )
			color = NavAttributeNoJumpColor;

		float size = 8.0f;
		Vector up( m_center.x, m_center.y - size, m_center.z );
		Vector down( m_center.x, m_center.y + size, m_center.z );
		Vector left( m_center.x - size, m_center.y, m_center.z );
		Vector right( m_center.x + size, m_center.y, m_center.z );
		NavDrawLine( up, right, color );
		NavDrawLine( right, down, color );
		NavDrawLine( down, left, color );
		NavDrawLine( left, up, color );
	}

	if (GetAttributes() & NAV_MESH_STAIRS)
	{
		if ( useAttributeColors )
			color = NavAttributeStairColor;

		float northZ = ( GetCorner( NORTH_WEST ).z + GetCorner( NORTH_EAST ).z ) / 2.0f;
		float southZ = ( GetCorner( SOUTH_WEST ).z + GetCorner( SOUTH_EAST ).z ) / 2.0f;
		float westZ = ( GetCorner( NORTH_WEST ).z + GetCorner( SOUTH_WEST ).z ) / 2.0f;
		float eastZ = ( GetCorner( NORTH_EAST ).z + GetCorner( SOUTH_EAST ).z ) / 2.0f;

		float deltaEastWest = fabs( westZ - eastZ );
		float deltaNorthSouth = fabs( northZ - southZ );

		float stepSize = navgenparams->step_height / 2.0f;
		float t;

		if ( deltaEastWest > deltaNorthSouth )
		{
			float inc = stepSize / GetSizeX();

			for( t = 0.0f; t <= 1.0f; t += inc )
			{
				float x = m_nwCorner.x + t * GetSizeX();
				
				NavDrawLine( Vector( x, m_nwCorner.y, GetZ( x, m_nwCorner.y ) ), 
							 Vector( x, m_seCorner.y, GetZ( x, m_seCorner.y ) ),
							 color );
			}
		}
		else
		{
			float inc = stepSize / GetSizeY();

			for( t = 0.0f; t <= 1.0f; t += inc )
			{
				float y = m_nwCorner.y + t * GetSizeY();

				NavDrawLine( Vector( m_nwCorner.x, y, GetZ( m_nwCorner.x, y ) ),
							 Vector( m_seCorner.x, y, GetZ( m_seCorner.x, y ) ),
							 color );
			}
		}
	}

	// Stop is represented by an octagon
	if (GetAttributes() & NAV_MESH_STOP)
	{
		if ( useAttributeColors )
			color = NavAttributeStopColor;

		float dist = 8.0f;
		float length = dist/2.5f;
		Vector start, end;

		start =	m_center + Vector( dist, -length, 0 );
		end =	m_center + Vector( dist,  length, 0 );
		NavDrawLine( start, end, color );

		start =	m_center + Vector(   dist, length, 0 );
		end =	m_center + Vector( length, dist,   0 );
		NavDrawLine( start, end, color );

		start =	m_center + Vector( -dist, -length, 0 );
		end =	m_center + Vector( -dist,  length, 0 );
		NavDrawLine( start, end, color );

		start =	m_center + Vector( -dist,   length, 0 );
		end =	m_center + Vector( -length, dist,   0 );
		NavDrawLine( start, end, color );

		start =	m_center + Vector( -length,  dist, 0 );
		end =	m_center + Vector(  length,  dist, 0 );
		NavDrawLine( start, end, color );

		start =	m_center + Vector( -dist,   -length, 0 );
		end =	m_center + Vector( -length, -dist,   0 );
		NavDrawLine( start, end, color );

		start =	m_center + Vector( -length, -dist, 0 );
		end =	m_center + Vector(  length, -dist, 0 );
		NavDrawLine( start, end, color );

		start =	m_center + Vector( length, -dist,   0 );
		end =	m_center + Vector( dist,   -length, 0 );
		NavDrawLine( start, end, color );
	}

	// Walk is represented by an arrow
	if (GetAttributes() & NAV_MESH_WALK)
	{
		if ( useAttributeColors )
			color = NavAttributeWalkColor;

		float size = 8.0f;
		NavDrawHorizontalArrow( m_center + Vector( -size, 0, 0 ), m_center + Vector( size, 0, 0 ), 4, color );
	}

	// Walk is represented by a double arrow
	if (GetAttributes() & NAV_MESH_RUN)
	{
		if ( useAttributeColors )
			color = NavAttributeRunColor;

		float size = 8.0f;
		float dist = 4.0f;
		NavDrawHorizontalArrow( m_center + Vector( -size,  dist, 0 ), m_center + Vector( size,  dist, 0 ), 4, color );
		NavDrawHorizontalArrow( m_center + Vector( -size, -dist, 0 ), m_center + Vector( size, -dist, 0 ), 4, color );
	}

	// Avoid is represented by an exclamation point
	if (GetAttributes() & NAV_MESH_AVOID)
	{
		if ( useAttributeColors )
			color = NavAttributeAvoidColor;

		float topHeight = 8.0f;
		float topWidth = 3.0f;
		float bottomHeight = 3.0f;
		float bottomWidth = 2.0f;
		NavDrawTriangle( m_center, m_center + Vector( -topWidth, topHeight, 0 ), m_center + Vector( +topWidth, topHeight, 0 ), color );
		NavDrawTriangle( m_center + Vector( 0, -bottomHeight, 0 ), m_center + Vector( -bottomWidth, -bottomHeight*2, 0 ), m_center + Vector( bottomWidth, -bottomHeight*2, 0 ), color );
	}

	if ( IsBlocked( NAV_TEAM_ANY ) || HasAvoidanceObstacle() || IsDamaging() )
	{
		NavEditColor color = (IsBlocked( NAV_TEAM_ANY ) && ( m_attributeFlags & NAV_MESH_NAV_BLOCKER ) ) ? NavBlockedByFuncNavBlockerColor : NavBlockedByDoorColor;
		const float blockedInset = 4.0f;
		nw.x += blockedInset;
		nw.y += blockedInset;
		ne.x -= blockedInset;
		ne.y += blockedInset;
		sw.x += blockedInset;
		sw.y -= blockedInset;
		se.x -= blockedInset;
		se.y -= blockedInset;
		NavDrawLine( nw, ne, color );
		NavDrawLine( ne, se, color );
		NavDrawLine( se, sw, color );
		NavDrawLine( sw, nw, color );
	}
}


//--------------------------------------------------------------------------------------------------------
/**
 * Draw area as a filled rect of the given color
 */
void CNavArea::DrawFilled( int r, int g, int b, int a, float deltaT, bool noDepthTest, float margin ) const
{
	Vector nw = GetCorner( NORTH_WEST ) + Vector( margin, margin, 0.0f );
	Vector ne = GetCorner( NORTH_EAST ) + Vector( -margin, margin, 0.0f );
	Vector sw = GetCorner( SOUTH_WEST ) + Vector( margin, -margin, 0.0f );
	Vector se = GetCorner( SOUTH_EAST ) + Vector( -margin, -margin, 0.0f );

	if ( a == 0 )
	{
		debugoverlay->AddLineOverlay( nw, ne, r, g, b, true, deltaT );
		debugoverlay->AddLineOverlay( nw, sw, r, g, b, true, deltaT );
		debugoverlay->AddLineOverlay( sw, se, r, g, b, true, deltaT );
		debugoverlay->AddLineOverlay( se, ne, r, g, b, true, deltaT );
	}
	else
	{
		debugoverlay->AddTriangleOverlay( nw, se, ne, r, g, b, a, noDepthTest, deltaT );
		debugoverlay->AddTriangleOverlay( se, nw, sw, r, g, b, a, noDepthTest, deltaT );
	}
}


//--------------------------------------------------------------------------------------------------------
void CNavArea::DrawSelectedSet( const Vector &shift ) const
{
	const float deltaT = NDEBUG_PERSIST_FOR_ONE_TICK;
	int r = s_selectedSetColor.r();
	int g = s_selectedSetColor.g();
	int b = s_selectedSetColor.b();
	int a = s_selectedSetColor.a();

	Vector nw = GetCorner( NORTH_WEST ) + shift;
	Vector ne = GetCorner( NORTH_EAST ) + shift;
	Vector sw = GetCorner( SOUTH_WEST ) + shift;
	Vector se = GetCorner( SOUTH_EAST ) + shift;

	debugoverlay->AddTriangleOverlay( nw, se, ne, r, g, b, a, true, deltaT );
	debugoverlay->AddTriangleOverlay( se, nw, sw, r, g, b, a, true, deltaT );

	r = s_selectedSetBorderColor.r();
	g = s_selectedSetBorderColor.g();
	b = s_selectedSetBorderColor.b();
	debugoverlay->AddLineOverlay( nw, ne, r, g, b, true, deltaT );
	debugoverlay->AddLineOverlay( nw, sw, r, g, b, true, deltaT );
	debugoverlay->AddLineOverlay( sw, se, r, g, b, true, deltaT );
	debugoverlay->AddLineOverlay( se, ne, r, g, b, true, deltaT );
}


//--------------------------------------------------------------------------------------------------------
void CNavArea::DrawDragSelectionSet( Color &dragSelectionSetColor ) const
{
	const float deltaT = NDEBUG_PERSIST_FOR_ONE_TICK;
	int r = dragSelectionSetColor.r();
	int g = dragSelectionSetColor.g();
	int b = dragSelectionSetColor.b();
	int a = dragSelectionSetColor.a();

	Vector nw = GetCorner( NORTH_WEST );
	Vector ne = GetCorner( NORTH_EAST );
	Vector sw = GetCorner( SOUTH_WEST );
	Vector se = GetCorner( SOUTH_EAST );

	debugoverlay->AddTriangleOverlay( nw, se, ne, r, g, b, a, true, deltaT );
	debugoverlay->AddTriangleOverlay( se, nw, sw, r, g, b, a, true, deltaT );

	r = s_dragSelectionSetBorderColor.r();
	g = s_dragSelectionSetBorderColor.g();
	b = s_dragSelectionSetBorderColor.b();
	debugoverlay->AddLineOverlay( nw, ne, r, g, b, true, deltaT );
	debugoverlay->AddLineOverlay( nw, sw, r, g, b, true, deltaT );
	debugoverlay->AddLineOverlay( sw, se, r, g, b, true, deltaT );
	debugoverlay->AddLineOverlay( se, ne, r, g, b, true, deltaT );
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Draw navigation areas and edit them
 */
void CNavArea::DrawHidingSpots( void ) const
{
	const HidingSpotVector *hidingSpots = GetHidingSpots();

	FOR_EACH_VEC( (*hidingSpots), it )
	{
		const HidingSpot *spot = (*hidingSpots)[ it ];

		NavEditColor color;

		if (spot->IsIdealSniperSpot())
		{
			color = NavIdealSniperColor;
		}
		else if (spot->IsGoodSniperSpot())
		{
			color = NavGoodSniperColor;
		}
		else if (spot->HasGoodCover())
		{
			color = NavGoodCoverColor;
		}
		else
		{
			color = NavExposedColor;
		}

		NavDrawLine( spot->GetPosition(), spot->GetPosition() + Vector( 0, 0, 50 ), color );
	}
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Draw ourselves and adjacent areas
 */
void CNavArea::DrawConnectedAreas( CNavMesh* TheNavMesh ) const
{
	int i;
	if (UTIL_GetListenServerHost() == NULL)
		return;

	// draw self
	if (TheNavMesh->IsEditMode( CNavMesh::PLACE_PAINTING ))
	{
		Draw();
	}
	else
	{
		Draw();
		DrawHidingSpots();
	}

	// draw connected ladders
	{
		FOR_EACH_VEC( m_ladder[ CNavLadder::LADDER_UP ], it )
		{
			CNavLadder *ladder = m_ladder[ CNavLadder::LADDER_UP ][ it ].ladder;

			ladder->DrawLadder(TheNavMesh->GetSelectedLadder() == ladder,
					TheNavMesh->GetMarkedLadder() == ladder,
					TheNavMesh->IsEditMode(CNavMesh::PLACE_PAINTING));
			if ( !ladder->IsConnected( this, CNavLadder::LADDER_DOWN ) )
			{
				NavDrawLine( m_center, ladder->m_bottom + Vector( 0, 0, navgenparams->generation_step_size ), NavConnectedOneWayColor );
			}
		}
	}
	{
		FOR_EACH_VEC( m_ladder[ CNavLadder::LADDER_DOWN ], it )
		{
			CNavLadder *ladder = m_ladder[ CNavLadder::LADDER_DOWN ][ it ].ladder;

			ladder->DrawLadder(TheNavMesh->GetSelectedLadder() == ladder,
					TheNavMesh->GetMarkedLadder() == ladder,
					TheNavMesh->IsEditMode(CNavMesh::PLACE_PAINTING));

			if ( !ladder->IsConnected( this, CNavLadder::LADDER_UP ) )
			{
				NavDrawLine( m_center, ladder->m_top, NavConnectedOneWayColor );
			}
		}
	}

	// draw connected areas
	for( i=0; i<NUM_DIRECTIONS; ++i )
	{
		NavDirType dir = (NavDirType)i;

		int count = GetAdjacentCount( dir );

		for( int a=0; a<count; ++a )
		{
			CNavArea *adj = GetAdjacentArea( dir, a );

			adj->Draw();

			if ( !TheNavMesh->IsEditMode( CNavMesh::PLACE_PAINTING ) )
			{
				adj->DrawHidingSpots();

				Vector from, to;
				Vector hookPos;
				float halfWidth;
				float size = 5.0f;
				ComputePortal( adj, dir, &hookPos, &halfWidth );

				switch( dir )
				{
					case NORTH:
						from = hookPos + Vector( 0.0f, size, 0.0f );
						to = hookPos + Vector( 0.0f, -size, 0.0f );
						break;
					case SOUTH:
						from = hookPos + Vector( 0.0f, -size, 0.0f );
						to = hookPos + Vector( 0.0f, size, 0.0f );
						break;
					case EAST:
						from = hookPos + Vector( -size, 0.0f, 0.0f );
						to = hookPos + Vector( +size, 0.0f, 0.0f );
						break;
					case WEST:
						from = hookPos + Vector( size, 0.0f, 0.0f );
						to = hookPos + Vector( -size, 0.0f, 0.0f );
						break;
				}

				from.z = GetZ( from );
				to.z = adj->GetZ( to );

				Vector drawTo;
				adj->GetClosestPointOnArea( to, &drawTo );

				if (sm_nav_show_contiguous.GetBool())
				{
					if ( IsContiguous( adj ) )
						NavDrawLine( from, drawTo, NavConnectedContiguous );
					else
						NavDrawLine( from, drawTo, NavConnectedNonContiguous );
				}
				else
				{
					if ( adj->IsConnected( this, OppositeDirection( dir ) ) )
						NavDrawLine( from, drawTo, NavConnectedTwoWaysColor );
					else
						NavDrawLine( from, drawTo, NavConnectedOneWayColor );
				}
			}
		}
	}

	for (auto& link : m_offmeshconnections)
	{
		const Vector& start = link.m_start;
		const Vector& end = link.m_end;
		link.m_link.area->Draw();
		NDebugOverlay::Line(start, end, 0, 255, 0, true, NDEBUG_PERSIST_FOR_ONE_TICK);
		char message[64];
		ke::SafeSprintf(message, sizeof(message), "Nav Link <%s>", NavOffMeshConnection::OffMeshConnectionTypeToString(link.m_type));
		NDebugOverlay::Text(start, message, false, NDEBUG_PERSIST_FOR_ONE_TICK);
	}
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Add to open list in decreasing value order
 */
void CNavArea::AddToOpenList( void )
{
	Assert( (m_openList && m_openList->m_prevOpen == NULL) || m_openList == NULL );

	if ( IsOpen() )
	{
		// already on list
		return;
	}

	// mark as being on open list for quick check
	m_openMarker = m_masterMarker;

	// if list is empty, add and return
	if ( m_openList == NULL )
	{
		m_openList = this;
		m_openListTail = this;
		this->m_prevOpen = NULL;
		this->m_nextOpen = NULL;
		return;
	}

	// insert self in ascending cost order
	CNavArea *area, *last = NULL;
	for( area = m_openList; area; area = area->m_nextOpen )
	{
		if ( GetTotalCost() < area->GetTotalCost() )
		{
			break;
		}
		last = area;
	}

	if ( area )
	{
		// insert before this area
		this->m_prevOpen = area->m_prevOpen;

		if ( this->m_prevOpen )
		{
			this->m_prevOpen->m_nextOpen = this;
		}
		else
		{
			m_openList = this;
		}

		this->m_nextOpen = area;
		area->m_prevOpen = this;
	}
	else
	{
		// append to end of list
		last->m_nextOpen = this;
		this->m_prevOpen = last;
	
		this->m_nextOpen = NULL;

		m_openListTail = this;
	}

	Assert( (m_openList && m_openList->m_prevOpen == NULL) || m_openList == NULL );
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Add to tail of the open list
 */
void CNavArea::AddToOpenListTail( void )
{
	Assert( (m_openList && m_openList->m_prevOpen == NULL) || m_openList == NULL );

	if ( IsOpen() )
	{
		// already on list
		return;
	}

	// mark as being on open list for quick check
	m_openMarker = m_masterMarker;

	// if list is empty, add and return
	if ( m_openList == NULL )
	{
		m_openList = this;
		m_openListTail = this;
		this->m_prevOpen = NULL;
		this->m_nextOpen = NULL;

		Assert( (m_openList && m_openList->m_prevOpen == NULL) || m_openList == NULL );
		return;
	}

	// append to end of list
	m_openListTail->m_nextOpen = this;

	this->m_prevOpen = m_openListTail;
	this->m_nextOpen = NULL;

	m_openListTail = this;

	Assert( (m_openList && m_openList->m_prevOpen == NULL) || m_openList == NULL );
}

//--------------------------------------------------------------------------------------------------------------
/**
 * A smaller value has been found, update this area on the open list
 * @todo "bubbling" does unnecessary work, since the order of all other nodes will be unchanged - only this node is altered
 */
void CNavArea::UpdateOnOpenList( void )
{
	// since value can only decrease, bubble this area up from current spot
	while( m_prevOpen && this->GetTotalCost() < m_prevOpen->GetTotalCost() )
	{
		// swap position with predecessor
		CNavArea *other = m_prevOpen;
		CNavArea *before = other->m_prevOpen;
		CNavArea *after  = this->m_nextOpen;

		this->m_nextOpen = other;
		this->m_prevOpen = before;

		other->m_prevOpen = this;
		other->m_nextOpen = after;

		if ( before )
		{
			before->m_nextOpen = this;
		}
		else
		{
			m_openList = this;
		}

		if ( after )
		{
			after->m_prevOpen = other;
		}
		else
		{
			m_openListTail = this;
		}
	}
}

//--------------------------------------------------------------------------------------------------------------
void CNavArea::RemoveFromOpenList( void )
{
	if ( m_openMarker == 0 )
	{
		// not on the list
		return;
	}

	if ( m_prevOpen )
	{
		m_prevOpen->m_nextOpen = m_nextOpen;
	}
	else
	{
		m_openList = m_nextOpen;
	}
	
	if ( m_nextOpen )
	{
		m_nextOpen->m_prevOpen = m_prevOpen;
	}
	else
	{
		m_openListTail = m_prevOpen;
	}
	
	// zero is an invalid marker
	m_openMarker = 0;
}

//--------------------------------------------------------------------------------------------------------------
bool CNavArea::IsDamaging( void ) const
{
	return (gpGlobals->tickcount <= m_damagingTickCount);	
}


//--------------------------------------------------------------------------------------------------------------
void CNavArea::MarkAsDamaging( float duration )
{
	m_damagingTickCount = gpGlobals->tickcount + TIME_TO_TICKS(duration);
}

//--------------------------------------------------------------------------------------------------------------
bool CNavArea::IsVisible( const Vector &eye, Vector *visSpot ) const
{
	Vector corner;
	trace_t result;
	trace::CTraceFilterNoNPCsOrPlayers tracefilter(nullptr, COLLISION_GROUP_NONE);
	const float offset = 0.75f * navgenparams->human_height; 

	// check center first
	trace::line(eye, GetCenter() + Vector(0.0f, 0.0f, offset), MASK_BLOCKLOS_AND_NPCS | CONTENTS_IGNORE_NODRAW_OPAQUE, &tracefilter, result);

	if (result.fraction == 1.0f)
	{
		// we can see this area
		if (visSpot)
		{
			*visSpot = GetCenter();
		}
		return true;
	}

	for( int c=0; c<NUM_CORNERS; ++c )
	{
		corner = GetCorner( (NavCornerType)c );

		trace::line(eye, GetCenter() + Vector(0.0f, 0.0f, offset), MASK_BLOCKLOS_AND_NPCS | CONTENTS_IGNORE_NODRAW_OPAQUE, &tracefilter, result);

		if (result.fraction == 1.0f)
		{
			// we can see this area
			if (visSpot)
			{
				*visSpot = corner;
			}
			return true;
		}
	}

	return false;
}

bool CNavArea::IsPartiallyVisible(const CNavArea* other, const bool checkPVS) const
{
	if (checkPVS)
	{
		int cluster = engine->GetClusterForOrigin(GetCenter());
		engine->GetPVSForCluster(cluster, static_cast<int>(CNavArea::s_pvs.size()), CNavArea::s_pvs.data());

		if (!engine->CheckOriginInPVS(other->GetCenter(), CNavArea::s_pvs.data(), static_cast<int>(CNavArea::s_pvs.size())))
		{
			return false; // outside PVS
		}
	}

	const Vector offset(0.0f, 0.0f, navgenparams->human_eye_height);

	Vector start = GetCenter() + offset;
	Vector end = other->GetCenter() + offset;

	CTraceFilterWorldAndPropsOnly filter;
	trace_t tr;
	trace::line(start, end, MASK_VISIBLE, &filter, tr);

	if (tr.fraction == 1.0f)
	{
		return true;
	}

	for (int c1 = 0; c1 < static_cast<int>(NavCornerType::NUM_CORNERS); c1++)
	{
		start = GetCorner(static_cast<NavCornerType>(c1)) + offset;

		for (int c2 = 0; c2 < static_cast<int>(NavCornerType::NUM_CORNERS); c2++)
		{
			end = other->GetCorner(static_cast<NavCornerType>(c2)) + offset;

			trace::line(start, end, MASK_VISIBLE, &filter, tr);

			if (tr.fraction == 1.0f)
			{
				return true;
			}
		}
	}

	return false;
}

bool CNavArea::IsCompletelyVisible(const CNavArea* other, const bool checkPVS) const
{
	if (checkPVS)
	{
		int cluster = engine->GetClusterForOrigin(GetCenter());
		engine->GetPVSForCluster(cluster, static_cast<int>(CNavArea::s_pvs.size()), CNavArea::s_pvs.data());

		if (!engine->CheckOriginInPVS(other->GetCenter(), CNavArea::s_pvs.data(), static_cast<int>(CNavArea::s_pvs.size())))
		{
			return false; // outside PVS
		}
	}

	const Vector offset(0.0f, 0.0f, navgenparams->human_eye_height);

	Vector start = GetCenter() + offset;
	Vector end = other->GetCenter() + offset;

	CTraceFilterWorldAndPropsOnly filter;
	trace_t tr;
	trace::line(start, end, MASK_VISIBLE, &filter, tr);

	if (tr.fraction < 1.0f)
	{
		return false;
	}

	for (int c1 = 0; c1 < static_cast<int>(NavCornerType::NUM_CORNERS); c1++)
	{
		start = GetCorner(static_cast<NavCornerType>(c1)) + offset;

		for (int c2 = 0; c2 < static_cast<int>(NavCornerType::NUM_CORNERS); c2++)
		{
			end = other->GetCorner(static_cast<NavCornerType>(c2)) + offset;

			trace::line(start, end, MASK_VISIBLE, &filter, tr);

			if (tr.fraction < 1.0f)
			{
				return false;
			}
		}
	}

	return true;
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Clears the open and closed lists for a new search
 */
void CNavArea::ClearSearchLists( void )
{
	// effectively clears all open list pointers and closed flags
	CNavArea::MakeNewMarker();

	m_openList = NULL;
	m_openListTail = NULL;
}

//--------------------------------------------------------------------------------------------------------------
void CNavArea::SetCorner( NavCornerType corner, const Vector& newPosition )
{
	switch( corner )
	{
		case NORTH_WEST:
			m_nwCorner = newPosition;
			break;

		case NORTH_EAST:
			m_seCorner.x = newPosition.x;
			m_nwCorner.y = newPosition.y;
			m_neZ = newPosition.z;
			break;

		case SOUTH_WEST:
			m_nwCorner.x = newPosition.x;
			m_seCorner.y = newPosition.y;
			m_swZ = newPosition.z;
			break;

		case SOUTH_EAST:
			m_seCorner = newPosition;
			break;

		default:
		{
			Vector oldPosition = GetCenter();
			Vector delta = newPosition - oldPosition;
			m_nwCorner += delta;
			m_seCorner += delta;
			m_neZ += delta.z;
			m_swZ += delta.z;
		}
	}

	m_center.x = (m_nwCorner.x + m_seCorner.x)/2.0f;
	m_center.y = (m_nwCorner.y + m_seCorner.y)/2.0f;
	m_center.z = (m_nwCorner.z + m_seCorner.z)/2.0f;

	if ( ( m_seCorner.x - m_nwCorner.x ) > 0.0f && ( m_seCorner.y - m_nwCorner.y ) > 0.0f )
	{
		m_invDxCorners = 1.0f / ( m_seCorner.x - m_nwCorner.x );
		m_invDyCorners = 1.0f / ( m_seCorner.y - m_nwCorner.y );
	}
	else
	{
		m_invDxCorners = m_invDyCorners = 0;
	}

	CalcDebugID();
}


void CNavArea::DecayDanger()
{
	const float decay = sm_nav_danger_decay_rate.GetFloat();

	for (size_t i = 0; i < NAV_TEAMS_ARRAY_SIZE; i++)
	{
		m_danger[i] = m_danger[i] - decay;

		if (m_danger[i] < 0.0f)
		{
			m_danger[i] = 0.0f;
		}
	}
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Returns true if an existing hiding spot is too close to given position
 */
bool CNavArea::IsHidingSpotCollision( const Vector &pos ) const
{
	const float collisionRange = 30.0f;

	FOR_EACH_VEC( m_hidingSpots, it )
	{
		const HidingSpot *spot = m_hidingSpots[ it ];

		if ((spot->GetPosition() - pos).IsLengthLessThan( collisionRange ))
			return true;
	}

	return false;
}

//--------------------------------------------------------------------------------------------------------------
bool IsHidingSpotInCover( const Vector &spot )
{
	int coverCount = 0;
	trace_t result;

	Vector from = spot;
	from.z += navgenparams->human_height;

	Vector to;

	// if we are crouched underneath something, that counts as good cover
	to = from + Vector( 0, 0, 20.0f );

	trace::line(from, to, MASK_PLAYERSOLID_BRUSHONLY, nullptr, COLLISION_GROUP_NONE, result);

	if (result.fraction != 1.0f)
		return true;

	const float coverRange = 100.0f;
	const float inc = M_PI / 8.0f;

	for( float angle = 0.0f; angle < 2.0f * M_PI; angle += inc )
	{
		to = from + Vector( coverRange * (float)cos(angle), coverRange * (float)sin(angle), navgenparams->human_height );

		trace::line(from, to, MASK_PLAYERSOLID_BRUSHONLY, nullptr, COLLISION_GROUP_NONE, result);

		// if traceline hit something, it hit "cover"
		if (result.fraction != 1.0f)
			++coverCount;
	}

	// if more than half of the circle has no cover, the spot is not "in cover"
	const int halfCover = 8;
	if (coverCount < halfCover)
		return false;

	return true;
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Finds the hiding spot position in a corner's area.  If the typical inset is off the nav area (small
 * hand-constructed areas), it tries to fit the position inside the area.
 */
static Vector FindPositionInArea( CNavArea *area, NavCornerType corner )
{
	int multX = 1, multY = 1;
	switch ( corner )
	{
	case NORTH_WEST:
		break;
	case NORTH_EAST:
		multX = -1;
		break;
	case SOUTH_WEST:
		multY = -1;
		break;
	case SOUTH_EAST:
		multX = -1;
		multY = -1;
		break;
	}

	const float offset = 12.5f;
	Vector cornerPos = area->GetCorner( corner );

	// Try the basic inset
	Vector pos = cornerPos + Vector(  offset*multX,  offset*multY, 0.0f );
	if ( !area->IsOverlapping( pos ) )
	{
		// Try pulling the Y offset to the area's center
		pos = cornerPos + Vector(  offset*multX,  area->GetSizeY()*0.5f*multY, 0.0f );
		if ( !area->IsOverlapping( pos ) )
		{
			// Try pulling the X offset to the area's center
			pos = cornerPos + Vector(  area->GetSizeX()*0.5f*multX,  offset*multY, 0.0f );
			if ( !area->IsOverlapping( pos ) )
			{
				// Try pulling the X and Y offsets to the area's center
				pos = cornerPos + Vector(  area->GetSizeX()*0.5f*multX,  area->GetSizeY()*0.5f*multY, 0.0f );
				if ( !area->IsOverlapping( pos ) )
				{
					// Just pull the position to a small offset
					pos = cornerPos + Vector(  1.0f*multX,  1.0f*multY, 0.0f );
					if ( !area->IsOverlapping( pos ) )
					{
						// Nothing is working (degenerate area?), so just put it directly on the corner
						pos = cornerPos;
					}
				}
			}
		}
	}

	return pos;
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Analyze local area neighborhood to find "hiding spots" for this area
 */
void CNavArea::ComputeHidingSpots( void )
{
	struct
	{
		float lo, hi;
	}
	extent;

	m_hidingSpots.PurgeAndDeleteElements();


	// "jump areas" cannot have hiding spots
	if ( GetAttributes() & NAV_MESH_JUMP )
		return;

	// "don't hide areas" cannot have hiding spots
	if ( GetAttributes() & NAV_MESH_DONT_HIDE )
		return;

	int cornerCount[NUM_CORNERS];
	for( int i=0; i<NUM_CORNERS; ++i )
		cornerCount[i] = 0;

	const float cornerSize = 20.0f;

	// for each direction, find extents of adjacent areas along the wall
	for( int d=0; d<NUM_DIRECTIONS; ++d )
	{
		extent.lo = 999999.9f;
		extent.hi = -999999.9f;

		bool isHoriz = (d == NORTH || d == SOUTH) ? true : false;

		FOR_EACH_VEC( m_connect[d], it )
		{
			NavConnect connect = m_connect[ d ][ it ];

			// if connection is only one-way, it's a "jump down" connection (ie: a discontinuity that may mean cover) 
			// ignore it
			if (connect.area->IsConnected( this, OppositeDirection( static_cast<NavDirType>( d ) ) ) == false)
				continue;

			// ignore jump areas
			if (connect.area->GetAttributes() & NAV_MESH_JUMP)
				continue;

			if (isHoriz)
			{
				if (connect.area->m_nwCorner.x < extent.lo)
					extent.lo = connect.area->m_nwCorner.x;

				if (connect.area->m_seCorner.x > extent.hi)
					extent.hi = connect.area->m_seCorner.x;			
			}
			else
			{
				if (connect.area->m_nwCorner.y < extent.lo)
					extent.lo = connect.area->m_nwCorner.y;

				if (connect.area->m_seCorner.y > extent.hi)
					extent.hi = connect.area->m_seCorner.y;
			}
		}

		switch( d )
		{
			case NORTH:
				if (extent.lo - m_nwCorner.x >= cornerSize)
					++cornerCount[ NORTH_WEST ];

				if (m_seCorner.x - extent.hi >= cornerSize)
					++cornerCount[ NORTH_EAST ];
				break;

			case SOUTH:
				if (extent.lo - m_nwCorner.x >= cornerSize)
					++cornerCount[ SOUTH_WEST ];

				if (m_seCorner.x - extent.hi >= cornerSize)
					++cornerCount[ SOUTH_EAST ];
				break;

			case EAST:
				if (extent.lo - m_nwCorner.y >= cornerSize)
					++cornerCount[ NORTH_EAST ];

				if (m_seCorner.y - extent.hi >= cornerSize)
					++cornerCount[ SOUTH_EAST ];
				break;

			case WEST:
				if (extent.lo - m_nwCorner.y >= cornerSize)
					++cornerCount[ NORTH_WEST ];

				if (m_seCorner.y - extent.hi >= cornerSize)
					++cornerCount[ SOUTH_WEST ];
				break;
		}
	}

	for ( int c=0; c<NUM_CORNERS; ++c )
	{
		// if a corner count is 2, then it really is a corner (walls on both sides)
		if (cornerCount[c] == 2)
		{
			Vector pos = FindPositionInArea( this, (NavCornerType)c );
			if ( !c || !IsHidingSpotCollision( pos ) )
			{
				HidingSpot *spot = TheNavMesh->CreateHidingSpot();
				spot->SetPosition( pos );
				spot->SetFlags( IsHidingSpotInCover( pos ) ? HidingSpot::IN_COVER : HidingSpot::EXPOSED );
				m_hidingSpots.AddToTail( spot );
			}
		}
	}
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Determine how much walkable area we can see from the spot, and how far away we can see.
 */
void ClassifySniperSpot( HidingSpot *spot )
{
	Vector eye = spot->GetPosition();

	CNavArea *hidingArea = TheNavMesh->GetNavArea( spot->GetPosition() );
	if (hidingArea && (hidingArea->GetAttributes() & NAV_MESH_STAND))
	{
		// we will be standing at this hiding spot
		eye.z += navgenparams->human_eye_height;
	}
	else
	{
		// we are crouching when at this hiding spot
		eye.z += navgenparams->human_crouch_eye_height;
	}

	Vector walkable;
	trace_t result;

	Extent sniperExtent;
	float farthestRangeSq = 0.0f;
	const float minSniperRangeSq = 1000.0f * 1000.0f;
	bool found = false;

	// to make compiler stop warning me
	sniperExtent.lo = Vector( 0.0f, 0.0f, 0.0f );
	sniperExtent.hi = Vector( 0.0f, 0.0f, 0.0f );

	Extent areaExtent;
	FOR_EACH_VEC( TheNavAreas, it )
	{
		CNavArea *area = TheNavAreas[ it ];

		area->GetExtent( &areaExtent );

		// scan this area
		for( walkable.y = areaExtent.lo.y + navgenparams->generation_step_size/2.0f; walkable.y < areaExtent.hi.y; walkable.y += navgenparams->generation_step_size )
		{
			for( walkable.x = areaExtent.lo.x + navgenparams->generation_step_size/2.0f; walkable.x < areaExtent.hi.x; walkable.x += navgenparams->generation_step_size )
			{
				walkable.z = area->GetZ(walkable) + navgenparams->half_human_height;;
				
				// check line of sight
				trace::line(eye, walkable, CONTENTS_SOLID | CONTENTS_MOVEABLE | CONTENTS_PLAYERCLIP, nullptr, COLLISION_GROUP_NONE, result);

				if (result.fraction == 1.0f && !result.startsolid)
				{
					// can see this spot

					// keep track of how far we can see
					float rangeSq = (eye - walkable).LengthSqr();
					if (rangeSq > farthestRangeSq)
					{
						farthestRangeSq = rangeSq;

						if (rangeSq >= minSniperRangeSq)
						{
							// this is a sniper spot
							// determine how good of a sniper spot it is by keeping track of the snipable area
							if (found)
							{
								if (walkable.x < sniperExtent.lo.x)
									sniperExtent.lo.x = walkable.x;
								if (walkable.x > sniperExtent.hi.x)
									sniperExtent.hi.x = walkable.x;

								if (walkable.y < sniperExtent.lo.y)
									sniperExtent.lo.y = walkable.y;
								if (walkable.y > sniperExtent.hi.y)
									sniperExtent.hi.y = walkable.y;
							}
							else
							{
								sniperExtent.lo = walkable;
								sniperExtent.hi = walkable;
								found = true;
							}
						}
					}
				}	
			}
		}
	}

	if (found)
	{
		// if we can see a large snipable area, it is an "ideal" spot
		float snipableArea = sniperExtent.Area();

		const float minIdealSniperArea = 200.0f * 200.0f;
		const float longSniperRangeSq = 1500.0f * 1500.0f;

		if (snipableArea >= minIdealSniperArea || farthestRangeSq >= longSniperRangeSq)
			spot->m_flags |= HidingSpot::IDEAL_SNIPER_SPOT;
		else
			spot->m_flags |= HidingSpot::GOOD_SNIPER_SPOT;
	}
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Analyze local area neighborhood to find "sniper spots" for this area
 */
void CNavArea::ComputeSniperSpots( void )
{
	if (sm_nav_quicksave.GetBool())
		return;

	FOR_EACH_VEC( m_hidingSpots, it )
	{
		HidingSpot *spot = m_hidingSpots[ it ];

		ClassifySniperSpot( spot );
	}
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Returns a 0..1 light intensity for the given point
 */
float CNavArea::GetLightIntensity( const Vector &pos ) const
{
	Vector testPos;
	testPos.x = clamp( pos.x, m_nwCorner.x, m_seCorner.x );
	testPos.y = clamp( pos.y, m_nwCorner.y, m_seCorner.y );
	testPos.z = pos.z;

	float dX = (testPos.x - m_nwCorner.x) / (m_seCorner.x - m_nwCorner.x);
	float dY = (testPos.y - m_nwCorner.y) / (m_seCorner.y - m_nwCorner.y);

	float northLight = m_lightIntensity[ NORTH_WEST ] * ( 1 - dX ) + m_lightIntensity[ NORTH_EAST ] * dX;
	float southLight = m_lightIntensity[ SOUTH_WEST ] * ( 1 - dX ) + m_lightIntensity[ SOUTH_EAST ] * dX;
	float light = northLight * ( 1 - dY ) + southLight * dY;

	return light;
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Returns a 0..1 light intensity for the given point
 */
float CNavArea::GetLightIntensity( float x, float y ) const
{
	return GetLightIntensity( Vector( x, y, 0 ) );
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Returns a 0..1 light intensity averaged over the whole area
 */
float CNavArea::GetLightIntensity( void ) const
{
	float light = m_lightIntensity[ NORTH_WEST ];
	light += m_lightIntensity[ NORTH_EAST ];
	light += m_lightIntensity[ SOUTH_WEST];
	light += m_lightIntensity[ SOUTH_EAST ];
	return light / 4.0f;
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Compute light intensity at corners and center (requires client via listenserver)
 */
bool CNavArea::ComputeLighting( void )
{
	return true;
/*
	if ( engine->IsDedicatedServer() )
	{
		for ( int i=0; i<NUM_CORNERS; ++i )
		{
			m_lightIntensity[i] = 1.0f;
		}

		return true;
	}

	// Calculate light at the corners
	for ( int i=0; i<NUM_CORNERS; ++i )
	{
		Vector pos = FindPositionInArea( this, (NavCornerType)i );
		pos.z = GetZ( pos ) + navgenparams->human_height - navgenparams->step_height;	// players light from their centers, and we light from slightly below that, to allow for low ceilings
		float height;
		if ( TheNavMesh->GetGroundHeight( pos, &height ) )
		{
			pos.z = height + navgenparams->human_height - navgenparams->step_height;	// players light from their centers, and we light from slightly below that, to allow for low ceilings
		}

		Vector light( 0, 0, 0 );
		Vector ambientColor;
		float ambientIntensity = ambientColor.x + ambientColor.y + ambientColor.z;
		float lightIntensity = light.x + light.y + light.z;
		lightIntensity = clamp( lightIntensity, 0.f, 1.f );	// sum can go well over 1.0, but it's the lower region we care about.  if it's bright, we don't need to know *how* bright.

		lightIntensity = MAX( lightIntensity, ambientIntensity );

		m_lightIntensity[i] = lightIntensity;
	}

	return true;
*/
}


//--------------------------------------------------------------------------------------------------------------

#if 0
CON_COMMAND_F( sm_nav_update_lighting, "Recomputes lighting values", FCVAR_CHEAT )
{

	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;
	int numComputed = 0;
	if ( args.ArgC() == 2 )
	{
		CNavArea *area = TheNavMesh->GetNavAreaByID( atoi( args[1] ) );
		if ( area && area->ComputeLighting() ) {
			++numComputed;
		}
	}
	else
	{
		FOR_EACH_VEC( TheNavAreas, index )
		{
			if ( TheNavAreas[ index ]->ComputeLighting() )
			{
				++numComputed;
			}
		}
	}
	DevMsg( "Computed lighting for %d/%d areas\n", numComputed, TheNavAreas.Count() );
}
#endif // 0

//--------------------------------------------------------------------------------------------------------------
/**
 * Raise/lower a corner
 */
void CNavArea::RaiseCorner( NavCornerType corner, int amount, bool raiseAdjacentCorners )
{
	if ( corner == NUM_CORNERS )
	{
		RaiseCorner( NORTH_WEST, amount, raiseAdjacentCorners );
		RaiseCorner( NORTH_EAST, amount, raiseAdjacentCorners );
		RaiseCorner( SOUTH_WEST, amount, raiseAdjacentCorners );
		RaiseCorner( SOUTH_EAST, amount, raiseAdjacentCorners );
		return;
	}

	// Move the corner
	switch (corner)
	{
	case NORTH_WEST:
		m_nwCorner.z += amount;
		break;
	case NORTH_EAST:
		m_neZ += amount;
		break;
	case SOUTH_WEST:
		m_swZ += amount;
		break;
	case SOUTH_EAST:
		m_seCorner.z += amount;
		break;
	}

	// Recompute the center
	m_center.x = (m_nwCorner.x + m_seCorner.x)/2.0f;
	m_center.y = (m_nwCorner.y + m_seCorner.y)/2.0f;
	m_center.z = (m_nwCorner.z + m_seCorner.z)/2.0f;

	if ( ( m_seCorner.x - m_nwCorner.x ) > 0.0f && ( m_seCorner.y - m_nwCorner.y ) > 0.0f )
	{
		m_invDxCorners = 1.0f / ( m_seCorner.x - m_nwCorner.x );
		m_invDyCorners = 1.0f / ( m_seCorner.y - m_nwCorner.y );
	}
	else
	{
		m_invDxCorners = m_invDyCorners = 0;
	}

	if ( !raiseAdjacentCorners || sm_nav_corner_adjust_adjacent.GetFloat() <= 0.0f )
	{
		return;
	}

	// Find nearby areas that share the corner
	CNavArea::MakeNewMarker();
	Mark();

	const float tolerance = sm_nav_corner_adjust_adjacent.GetFloat();

	Vector cornerPos = GetCorner( corner );
	cornerPos.z -= amount; // use the pre-adjustment corner for adjacency checks

	int gridX = TheNavMesh->WorldToGridX( cornerPos.x );
	int gridY = TheNavMesh->WorldToGridY( cornerPos.y );

	const int shift = 1; // try a 3x3 set of grids in case we're on the edge

	for( int x = gridX - shift; x <= gridX + shift; ++x )
	{
		if (x < 0 || x >= TheNavMesh->m_gridSizeX)
			continue;

		for( int y = gridY - shift; y <= gridY + shift; ++y )
		{
			if (y < 0 || y >= TheNavMesh->m_gridSizeY)
				continue;

			NavAreaVector *areas = &TheNavMesh->m_grid[ x + y*TheNavMesh->m_gridSizeX ];

			// find closest area in this cell
			FOR_EACH_VEC( (*areas), it )
			{
				CNavArea *area = (*areas)[ it ];

				// skip if we've already visited this area
				if (area->IsMarked())
					continue;

				area->Mark();

				Vector areaPos;
				for ( int i=0; i<NUM_CORNERS; ++i )
				{
					areaPos = area->GetCorner( NavCornerType(i) );
					if ( areaPos.DistTo( cornerPos ) < tolerance )
					{
						area->RaiseCorner( NavCornerType(i), (cornerPos.z + amount ) - areaPos.z, false );
					}
				}
			}
		}
	}
}


//--------------------------------------------------------------------------------------------------------------
/**
 * FindGroundZFromPoint walks from the start position to the end position in navgenparams->generation_step_size increments,
 * checking the ground height along the way.
 */
static float FindGroundZFromPoint( const Vector& end, const Vector& start )
{
	Vector step( 0, 0, navgenparams->step_height );
	if ( fabs( end.x - start.x ) > fabs( end.y - start.y ) )
	{
		step.x = navgenparams->generation_step_size;
		if ( end.x < start.x )
		{
			step.x = -step.x;
		}
	}
	else
	{
		step.y = navgenparams->generation_step_size;
		if ( end.y < start.y )
		{
			step.y = -step.y;
		}
	}

	// step towards our end point
	Vector point = start;
	float z;
	while ( point.AsVector2D().DistTo( end.AsVector2D() ) > navgenparams->generation_step_size )
	{
		point = point + step;
		z = point.z;
		if ( TheNavMesh->GetGroundHeight( point, &z ) )
		{
			point.z = z;
		}
		else
		{
			point.z -= step.z;
		}
	}

	// now do the exact one once we're within navgenparams->generation_step_size of it
	z = point.z + step.z;
	point = end;
	point.z = z;
	return TheNavMesh->GetGroundHeight( point, &z ) ? z : (point.z - step.z);
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Finds the Z value for a corner given two other corner points.  This walks along the edges of the nav area
 * in navgenparams->generation_step_size increments, to increase accuracy.
 */
static float FindGroundZ( const Vector& original, const Vector& corner1, const Vector& corner2 )
{
	float first = FindGroundZFromPoint( original, corner1 );
	float second = FindGroundZFromPoint( original, corner2 );

	if ( std::abs( first - second ) > navgenparams->step_height )
	{
		// approaching the point from the two directions didn't agree.  Take the one closest to the original z.
		if ( std::abs( original.z - first ) > std::abs( original.z - second ) )
		{
			return second;
		}
		else
		{
			return first;
		}
	}

	return first;
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Places a corner (or all corners if corner == NUM_CORNERS) on the ground
 */
void CNavArea::PlaceOnGround( NavCornerType corner, float inset )
{
	// trace_t result;
	// Vector from, to;

	Vector nw = m_nwCorner + Vector ( inset, inset, 0 );
	Vector se = m_seCorner + Vector ( -inset, -inset, 0 );
	Vector ne, sw;
	ne.x = se.x;
	ne.y = nw.y;
	ne.z = m_neZ;
	sw.x = nw.x;
	sw.y = se.y;
	sw.z = m_swZ;

	if ( corner == NORTH_WEST || corner == NUM_CORNERS )
	{
		float newZ = FindGroundZ( nw, ne, sw );
		RaiseCorner( NORTH_WEST, newZ - nw.z );
	}

	if ( corner == NORTH_EAST || corner == NUM_CORNERS )
	{
		float newZ = FindGroundZ( ne, nw, se );
		RaiseCorner( NORTH_EAST, newZ - ne.z );
	}

	if ( corner == SOUTH_WEST || corner == NUM_CORNERS )
	{
		float newZ = FindGroundZ( sw, nw, se );
		RaiseCorner( SOUTH_WEST, newZ - sw.z );
	}

	if ( corner == SOUTH_EAST || corner == NUM_CORNERS )
	{
		float newZ = FindGroundZ( se, ne, sw );
		RaiseCorner( SOUTH_EAST, newZ - se.z );
	}
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Shift the nav area
 */
void CNavArea::Shift( const Vector &shift )
{
	m_nwCorner += shift;
	m_seCorner += shift;
	
	m_center += shift;
}


//--------------------------------------------------------------------------------------------------------------
static void CommandNavUpdateBlocked( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;
	if ( TheNavMesh->GetMarkedArea() )
	{
		CNavArea *area = TheNavMesh->GetMarkedArea();
		area->UpdateBlocked( true );

		if (area->IsBlocked(NAV_TEAM_ANY))
		{
			auto& center = area->GetCenter();
			DevMsg("Area #%i <%3.2f, %3.2f %3.2f is blocked\n>", area->GetID(), center.x, center.y, center.z);
		}
	}
	else
	{
		float start = Plat_FloatTime();
		CNavArea *blockedArea = NULL;
		FOR_EACH_VEC( TheNavAreas, nit )
		{
			CNavArea *area = TheNavAreas[ nit ];
			area->UpdateBlocked( true );
			if ( area->IsBlocked( NAV_TEAM_ANY ) )
			{
				auto& center = area->GetCenter();
				DevMsg("Area #%i <%3.2f, %3.2f %3.2f is blocked\n>", area->GetID(), center.x, center.y, center.z);

				if ( !blockedArea )
				{
					blockedArea = area;
				}
			}
		}

		float end = Plat_FloatTime();
		float time = (end - start) * 1000.0f;
		DevMsg( "nav_update_blocked took %2.2f ms\n", time );
/*
 * TODO
		if (blockedArea) {
			edict_t *ent = UTIL_GetListenServerHost();
			IPlayerInfo* player = playerinfomanager->GetPlayerInfo(ent);
			if (player
					|| (player->IsDead() || player->IsObserver())
							&& player->GetObserverMode() == OBS_MODE_ROAMING) {
				Vector origin = blockedArea->GetCenter()
						+ Vector(0, 0, 0.75f * navgenparams->human_height);
				UTIL_SetOrigin(player, origin);
			}

		}

 */
	}

}
static ConCommand sm_nav_update_blocked( "sm_nav_update_blocked", CommandNavUpdateBlocked, "Updates the blocked/unblocked status for every nav area.", FCVAR_GAMEDLL );


//--------------------------------------------------------------------------------------------------------
bool CNavArea::IsBlocked( int teamID, bool ignoreNavBlockers ) const
{
	if ( ignoreNavBlockers && ( m_attributeFlags & NAV_MESH_NAV_BLOCKER ) )
	{
		return false;
	}

	if (!ignoreNavBlockers)
	{
		for (INavBlocker* blocker : m_navblockers)
		{
			if (blocker->IsBlocked(teamID))
			{
				return true;
			}
		}
	}

	if (m_volume != nullptr)
	{
		if (m_volume->IsBlocked(teamID))
		{
			return true; // we are blocked by a nav volume
		}
	}

	if (m_elevator != nullptr)
	{
		if (m_elevator->IsBlocked(teamID))
		{
			return true; // we are blocked because my elevator is blocked
		}
	}

	bool result = false;

	if (teamID < 0 || teamID >= static_cast<int>(m_isBlocked.size()))
	{
		for (auto& blocked : m_isBlocked)
		{
			if (blocked)
			{
				result = true;
				break;
			}
		}
	}
	else
	{
		result = m_isBlocked[teamID];
	}

	return result;
}

//--------------------------------------------------------------------------------------------------------
void CNavArea::MarkAsBlocked( int teamID, edict_t* blocker, bool bGenerateEvent )
{
	if ( blocker && UtilHelpers::FClassnameIs(blocker,  "func_nav_blocker" ) )
	{
		m_attributeFlags |= NAV_MESH_NAV_BLOCKER;
	}

	bool wasBlocked = false;

	if (teamID < 0 || teamID >= static_cast<int>(m_isBlocked.size()))
	{
		for (auto& blocked : m_isBlocked)
		{
			if (blocked)
			{
				wasBlocked = true;
			}

			blocked = true;
		}
	}
	else
	{
		wasBlocked = m_isBlocked[teamID];
		m_isBlocked[teamID] = true;
	}

	if ( !wasBlocked )
	{
		if (sm_nav_debug_blocked.GetBool() )
		{
			if ( blocker )
			{
				META_CONPRINTF("%s %d blocked area %d\n", blocker->GetClassName(), gamehelpers->IndexOfEdict(blocker), GetID());
			}
			else
			{
				META_CONPRINTF("non-entity blocked area %d", GetID());
			}
		}
		TheNavMesh->OnAreaBlocked( this );
	}
	else if (sm_nav_debug_blocked.GetBool())
	{
		if ( blocker )
		{
			META_CONPRINTF("DUPE: %s %d blocked area %d\n", blocker->GetClassName(), gamehelpers->IndexOfEdict(blocker), GetID());
		}
		else
		{
			META_CONPRINTF("DUPE: non-entity blocked area %d\n", GetID());
		}
	}
}


//--------------------------------------------------------------------------------------------------------
// checks if any func_nav_blockers are still blocking the area
void CNavArea::UpdateBlockedFromNavBlockers( void )
{
#if 0 // TO-DO
	Extent bounds;
	GetExtent( &bounds );

	// Save off old values, reset to not blocked state
	m_attributeFlags &= ~NAV_MESH_NAV_BLOCKER;
	bool oldBlocked[MAX_NAV_TEAMS];
	bool wasBlocked = false;
	for ( int i=0; i<MAX_NAV_TEAMS; ++i )
	{
		oldBlocked[i] = m_isBlocked[i];
		wasBlocked = wasBlocked || m_isBlocked[i];
		m_isBlocked[i] = false;
	}

	bool isBlocked = CFuncNavBlocker::CalculateBlocked( m_isBlocked, bounds.lo, bounds.hi );

	if ( isBlocked )
	{
		m_attributeFlags |= NAV_MESH_NAV_BLOCKER;
	}

	// If we're unblocked, fire a nav_blocked event.
	if ( wasBlocked != isBlocked )
	{
		if ( isBlocked )
		{
			if (sm_nav_debug_blocked.GetBool())
			{
				ConColorMsg( Color( 0, 255, 128, 255 ), "area %d is blocked by a nav blocker\n", GetID() );
			}
			TheNavMesh->OnAreaBlocked( this );
		}
		else
		{
			if (sm_nav_debug_blocked.GetBool())
			{
				ConColorMsg( Color( 0, 128, 255, 255 ), "area %d is unblocked by a nav blocker\n", GetID() );
			}
			TheNavMesh->OnAreaUnblocked( this );
		}
	}
#endif
}


//--------------------------------------------------------------------------------------------------------------
void CNavArea::UnblockArea( int teamID )
{
	bool wasBlocked = IsBlocked( teamID );

	if (teamID < 0 || teamID >= static_cast<int>(m_isBlocked.size()))
	{
		for (auto& b : m_isBlocked)
		{
			b = false;
		}
	}
	else
	{
		m_isBlocked[teamID] = false;
	}

	if ( wasBlocked )
	{
		if (sm_nav_debug_blocked.GetBool())
		{
			META_CONPRINTF("area %d is unblocked by UnblockArea\n", GetID());
		}
		TheNavMesh->OnAreaUnblocked( this );
	}
}

// Updates the (un)blocked status of the nav area
void CNavArea::UpdateBlocked( bool force, int teamID )
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("[NavBot] CNavArea::UpdateBlocked", "NavBot");
#endif // EXT_VPROF_ENABLED

	if ( !force && !m_blockedTimer.IsElapsed() )
	{
		return;
	}

	constexpr auto MaxBlockedCheckInterval = 5.0f;
	float interval = m_blockedTimer.GetCountdownDuration() + 1;
	if ( interval > MaxBlockedCheckInterval )
	{
		interval = MaxBlockedCheckInterval;
	}
	m_blockedTimer.Start( interval );

	if ( ( m_attributeFlags & NAV_MESH_NAV_BLOCKER ) )
	{
		if ( force )
		{
			UpdateBlockedFromNavBlockers();
		}
		return;
	}

	bool wasBlocked = IsBlocked( NAV_TEAM_ANY );

	// run floor and obstruction checks on transient areas
	if (HasAttributes(static_cast<int>(NavAttributeType::NAV_MESH_TRANSIENT)))
	{
		// A nav area is blocked if there isn't a solid floor underneath it or if something solid is on top of it
		if (!HasSolidFloor() || HasSolidObstruction())
		{
			SetBlocked(true, teamID);
		}
		else
		{
			SetBlocked(false, teamID);
		}
	}

	bool isBlocked = IsBlocked( NAV_TEAM_ANY );

	if ( wasBlocked != isBlocked )
	{
		if ( isBlocked )
		{
			TheNavMesh->OnAreaBlocked( this );
		}
		else
		{
			TheNavMesh->OnAreaUnblocked( this );
		}
	}
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Checks if there is a floor under the nav area, in case a breakable floor is gone
 */
void CNavArea::CheckFloor( edict_t* ignore )
{
	if ( IsBlocked( NAV_TEAM_ANY ) )
		return;

	Vector origin = GetCenter();
	origin.z -= navgenparams->jump_crouch_height;

	const float size = navgenparams->generation_step_size * 0.5f;
	Vector mins = Vector( -size, -size, 0 );
	Vector maxs = Vector( size, size, navgenparams->jump_crouch_height + 10.0f );

	// See if spot is valid
	CBaseEntity* be = reinterpret_cast<CBaseEntity*>(ignore->GetIServerEntity());
	trace_t tr;
	trace::CTraceFilterSimple filter(COLLISION_GROUP_PLAYER_MOVEMENT, be);
	trace::hull(origin, origin, mins, maxs, MASK_PLAYERSOLID_BRUSHONLY, &filter, tr);

	// If the center is open space, we're effectively blocked
	if ( !tr.startsolid )
	{
		MarkAsBlocked( NAV_TEAM_ANY, NULL );
	}
}

bool CNavArea::HasSolidFloor() const
{
	constexpr auto hullsize = 12.0f;
	Vector startPos = GetCenter();
	startPos.z += navgenparams->step_height;
	Vector endPos = GetCenter();
	endPos.z -= navgenparams->jump_height;
	Vector mins(-hullsize, -hullsize, 0.0f);
	Vector maxs(hullsize, hullsize, 1.0f);

	trace::CTraceFilterNoNPCsOrPlayers filter(nullptr, COLLISION_GROUP_NONE);

	trace_t result;

	trace::hull(startPos, endPos, mins, maxs, MASK_PLAYERSOLID, &filter, result);

	if (!result.DidHit())
	{
		if (sm_nav_debug_blocked.GetBool())
		{
			NDebugOverlay::VertArrow(startPos, endPos, 8.0f, 255, 0, 0, 255, true, 20.0f);
		}

		return false;
	}

	return true;
}

bool CNavArea::HasSolidObstruction() const
{
	CTraceFilterTransientAreas filter(nullptr, COLLISION_GROUP_NONE);
	trace_t result;
	Vector origin = GetCenter();
	origin.z += navgenparams->human_height;

	const float sizeX = std::max(1.0f, std::min(GetSizeX() / 2 - 5, navgenparams->half_human_width));
	const float sizeY = std::max(1.0f, std::min(GetSizeY() / 2 - 5, navgenparams->half_human_width));
	Extent bounds;
	bounds.lo.Init(-sizeX, -sizeY, 0);
	// duck height - halfhumanheight
	bounds.hi.Init(sizeX, sizeY, 36.0f - navgenparams->human_height);

	trace::hull(origin, origin, bounds.lo, bounds.hi, MASK_PLAYERSOLID, &filter, result);

	if (result.DidHit())
	{
		if (sm_nav_debug_blocked.GetBool())
		{
			auto edict = gamehelpers->EdictOfIndex(result.GetEntityIndex());

			if (edict != nullptr)
			{
				Msg("CNavArea::HasSolidObstruction() hit entity #%i <%s> \n", result.GetEntityIndex(), gamehelpers->GetEntityClassname(edict));

				if (result.DidHitNonWorldEntity())
				{
					NDebugOverlay::EntityBounds(edict, 100, 0, 255, 150, 2.0f);
				}
			}

			NDebugOverlay::SweptBox(origin, origin, bounds.lo, bounds.hi, vec3_angle, 255, 0, 0, 255, 2.0f);
			NDebugOverlay::Text(GetCenter(), "CNavArea::HasSolidObstruction() == true", false, 2.0f);
		}

		return true; // something is obstructing the area
	}
	else
	{
		Vector v1 = GetCorner(NavCornerType::NORTH_WEST);
		Vector v2 = GetCorner(NavCornerType::SOUTH_EAST);

		v1.z += navgenparams->human_crouch_eye_height;
		v2.z += navgenparams->human_crouch_eye_height;

		trace::line(v1, v2, MASK_PLAYERSOLID, &filter, result);

		if (result.DidHit())
		{
			if (sm_nav_debug_blocked.GetBool())
			{
				debugoverlay->AddLineOverlay(v1, v2, 255, 0, 0, true, 2.0f);
			}

			return true;
		}
	}

	if (sm_nav_debug_blocked.GetBool())
	{
		NDebugOverlay::BoxAngles(origin, bounds.lo, bounds.hi, vec3_angle, 0, 130, 0, 200, 2.0f);
	}

	return false;
}

//--------------------------------------------------------------------------------------------------------
void CNavArea::MarkObstacleToAvoid( float obstructionHeight )
{
	if ( m_avoidanceObstacleHeight < obstructionHeight )
	{
		if ( m_avoidanceObstacleHeight == 0 )
		{
			TheNavMesh->OnAvoidanceObstacleEnteredArea( this );
		}

		m_avoidanceObstacleHeight = obstructionHeight;
	}
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Updates the (un)obstructed status of the nav area
 */
void CNavArea::UpdateAvoidanceObstacles( void )
{
	if ( !m_avoidanceObstacleTimer.IsElapsed() )
	{
		return;
	}

	const float MaxBlockedCheckInterval = 5;
	float interval = m_blockedTimer.GetCountdownDuration() + 1;
	if ( interval > MaxBlockedCheckInterval )
	{
		interval = MaxBlockedCheckInterval;
	}
	m_avoidanceObstacleTimer.Start( interval );

	Vector mins = m_nwCorner;
	Vector maxs = m_seCorner;

	mins.z = MIN( m_nwCorner.z, m_seCorner.z );
	maxs.z = MAX( m_nwCorner.z, m_seCorner.z ) + navgenparams->human_crouch_height;

	float obstructionHeight = 0.0f;
	for ( int i=0; i<TheNavMesh->GetObstructions().Count(); ++i )
	{
		INavAvoidanceObstacle *obstruction = TheNavMesh->GetObstructions()[i];
		edict_t *obstructingEntity = obstruction->GetObstructingEntity();
		if ( !obstructingEntity )
			continue;

		// check if the aabb intersects the search aabb.
		Vector vecSurroundMins, vecSurroundMaxs;
		obstructingEntity->GetCollideable()->WorldSpaceSurroundingBounds(
				&vecSurroundMins, &vecSurroundMaxs);
		if ( !IsBoxIntersectingBox( mins, maxs, vecSurroundMins, vecSurroundMaxs )
				|| !obstruction->CanObstructNavAreas() )
			continue;

		float propHeight = obstruction->GetNavObstructionHeight();

		obstructionHeight = MAX( obstructionHeight, propHeight );
	}

	m_avoidanceObstacleHeight = obstructionHeight;

	if ( m_avoidanceObstacleHeight == 0.0f )
	{
		TheNavMesh->OnAvoidanceObstacleLeftArea( this );
	}
}


//--------------------------------------------------------------------------------------------------------------
// Clear set of func_nav_cost entities that affect this area
void CNavArea::ClearAllNavCostEntities( void )
{
	RemoveAttributes( NAV_MESH_FUNC_COST );
	m_funcNavCostVector.RemoveAll();
}


//--------------------------------------------------------------------------------------------------------------
// Add the given func_nav_cost entity to the cost of this area
void CNavArea::AddFuncNavCostEntity( CFuncNavCost *cost )
{
	SetAttributes( NAV_MESH_FUNC_COST );
	m_funcNavCostVector.AddToTail( cost );
}


//--------------------------------------------------------------------------------------------------------------
// Return the cost multiplier of this area's func_nav_cost entities for the given actor
float CNavArea::ComputeFuncNavCost( edict_t *who ) const
{
	float funcCost = 1.0f;

	for( int i=0; i<m_funcNavCostVector.Count(); ++i )
	{
		if ( m_funcNavCostVector[i] != NULL )
		{
			funcCost *= m_funcNavCostVector[i]->GetCostMultiplier( who );
		}
	}

	return funcCost;
}


//--------------------------------------------------------------------------------------------------------------
bool CNavArea::HasFuncNavAvoid( void ) const
{
	for( int i=0; i<m_funcNavCostVector.Count(); ++i )
	{
		CFuncNavAvoid *avoid = dynamic_cast< CFuncNavAvoid * >( m_funcNavCostVector[i].Get() );
		if ( avoid )
		{
			return true;
		}
	}

	return false;
}


//--------------------------------------------------------------------------------------------------------------
bool CNavArea::HasFuncNavPrefer( void ) const
{
	for( int i=0; i<m_funcNavCostVector.Count(); ++i )
	{
		CFuncNavPrefer *prefer = dynamic_cast< CFuncNavPrefer * >( m_funcNavCostVector[i].Get() );
		if ( prefer )
		{
			return true;
		}
	}

	return false;
}


//--------------------------------------------------------------------------------------------------------------
void CNavArea::CheckWaterLevel( void )
{
	Vector pos( GetCenter() );
	if ( !TheNavMesh->GetGroundHeight( pos, &pos.z ) )
	{
		m_isUnderwater = false;
		return;
	}

	pos.z += 1;
	extern IEngineTrace *enginetrace;
	m_isUnderwater = (enginetrace->GetPointContents( pos ) & MASK_WATER ) != 0;
}


//--------------------------------------------------------------------------------------------------------------
static void CommandNavCheckFloor( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;
	if ( TheNavMesh->GetMarkedArea() )
	{
		CNavArea *area = TheNavMesh->GetMarkedArea();
		area->CheckFloor( NULL );
#if SOURCE_ENGINE == SE_SDK2013
		if ( area->IsBlocked( NAV_TEAM_ANY ) )
		{
			DevMsg( "Area #%d %s is blocked\n", area->GetID(), VecToString( area->GetCenter() + Vector( 0, 0, navgenparams->human_height ) ) );
		}
#endif
	}
	else
	{
		float start = Plat_FloatTime();
		FOR_EACH_VEC( TheNavAreas, nit )
		{
			CNavArea *area = TheNavAreas[ nit ];
			area->CheckFloor( NULL );
#if SOURCE_ENGINE == SE_SDK2013
			if ( area->IsBlocked( NAV_TEAM_ANY ) )
			{
				DevMsg( "Area #%d %s is blocked\n", area->GetID(), VecToString( area->GetCenter() + Vector( 0, 0, navgenparams->human_height ) ) );
			}
#endif
		}

		float end = Plat_FloatTime();
		float time = (end - start) * 1000.0f;
		DevMsg( "nav_check_floor took %2.2f ms\n", time );
	}
}
static ConCommand sm_nav_check_floor( "sm_nav_check_floor", CommandNavCheckFloor, "Updates the blocked/unblocked status for every nav area.", FCVAR_GAMEDLL );


//--------------------------------------------------------------------------------------------------------------
bool SelectOverlappingAreas::operator()( CNavArea *area )
{
	CNavArea *overlappingArea = NULL;
	CNavLadder *overlappingLadder = NULL;

	Vector nw = area->GetCorner( NORTH_WEST );
	Vector se = area->GetCorner( SOUTH_EAST );
	Vector start = nw;
	start.x += navgenparams->generation_step_size/2;
	start.y += navgenparams->generation_step_size/2;

	while ( start.x < se.x )
	{
		start.y = nw.y + navgenparams->generation_step_size/2;
		while ( start.y < se.y )
		{
			start.z = area->GetZ( start.x, start.y );
			Vector end = start;
			start.z -= navgenparams->step_height;
			end.z += navgenparams->human_height;

			if ( TheNavMesh->FindNavAreaOrLadderAlongRay( start, end, &overlappingArea, &overlappingLadder, area ) )
			{
				if ( overlappingArea )
				{
					TheNavMesh->AddToSelectedSet( overlappingArea );
					TheNavMesh->AddToSelectedSet( area );
				}
			}

			start.y += navgenparams->generation_step_size;
		}
		start.x += navgenparams->generation_step_size;
	}
	return true;
}

bool CollectOverlappingAreas::operator()(CNavArea* area)
{
	CNavArea* overlappingArea = NULL;
	CNavLadder* overlappingLadder = NULL;

	Vector nw = area->GetCorner(NORTH_WEST);
	Vector se = area->GetCorner(SOUTH_EAST);
	Vector start = nw;
	start.x += navgenparams->generation_step_size / 2;
	start.y += navgenparams->generation_step_size / 2;

	while (start.x < se.x)
	{
		start.y = nw.y + navgenparams->generation_step_size / 2;
		while (start.y < se.y)
		{
			start.z = area->GetZ(start.x, start.y);
			Vector end = start;
			start.z -= navgenparams->step_height;
			end.z += navgenparams->human_height;

			if (TheNavMesh->FindNavAreaOrLadderAlongRay(start, end, &overlappingArea, &overlappingLadder, area))
			{
				if (overlappingArea)
				{
					if (added_areas.find(overlappingArea->GetID()) == added_areas.end())
					{
						this->overlapping_areas.push_back(overlappingArea);
						added_areas.insert(overlappingArea->GetID());
					}

					if (added_areas.find(area->GetID()) == added_areas.end())
					{
						this->overlapping_areas.push_back(area);
						added_areas.insert(area->GetID());
					}
				}
			}

			start.y += navgenparams->generation_step_size;
		}
		start.x += navgenparams->generation_step_size;
	}
	return true;
}

//--------------------------------------------------------------------------------------------------------------
static void CommandNavSelectOverlapping( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;
	TheNavMesh->ClearSelectedSet();

	SelectOverlappingAreas overlapCheck;
	TheNavMesh->ForAllAreas( overlapCheck );

	Msg( "%d overlapping areas selected\n", TheNavMesh->GetSelecteSetSize() );
}
static ConCommand sm_nav_select_overlapping( "sm_nav_select_overlapping", CommandNavSelectOverlapping, "Selects nav areas that are overlapping others.", FCVAR_GAMEDLL );


//--------------------------------------------------------------------------------------------------------

#define MASK_NAV_VISION				(MASK_BLOCKLOS_AND_NPCS|CONTENTS_IGNORE_NODRAW_OPAQUE)

//--------------------------------------------------------------------------------------------------------
Vector CNavArea::GetRandomPoint( void ) const
{
	Extent extent;
	GetExtent( &extent );

	Vector spot;
	spot.x = randomgen->GetRandomReal<float>(extent.lo.x, extent.hi.x);
	spot.y = randomgen->GetRandomReal<float>(extent.lo.y, extent.hi.y);
	spot.z = GetZ( spot.x, spot.y );

	return spot;
}

void CNavArea::RegisterNavBlocker(INavBlocker* blocker)
{
	for (INavBlocker* it : m_navblockers)
	{
		if (it == blocker)
		{
			return;
		}
	}

	m_navblockers.push_back(blocker);
}

void CNavArea::UnregisterNavBlocker(INavBlocker* blocker)
{
	m_navblockers.erase(std::remove_if(m_navblockers.begin(), m_navblockers.end(), [&blocker](const INavBlocker* it) {
		return it == blocker;
	}), m_navblockers.end());
}

void CNavArea::RegisterNavPathCostMod(const INavPathCostMod* pathcostmod)
{
	for (const INavPathCostMod* mod : m_navpathcostmods)
	{
		if (mod == pathcostmod)
		{
			return;
		}
	}

	m_navpathcostmods.push_back(pathcostmod);
}

void CNavArea::UnregisterNavPathCostMod(const INavPathCostMod* pathcostmod)
{
	m_navpathcostmods.erase(std::remove_if(m_navpathcostmods.begin(), m_navpathcostmods.end(), [&pathcostmod](const INavPathCostMod* it) {
		return it == pathcostmod;
	}), m_navpathcostmods.end());
}

void CNavArea::GetPathCost(const float originalCost, float& cost, CBaseBot* bot) const
{
	float out = originalCost;
	const int team = bot != nullptr ? bot->GetCurrentTeamIndex() : NAV_TEAM_ANY;

	for (const INavPathCostMod* mod : m_navpathcostmods)
	{
		if (mod->IsEnabled() && mod->AppliesTo(team, bot))
		{
			out = mod->GetCostMod(this, originalCost, out, team, bot);
		}
	}

	cost = out;
}
