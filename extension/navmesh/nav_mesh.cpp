//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
// NavMesh.cpp
// Implementation of Navigation Mesh interface
// Author: Michael S. Booth, 2003-2004

#include "extension.h"
#include <manager.h>
#include <mods/basemod.h>
#include <util/helpers.h>
#include <util/librandom.h>
#include <sdkports/debugoverlay_shared.h>
#include <sdkports/sdk_traces.h>
#include <entities/baseentity.h>

#include "nav_mesh.h"
#include "nav_trace.h"
#include "nav_area.h"
#include "nav_node.h"
#include "nav_waypoint.h"
#include "nav_volume.h"
#include "nav_elevator.h"
#include "nav_place_loader.h"
#include "nav_prereq.h"
#include <utlbuffer.h>
#include <utlhash.h>
#include <generichash.h>
#include <fmtstr.h>

#define DrawLine( from, to, duration, red, green, blue )		debugoverlay->AddLineOverlay( from, to, red, green, blue, true, NDEBUG_PERSIST_FOR_ONE_TICK )

/**
 * The singleton for accessing the navigation mesh
 */
extern CNavMesh *TheNavMesh;

ConVar sm_nav_edit( "sm_nav_edit", "0", FCVAR_GAMEDLL | FCVAR_CHEAT, "Set to one to interactively edit the Navigation Mesh. Set to zero to leave edit mode." );
ConVar sm_nav_quicksave( "sm_nav_quicksave", "1", FCVAR_GAMEDLL | FCVAR_CHEAT, "Set to one to skip the time consuming phases of the analysis.  Useful for data collection and testing." );	// TERROR: defaulting to 1, since we don't need the other data
ConVar sm_nav_show_approach_points( "sm_nav_show_approach_points", "0", FCVAR_GAMEDLL | FCVAR_CHEAT, "Show Approach Points in the Navigation Mesh." );
ConVar sm_nav_show_danger( "sm_nav_show_danger", "0", FCVAR_GAMEDLL | FCVAR_CHEAT, "Show current 'danger' levels." );
ConVar sm_nav_show_player_counts( "sm_nav_show_player_counts", "0", FCVAR_GAMEDLL | FCVAR_CHEAT, "Show current player counts in each area." );
ConVar sm_nav_show_func_nav_avoid( "sm_nav_show_func_nav_avoid", "0", FCVAR_GAMEDLL | FCVAR_CHEAT, "Show areas of designer-placed bot avoidance due to func_nav_avoid entities" );
ConVar sm_nav_show_func_nav_prefer( "sm_nav_show_func_nav_prefer", "0", FCVAR_GAMEDLL | FCVAR_CHEAT, "Show areas of designer-placed bot preference due to func_nav_prefer entities" );
ConVar sm_nav_show_func_nav_prerequisite( "sm_nav_show_func_nav_prerequisite", "0", FCVAR_GAMEDLL | FCVAR_CHEAT, "Show areas of designer-placed bot preference due to func_nav_prerequisite entities" );
ConVar sm_nav_max_vis_delta_list_length( "sm_nav_max_vis_delta_list_length", "64", FCVAR_CHEAT );


extern ConVar sm_nav_show_potentially_visible;
extern NavAreaVector TheNavAreas;
#ifdef STAGING_ONLY
int g_DebugPathfindCounter = 0;
#endif


bool NavAttributeClearer::operator() ( CNavArea *area )
{
	area->SetAttributes( area->GetAttributes() & (~m_attribute) );

	return true;
}

bool NavAttributeSetter::operator() ( CNavArea *area )
{
	area->SetAttributes( area->GetAttributes() | m_attribute );

	return true;
}

template<typename Functor>
bool ForEachActor(Functor &func) {
	// iterate all non-bot players
	for (int i = 1; i <= gpGlobals->maxClients; ++i) {
		edict_t* ent = gamehelpers->EdictOfIndex(i);
		if (ent == nullptr) {
			continue;
		}
		IPlayerInfo* player = playerinfomanager->GetPlayerInfo(ent);
		if (player == NULL || !player->IsPlayer() || !player->IsConnected())
			continue;
#ifdef NEXT_BOT
		// skip bots - ForEachCombatCharacter will catch them
		INextBot *bot = player->MyNextBotPointer();
		if ( bot )
		{
			continue;
		}
#endif // NEXT_BOT
		if (!func(ent)) {
			return false;
		}
	}

#ifdef NEXT_BOT
	// iterate all NextBots
	return TheNextBots().ForEachCombatCharacter( func );
#else
	return true;
#endif // NEXT_BOT
}

template bool ForEachActor(EditDestroyNotification&);
template bool ForEachActor(ForgetArea&);

//--------------------------------------------------------------------------------------------------------------
CNavMesh::CNavMesh( void )
{
	m_gridCellSize = 300.0f;
	m_editMode = NORMAL;
	m_bQuitWhenFinished = false;
	m_hostThreadModeRestoreValue = 0;
	// m_placeCount = 0;
	// m_placeName = NULL;
	m_invokeAreaUpdateTimer.Start(NAV_AREA_UPDATE_INTERVAL);
	m_invokeWaypointUpdateTimer.Start(CWaypoint::UPDATE_INTERVAL);
	m_invokeVolumeUpdateTimer.Start(CNavVolume::UPDATE_INTERVAL);
	m_invokeElevatorUpdateTimer.Start(CNavElevator::UPDATE_INTERVAL);
	m_linkorigin = vec3_origin;
	m_placeMap.reserve(512);
	m_waypoints.reserve(128);
	m_volumes.reserve(8);
	m_elevators.reserve(8);
	m_prerequisites.reserve(8);
	m_selectedWaypoint = nullptr;
	m_selectedVolume = nullptr;
	m_selectedElevator = nullptr;
	m_selectedPrerequisite = nullptr;
		
	Reset();

	ListenForGameEvent("round_start");
	ListenForGameEvent("dod_round_start");
	ListenForGameEvent("teamplay_round_start");

	// Default walkable entities for generation
	AddWalkableEntity("info_player_start");
	AddWalkableEntity("info_player_teamspawn");
}

//--------------------------------------------------------------------------------------------------------------
CNavMesh::~CNavMesh()
{
}

bool CNavMesh::IsEntityWalkable(CBaseEntity* pEntity, unsigned int flags)
{
	entities::HBaseEntity be(pEntity);

	if (UtilHelpers::FClassnameIs(pEntity, "worldspawn"))
		return false;

	if (UtilHelpers::FClassnameIs(pEntity, "player"))
		return false;

	// if we hit a door, assume its walkable because it will open when we touch it
	if (UtilHelpers::FClassnameIs(pEntity, "func_door*"))
	{
#ifdef PROBLEMATIC	// cp_dustbowl doors dont open by touch - they use surrounding triggers
		if (!entity->HasSpawnFlags(SF_DOOR_PTOUCH))
		{
			// this door is not opened by touching it, if it is closed, the area is blocked
			CBaseDoor* door = (CBaseDoor*)entity;
			return door->m_toggle_state == TS_AT_TOP;
		}
#endif // _DEBUG

		return (flags & WALK_THRU_FUNC_DOORS) ? true : false;
	}

	if (UtilHelpers::FClassnameIs(pEntity, "prop_door*"))
	{
		return (flags & WALK_THRU_PROP_DOORS) ? true : false;
	}

	// if we hit a clip brush, ignore it if it is not BRUSHSOLID_ALWAYS
	if (UtilHelpers::FClassnameIs(pEntity, "func_brush"))
	{
		entities::HFuncBrush brush(pEntity);

		switch (brush.GetSolidity())
		{
		case entities::HFuncBrush::BrushSolidities_e::BRUSHSOLID_ALWAYS:
			return false;
		case entities::HFuncBrush::BrushSolidities_e::BRUSHSOLID_NEVER:
			return true;
		case entities::HFuncBrush::BrushSolidities_e::BRUSHSOLID_TOGGLE:
		default:
			return (flags & WALK_THRU_TOGGLE_BRUSHES) ? true : false;
		}
	}

	// if we hit a breakable object, assume its walkable because we will shoot it when we touch it
	if (UtilHelpers::FClassnameIs(pEntity, "func_breakable") && be.GetHealth() > 0 && be.GetTakeDamage() == DAMAGE_YES)
		return (flags & WALK_THRU_BREAKABLES) ? true : false;

	if (UtilHelpers::FClassnameIs(pEntity, "func_breakable_surf") && be.GetTakeDamage() == DAMAGE_YES)
		return (flags & WALK_THRU_BREAKABLES) ? true : false;

	if (UtilHelpers::FClassnameIs(pEntity, "func_playerinfected_clip") == true)
		return true;

	ConVarRef solidprops("sm_nav_solid_props", false);

	if (solidprops.GetBool() && UtilHelpers::FClassnameIs(pEntity, "prop_*"))
		return true;

	return false;
}

void CNavMesh::Precache()
{
}

void CNavMesh::OnMapStart()
{
	LoadPlaceDatabase();

	NavErrorType error = NAV_CORRUPT_DATA;
	
	try
	{
		error = Load();
	}
	catch (const std::ios_base::failure& ex)
	{
		smutils->LogError(myself, "Exception throw while reading navigation mesh file: %s", ex.what());
		Reset();
		error = NAV_CORRUPT_DATA;
	}
	catch (const std::exception& ex)
	{
		smutils->LogError(myself, "Failed to load navigation mesh: %s", ex.what());
		Reset();
		error = NAV_CORRUPT_DATA;
	}

	switch (error)
	{
	case NAV_OK:
		rootconsole->ConsolePrint("[NavBot] Nav mesh loaded successfully.");
		break;
	case NAV_CANT_ACCESS_FILE: // don't log this as error, just warn on the console
		rootconsole->ConsolePrint("[Navbot] Failed to load nav mesh: File not found.");
		break;
	case NAV_INVALID_FILE:
		smutils->LogError(myself, "Failed to load nav mesh: File is invalid.");
		break;
	case NAV_BAD_FILE_VERSION:
		smutils->LogError(myself, "Failed to load nav mesh: Invalid file version.");
		break;
	case NAV_FILE_OUT_OF_DATE:
		smutils->LogError(myself, "Failed to load nav mesh: Nav mesh is out of date.");
		break;
	case NAV_CORRUPT_DATA:
		smutils->LogError(myself, "Failed to load nav mesh: File is corrupt.");
		break;
	case NAV_OUT_OF_MEMORY:
		smutils->LogError(myself, "Failed to load nav mesh: Out of memory.");
		break;
	default:
		break;
	}

	// Sourcemod's OnMapStart is called on a ServerActivate hook
	OnServerActivate(); // this isn't called anywhere else so just call it here
}

void CNavMesh::OnMapEnd()
{
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Reset the Navigation Mesh to initial values
 */
void CNavMesh::Reset( void )
{
	DestroyNavigationMesh();

	m_generationMode = GENERATE_NONE;
	m_currentNode = NULL;
	ClearWalkableSeeds();

	m_isAnalyzed = false;
	m_isOutOfDate = false;
	m_isEditing = false;
	m_navPlace = UNDEFINED_PLACE;
	m_markedArea = NULL;
	m_selectedArea = NULL;
	m_bQuitWhenFinished = false;

	m_editMode = NORMAL;

	m_lastSelectedArea = NULL;
	m_isPlacePainting = false;

	m_climbableSurface = false;
	m_markedLadder = NULL;
	m_selectedLadder = NULL;

	m_updateBlockedAreasTimer.Invalidate();

	m_walkableSeeds.RemoveAll();
}


//--------------------------------------------------------------------------------------------------------------
CNavArea *CNavMesh::GetMarkedArea( void ) const
{
	if ( m_markedArea )
	{
		return m_markedArea;
	}

	if ( m_selectedSet.Count() == 1 )
	{
		return m_selectedSet[0];
	}

	return NULL;
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Free all resources of the mesh and reset it to empty state
 */
void CNavMesh::DestroyNavigationMesh( bool incremental )
{
	// these needs the nav area pointers to still be valid since some of them notify their destruction via the destructor
	m_selectedWaypoint = nullptr;
	m_selectedVolume = nullptr;
	m_selectedElevator = nullptr;
	m_selectedPrerequisite = nullptr;

	if (!incremental)
	{
		m_waypoints.clear();
		m_volumes.clear();
		m_elevators.clear();
		m_prerequisites.clear();
	}

	m_blockedAreas.RemoveAll();
	m_avoidanceObstacleAreas.RemoveAll();
	m_transientAreas.clear();

	if ( !incremental )
	{
		// destroy all areas
		CNavArea::m_isReset = true;

		// tell players to forget about the areas
		FOR_EACH_VEC( TheNavAreas, it )
		{
			EditDestroyNotification notification( TheNavAreas[it] );
			ForEachActor( notification );
		}

		// remove each element of the list and delete them
		FOR_EACH_VEC( TheNavAreas, it )
		{
			DestroyArea( TheNavAreas[ it ] );
		}

		TheNavAreas.RemoveAll();

		CNavArea::m_isReset = false;


		// destroy ladder representations
		DestroyLadders();
	}
	else
	{
		FOR_EACH_VEC( TheNavAreas, it )
		{
			TheNavAreas[ it ]->ResetNodes();
		}
	}

	// destroy all hiding spots
	DestroyHidingSpots();

	// destroy navigation nodes created during map generation
	CNavNode::CleanupGeneration();

	if ( !incremental )
	{
		// destroy the grid
		m_grid.RemoveAll();
		m_gridSizeX = 0;
		m_gridSizeY = 0;
	}

	// clear the hash table
	for( int i=0; i<HASH_TABLE_SIZE; ++i )
	{
		m_hashTable[i] = NULL;
	}

	if ( !incremental )
	{
		m_areaCount = 0;
		// Reset the next area and ladder IDs to 1
		CNavArea::CompressIDs(this);
		CNavLadder::CompressIDs(this);
		m_isLoaded = false;
	}

	SetEditMode( NORMAL );

	m_markedArea = NULL;
	m_selectedArea = NULL;
	m_lastSelectedArea = NULL;
	m_climbableSurface = false;
	m_markedLadder = NULL;
	m_selectedLadder = NULL;

	extmanager->GetMod()->OnNavMeshDestroyed();
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Invoked on each game frame
 */
void CNavMesh::Update( void )
{
	if (IsGenerating())
	{
		UpdateGeneration( 0.03 );
		return; // don't bother trying to draw stuff while we're generating
	}

	// Test all of the areas for blocked status
	if ( m_updateBlockedAreasTimer.HasStarted() && m_updateBlockedAreasTimer.IsElapsed() )
	{
		TestAllAreasForBlockedStatus();
		m_updateBlockedAreasTimer.Start(2.0f);
	}

	UpdateBlockedAreas();
	UpdateAvoidanceObstacleAreas();

	if (sm_nav_edit.GetBool())
	{
		if (m_isEditing == false)
		{
			OnEditModeStart();
			m_isEditing = true;
		}

		DrawEditMode();

		if (sm_nav_waypoint_edit.GetBool())
		{
			DrawWaypoints();
		}

		if (sm_nav_volume_edit.GetBool())
		{
			DrawVolumes();
		}

		if (sm_nav_elevator_edit.GetBool())
		{
			DrawElevators();
		}

		if (sm_nav_prerequisite_edit.GetBool())
		{
			DrawPrerequisites();
		}
	}
	else
	{
		if (m_isEditing)
		{
			OnEditModeEnd();
			m_isEditing = false;
		}
	}

	if (sm_nav_show_danger.GetBool())
	{
		DrawDanger();
	}

	if (sm_nav_show_player_counts.GetBool())
	{
		DrawPlayerCounts();
	}

	if (sm_nav_show_func_nav_avoid.GetBool())
	{
		DrawFuncNavAvoid();
	}

	if (sm_nav_show_func_nav_prefer.GetBool())
	{
		DrawFuncNavPrefer();
	}

	{
		FOR_EACH_VEC(TheNavAreas, it)
		{
			CNavArea* area = TheNavAreas[it];

			area->OnFrame();

			if (m_invokeAreaUpdateTimer.IsElapsed())
			{
				area->OnUpdate();
			}
		}

		if (m_invokeAreaUpdateTimer.IsElapsed())
		{
			m_invokeAreaUpdateTimer.Start(NAV_AREA_UPDATE_INTERVAL);
		}
	}

	if (m_invokeWaypointUpdateTimer.IsElapsed())
	{
		m_invokeWaypointUpdateTimer.Start(CWaypoint::UPDATE_INTERVAL);

		std::for_each(m_waypoints.begin(), m_waypoints.end(), [](const std::pair<WaypointID, std::shared_ptr<CWaypoint>>& object) {
			object.second->Update();
		});
	}

	if (m_invokeVolumeUpdateTimer.IsElapsed())
	{
		m_invokeVolumeUpdateTimer.Start(CNavVolume::UPDATE_INTERVAL);

		std::for_each(m_volumes.begin(), m_volumes.end(), [](const std::pair<unsigned int, std::shared_ptr<CNavVolume>>& object) {
			object.second->Update();
		});
	}

	if (m_invokeElevatorUpdateTimer.IsElapsed())
	{
		m_invokeElevatorUpdateTimer.Start(CNavElevator::UPDATE_INTERVAL);

		std::for_each(m_elevators.begin(), m_elevators.end(), [](const std::pair<unsigned int, std::shared_ptr<CNavElevator>>& object) {
			object.second->Update();
		});
	}

	// draw any walkable seeds that have been marked
	for ( int it=0; it < m_walkableSeeds.Count(); ++it )
	{
		WalkableSeedSpot spot = m_walkableSeeds[ it ];

		const float height = 50.0f;
		const float width = 25.0f;
		DrawLine( spot.pos, spot.pos + height * spot.normal, 3, 255, 0, 255 ); 
		DrawLine( spot.pos + Vector( width, 0, 0 ), spot.pos + height * spot.normal, 3, 255, 0, 255 ); 
		DrawLine( spot.pos + Vector( -width, 0, 0 ), spot.pos + height * spot.normal, 3, 255, 0, 255 ); 
		DrawLine( spot.pos + Vector( 0, width, 0 ), spot.pos + height * spot.normal, 3, 255, 0, 255 ); 
		DrawLine( spot.pos + Vector( 0, -width, 0 ), spot.pos + height * spot.normal, 3, 255, 0, 255 ); 
	}
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Check all nav areas inside the breakable's extent to see if players would now fall through
 */
class CheckAreasOverlappingBreakable
{
public:
	CheckAreasOverlappingBreakable( edict_t *breakable )
	{
		m_breakable = breakable;
		ICollideable *collideable = breakable->GetCollideable();
		collideable->WorldSpaceSurroundingBounds( &m_breakableExtent.lo, &m_breakableExtent.hi );

		const float expand = 10.0f;
		m_breakableExtent.lo += Vector( -expand, -expand, -expand );
		m_breakableExtent.hi += Vector(  expand,  expand,  expand );
	}

	bool operator() ( CNavArea *area )
	{
		if ( area->IsOverlapping( m_breakableExtent ) )
		{
			// area overlaps the breakable
			area->CheckFloor( m_breakable );
		}

		return true;
	}

private:
	Extent m_breakableExtent;
	edict_t *m_breakable;
};


//--------------------------------------------------------------------------------------------------------------
class NavRoundRestart
{
public:
	bool operator()( CNavArea *area )
	{
		area->OnRoundRestart();
		return true;
	}

	bool operator()( CNavLadder *ladder )
	{
		ladder->OnRoundRestart();
		return true;
	}
};


//--------------------------------------------------------------------------------------------------------------
/**
 * Invoked when the round restarts
 */

void CNavMesh::FireGameEvent(IGameEvent* event)
{
	// For some reason the engine accepts null events
	if (event == nullptr)
		return;

	auto name = event->GetName();

	if (strncmp(name, "round_start", 11) == 0 || strncmp(name, "dod_round_start", 15) == 0 || strncmp(name, "teamplay_round_start", 20) == 0)
	{
		OnRoundRestart();
		PropagateOnRoundRestart();
	}
}

#if SOURCE_ENGINE >= SE_LEFT4DEAD
int CNavMesh::GetEventDebugID(void)
{
	return EVENT_DEBUG_ID_INIT;
}
#endif // SOURCE_ENGINE >= SE_LEFT4DEAD


//--------------------------------------------------------------------------------------------------------------
/**
 * Allocate the grid and define its extents
 */
void CNavMesh::AllocateGrid( float minX, float maxX, float minY, float maxY )
{
	m_grid.RemoveAll();

	m_minX = minX;
	m_minY = minY;

	m_gridSizeX = (int)((maxX - minX) / m_gridCellSize) + 1;
	m_gridSizeY = (int)((maxY - minY) / m_gridCellSize) + 1;

	m_grid.SetCount( m_gridSizeX * m_gridSizeY );
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Add an area to the mesh
 */
void CNavMesh::AddNavArea( CNavArea *area )
{
	if ( !m_grid.Count() )
	{
		// If we somehow have no grid (manually creating a nav area without loading or generating a mesh), don't crash
		AllocateGrid( 0, 0, 0, 0 );
	}

	// add to grid
	int loX = WorldToGridX( area->GetCorner( NORTH_WEST ).x );
	int loY = WorldToGridY( area->GetCorner( NORTH_WEST ).y );
	int hiX = WorldToGridX( area->GetCorner( SOUTH_EAST ).x );
	int hiY = WorldToGridY( area->GetCorner( SOUTH_EAST ).y );

	for( int y = loY; y <= hiY; ++y )
	{
		for( int x = loX; x <= hiX; ++x )
		{
			m_grid[ x + y*m_gridSizeX ].AddToTail( const_cast<CNavArea *>( area ) );
		}
	}

	// add to hash table
	int key = ComputeHashKey( area->GetID() );

	if (m_hashTable[key])
	{
		// add to head of list in this slot
		area->m_prevHash = NULL;
		area->m_nextHash = m_hashTable[key];
		m_hashTable[key]->m_prevHash = area;
		m_hashTable[key] = area;
	}
	else
	{
		// first entry in this slot
		m_hashTable[key] = area;
		area->m_nextHash = NULL;
		area->m_prevHash = NULL;
	}

	if ( area->GetAttributes() & NAV_MESH_TRANSIENT )
	{
		m_transientAreas.push_back(area);
	}

	++m_areaCount;
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Remove an area from the mesh
 */
void CNavMesh::RemoveNavArea( CNavArea *area )
{
	// add to grid
	int loX = WorldToGridX( area->GetCorner( NORTH_WEST ).x );
	int loY = WorldToGridY( area->GetCorner( NORTH_WEST ).y );
	int hiX = WorldToGridX( area->GetCorner( SOUTH_EAST ).x );
	int hiY = WorldToGridY( area->GetCorner( SOUTH_EAST ).y );

	for( int y = loY; y <= hiY; ++y )
	{
		for( int x = loX; x <= hiX; ++x )
		{
			m_grid[ x + y*m_gridSizeX ].FindAndRemove( area );
		}
	}

	// remove from hash table
	int key = ComputeHashKey( area->GetID() );

	if (area->m_prevHash)
	{
		area->m_prevHash->m_nextHash = area->m_nextHash;
	}
	else
	{
		// area was at start of list
		m_hashTable[key] = area->m_nextHash;

		if (m_hashTable[key])
		{
			m_hashTable[key]->m_prevHash = NULL;
		}
	}

	if (area->m_nextHash)
	{
		area->m_nextHash->m_prevHash = area->m_prevHash;
	}

	if ( area->GetAttributes() & NAV_MESH_TRANSIENT )
	{
		BuildTransientAreaList();
	}

	m_avoidanceObstacleAreas.FindAndRemove( area );
	m_blockedAreas.FindAndRemove( area );

	--m_areaCount;
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Invoked when server loads a new map
 */
void CNavMesh::OnServerActivate( void )
{
	FOR_EACH_VEC( TheNavAreas, pit )
	{
		CNavArea *area = TheNavAreas[ pit ];
		area->OnServerActivate();
	}

	BuildTransientAreaList();

	// Some mods may not have round start events, call this at least once on every new map load
	OnRoundRestart();
}

#ifdef NEXT_BOT

//--------------------------------------------------------------------------------------------------------------
class CRegisterPrerequisite
{
public:
	CRegisterPrerequisite( CFuncNavPrerequisite *prereq )
	{
		m_prereq = prereq;
	}

	bool operator() ( CNavArea *area )
	{
		area->AddPrerequisite( m_prereq );
		return true;
	}

	CFuncNavPrerequisite *m_prereq;
};

#endif

//--------------------------------------------------------------------------------------------------------------
/**
* Test all areas for blocked status
*/
void CNavMesh::TestAllAreasForBlockedStatus( void )
{
	FOR_EACH_VEC( TheNavAreas, pit )
	{
		CNavArea *area = TheNavAreas[ pit ];
		area->UpdateBlocked( true );
	}
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Invoked when a game round restarts
 */
void CNavMesh::OnRoundRestart( void )
{
#ifdef EXT_DEBUG
	Msg("CNavMesh::OnRoundRestart\n");
#endif // EXT_DEBUG

	m_updateBlockedAreasTimer.Start( 1.0f );
	m_invokeAreaUpdateTimer.Start(NAV_AREA_UPDATE_INTERVAL);
	m_invokeWaypointUpdateTimer.Start(CWaypoint::UPDATE_INTERVAL);
	m_invokeVolumeUpdateTimer.Start(CNavVolume::UPDATE_INTERVAL);
	m_invokeElevatorUpdateTimer.Start(CNavElevator::UPDATE_INTERVAL);

#ifdef NEXT_BOT
	FOR_EACH_VEC( TheNavAreas, pit )
	{
		CNavArea *area = TheNavAreas[ pit ];
		area->RemoveAllPrerequisites();
	}

	// attach prerequisites
	for ( int i=0; i<IFuncNavPrerequisiteAutoList::AutoList().Count(); ++i )
	{
		CFuncNavPrerequisite *prereq = static_cast< CFuncNavPrerequisite* >( IFuncNavPrerequisiteAutoList::AutoList()[i] );

		Extent prereqExtent;
		prereqExtent.Init( prereq );

		CRegisterPrerequisite apply( prereq );

		ForAllAreasOverlappingExtent( apply, prereqExtent );
	}
#endif
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Invoked when a game round restarts, but before entities are deleted and recreated
 */
void CNavMesh::OnRoundRestartPreEntity( void )
{
}

void CNavMesh::PropagateOnRoundRestart()
{
#ifdef EXT_DEBUG
	Msg("CNavMesh::PropagateOnRoundRestart\n");
#endif // EXT_DEBUG

	NavRoundRestart restart;
	ForAllAreas(restart);
	ForAllLadders(restart);

	std::for_each(m_waypoints.begin(), m_waypoints.end(), [](const std::pair<WaypointID, std::shared_ptr<CWaypoint>>& object) {
		object.second->OnRoundRestart();
	});

	std::for_each(m_elevators.begin(), m_elevators.end(), [](const std::pair<WaypointID, std::shared_ptr<CNavElevator>>& object) {
		object.second->OnRoundRestart();
	});

	std::for_each(m_volumes.begin(), m_volumes.end(), [](const std::pair<WaypointID, std::shared_ptr<CNavVolume>>& object) {
		object.second->OnRoundRestart();
	});

	std::for_each(m_prerequisites.begin(), m_prerequisites.end(), [](const std::pair<WaypointID, std::shared_ptr<CNavPrerequisite>>& object) {
		object.second->OnRoundRestart();
	});
}

//--------------------------------------------------------------------------------------------------------------
void CNavMesh::BuildTransientAreaList( void )
{
	m_transientAreas.clear();

	FOR_EACH_VEC( TheNavAreas, it )
	{
		CNavArea *area = TheNavAreas[ it ];

		if ( area->GetAttributes() & NAV_MESH_TRANSIENT )
		{
			m_transientAreas.push_back(area);
		}
	}
}

//--------------------------------------------------------------------------------------------------------------
inline void CNavMesh::GridToWorld( int gridX, int gridY, Vector *pos ) const
{
	gridX = clamp( gridX, 0, m_gridSizeX-1 );
	gridY = clamp( gridY, 0, m_gridSizeY-1 );

	pos->x = m_minX + gridX * m_gridCellSize;
	pos->y = m_minY + gridY * m_gridCellSize;
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Given a position, return the nav area that IsOverlapping and is *immediately* beneath it
 */
CNavArea *CNavMesh::GetNavArea( const Vector &pos, float beneathLimit ) const
{
	if ( !m_grid.Count() )
		return NULL;

	// get list in cell that contains position
	int x = WorldToGridX( pos.x );
	int y = WorldToGridY( pos.y );
	NavAreaVector *areaVector = &m_grid[ x + y*m_gridSizeX ];

	// search cell list to find correct area
	CNavArea *use = NULL;
	float useZ = -99999999.9f;
	Vector testPos = pos + Vector( 0, 0, 5 );

	FOR_EACH_VEC( (*areaVector), it )
	{
		CNavArea *area = (*areaVector)[ it ];

		// check if position is within 2D boundaries of this area
		if (area->IsOverlapping( testPos ))
		{
			// project position onto area to get Z
			float z = area->GetZ( testPos );

			// don't use area  above us
			if (z <= testPos.z
			// don't use area is too far below us
					&& z >= pos.z - beneathLimit
					// if area is higher than the one we have, use this instead
					&& z > useZ)
			{
				use = area;
				useZ = z;
			}
		}
	}

	return use;
}


//----------------------------------------------------------------------------
// Given a position, return the nav area that IsOverlapping and is *immediately* beneath it
//----------------------------------------------------------------------------
CNavArea *CNavMesh::GetNavArea( edict_t *pEntity, int nFlags, float flBeneathLimit ) const
{
	if ( !m_grid.Count() )
		return NULL;

	IPlayerInfo *pBCC = playerinfomanager->GetPlayerInfo(pEntity);
	Vector testPos = pEntity->GetCollideable()->GetCollisionOrigin();

	float flStepHeight = 1e-3;
	bool isPlayer = UtilHelpers::IsPlayerIndex(gamehelpers->IndexOfEdict(pEntity));

	if ( isPlayer )
	{
		/**
		 * TODO
		// Check if we're still in the last area
		CNavArea *pLastNavArea = pBCC->GetLastKnownArea();
		if ( pLastNavArea && pLastNavArea->IsOverlapping( testPos ) )
		{
			float flZ = pLastNavArea->GetZ( testPos );
			if ( ( flZ <= testPos.z + navgenparams->step_height ) && ( flZ >= testPos.z - navgenparams->step_height ) )
				return pLastNavArea;
		}
		 */
		flStepHeight = navgenparams->step_height;
	}

	// get list in cell that contains position
	int x = WorldToGridX( testPos.x );
	int y = WorldToGridY( testPos.y );
	NavAreaVector *areaVector = &m_grid[ x + y*m_gridSizeX ];

	// search cell list to find correct area
	CNavArea *use = NULL;
	float useZ = -99999999.9f;

	bool bSkipBlockedAreas = ( ( nFlags & GETNAVAREA_ALLOW_BLOCKED_AREAS ) == 0 );
	FOR_EACH_VEC( (*areaVector), it )
	{
		CNavArea *pArea = (*areaVector)[ it ];

		// check if position is within 2D boundaries of this area
		if ( !pArea->IsOverlapping( testPos )
		// don't consider blocked areas
				|| ( bSkipBlockedAreas && isPlayer
						&& pArea->IsBlocked( pBCC->GetTeamIndex() ) ))
			continue;

		// project position onto area to get Z
		float z = pArea->GetZ( testPos );

		// if area is above us, skip it
		if ( z > testPos.z + flStepHeight
		// if area is too far below us, skip it
				|| z < testPos.z - flBeneathLimit
		// if area is lower than the one we have, skip it
				|| z <= useZ )
			continue;

		use = pArea;
		useZ = z;
	}

	// Check LOS if necessary
	if ( use && ( nFlags & GETNAVAREA_CHECK_LOS ) && ( useZ < testPos.z - flStepHeight ) )
	{
		// trace directly down to see if it's below us and unobstructed
		trace_t result;
		trace::line(testPos, Vector(testPos.x, testPos.y, useZ), MASK_NPCSOLID_BRUSHONLY, nullptr, COLLISION_GROUP_NONE, result);

		if ( ( result.fraction != 1.0f ) && ( fabs( result.endpos.z - useZ ) > flStepHeight ) )
			return NULL;
	}
	return use;
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Given a position in the world, return the nav area that is closest
 * and at the same height, or beneath it.
 * Used to find initial area if we start off of the mesh.
 * @todo Make sure area is not on the other side of the wall from goal.
 */
CNavArea *CNavMesh::GetNearestNavArea( const Vector &pos, float maxDist, bool checkLOS, bool checkGround, int team ) const
{
	if ( !m_grid.Count() )
		return NULL;	

	CNavArea *close = NULL;
	float closeDistSq = maxDist * maxDist;

	// quick check
	if ( !checkLOS && !checkGround )
	{
		close = GetNavArea( pos );
		if ( close )
		{
			return close;
		}
	}

	// ensure source position is well behaved
	Vector source;
	source.x = pos.x;
	source.y = pos.y;
	if ( !GetGroundHeight( pos, &source.z ) )
	{
		if ( !checkGround )
		{
			source.z = pos.z;
		}
		else
		{
			return NULL;
		}
	}

	source.z += navgenparams->human_height;

	// find closest nav area

	// use a unique marker for this method, so it can be used within a SearchSurroundingArea() call
	static unsigned int searchMarker = librandom::generate_random_int(0, 1024 * 1024);

	++searchMarker;

	if ( searchMarker == 0 )
	{
		++searchMarker;
	}


	// get list in cell that contains position
	int originX = WorldToGridX( pos.x );
	int originY = WorldToGridY( pos.y );

	int shiftLimit = ceil(maxDist / m_gridCellSize);

	//
	// Search in increasing rings out from origin, starting with cell
	// that contains the given position.
	// Once we find a close area, we must check one more step out in
	// case our position is just against the edge of the cell boundary
	// and an area in an adjacent cell is actually closer.
	// 
	for( int shift=0; shift <= shiftLimit; ++shift )
	{
		for( int x = originX - shift; x <= originX + shift; ++x )
		{
			if ( x < 0 || x >= m_gridSizeX )
				continue;

			for( int y = originY - shift; y <= originY + shift; ++y )
			{
				if ( y < 0 || y >= m_gridSizeY

				// only check these areas if we're on the outer edge of our spiral
						|| (x > originX - shift &&
					 x < originX + shift &&
					 y > originY - shift &&
					 y < originY + shift) )
					continue;

				NavAreaVector *areaVector = &m_grid[ x + y*m_gridSizeX ];

				// find closest area in this cell
				FOR_EACH_VEC( (*areaVector), it )
				{
					CNavArea *area = (*areaVector)[ it ];

					// skip if we've already visited this area
					if ( area->m_nearNavSearchMarker == searchMarker
							// don't consider blocked areas
							|| area->IsBlocked( team )
							// don't consider area that is overhead
							|| area->GetCenter().z - pos.z > navgenparams->human_height)
						continue;

					// mark as visited
					area->m_nearNavSearchMarker = searchMarker;

					Vector areaPos;
					area->GetClosestPointOnArea( source, &areaPos );

					// TERROR: Using the original pos for distance calculations.  Since it's a pure 3D distance,
					// with no Z restrictions or LOS checks, this should work for passing in bot foot positions.
					// This needs to be ported back to CS:S.
					float distSq = ( areaPos - pos ).LengthSqr();

					// keep the closest area
					if ( distSq >= closeDistSq )
						continue;

					// check LOS to area
					// REMOVED: If we do this for !anyZ, it's likely we wont have LOS and will enumerate every area in the mesh
					// It is still good to do this in some isolated cases, however
					if ( checkLOS )
					{
						trace_t result;

						// make sure 'pos' is not embedded in the world
						trace::line(pos, pos + Vector(0, 0, navgenparams->step_height), MASK_NPCSOLID_BRUSHONLY, nullptr, COLLISION_GROUP_NONE, result);

						// it was embedded - move it out
						Vector safePos = result.startsolid ? result.endpos + Vector( 0, 0, 1.0f )
									: pos;

						// Don't bother tracing from the nav area up to safePos.z if it's within navgenparams->step_height of the area, since areas can be embedded in the ground a bit
						float heightDelta = fabs(areaPos.z - safePos.z);
						if ( heightDelta > navgenparams->step_height )
						{
							// trace to the height of the original point
							trace::line(areaPos + Vector(0, 0, navgenparams->step_height), Vector(areaPos.x, areaPos.y, safePos.z), MASK_NPCSOLID_BRUSHONLY, nullptr, COLLISION_GROUP_NONE, result);
							
							if ( result.fraction != 1.0f )
							{
								continue;
							}
						}

						// trace to the original point's height above the area
						trace::line(safePos, Vector(areaPos.x, areaPos.y, safePos.z + navgenparams->step_height), MASK_NPCSOLID_BRUSHONLY, nullptr, COLLISION_GROUP_NONE, result);

						if ( result.fraction != 1.0f )
						{
							continue;
						}
					}

					closeDistSq = distSq;
					close = area;

					// look one more step outwards
					shiftLimit = shift+1;
				}
			}
		}
	}

	return close;
}


//----------------------------------------------------------------------------
// Given a position in the world, return the nav area that is closest
// and at the same height, or beneath it.
// Used to find initial area if we start off of the mesh.
// @todo Make sure area is not on the other side of the wall from goal.
//----------------------------------------------------------------------------
CNavArea *CNavMesh::GetNearestNavArea( edict_t *pEntity, int nFlags, float maxDist ) const
{
	if ( !m_grid.Count() )
		return NULL;

	// quick check
	CNavArea *pClose = GetNavArea( pEntity, nFlags );
	int index = gamehelpers->IndexOfEdict(pEntity);
	return pClose ? pClose
		: GetNearestNavArea(pEntity->GetCollideable()->GetCollisionOrigin(),
			maxDist,
			(nFlags & GETNAVAREA_CHECK_LOS) != 0,
			(nFlags & GETNAVAREA_CHECK_GROUND) != 0,
			index > 0 && index <= gpGlobals->maxClients
			? playerinfomanager->GetPlayerInfo(pEntity)->GetTeamIndex() : NAV_TEAM_ANY);
}

CNavArea* CNavMesh::GetNearestNavArea(CBaseEntity* entity, float maxDist, bool checkLOS, bool checkGround, int team) const
{
	entities::HBaseEntity be(entity);

	Vector origin = be.GetAbsOrigin();

	CNavArea* result = GetNearestNavArea(origin, maxDist, checkLOS, checkGround, team);

	return result;
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Given an ID, return the associated area
 */
CNavArea *CNavMesh::GetNavAreaByID( unsigned int id ) const
{
	if (id == 0)
		return NULL;

	int key = ComputeHashKey( id );

	for( CNavArea *area = m_hashTable[key]; area; area = area->m_nextHash )
	{
		if (area->GetID() == id)
		{
			return area;
		}
	}

	return NULL;
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Given an ID, return the associated ladder
 */
CNavLadder *CNavMesh::GetLadderByID( unsigned int id ) const
{
	if (id == 0)
		return NULL;

	for ( int i=0; i<m_ladders.Count(); ++i )
	{
		CNavLadder *ladder = m_ladders[i];
		if ( ladder->GetID() == id )
		{
			return ladder;
		}
	}

	return NULL;
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Return radio chatter place for given coordinate
 */
unsigned int CNavMesh::GetPlace( const Vector &pos ) const
{
	CNavArea *area = GetNearestNavArea( pos, true );
	return area != nullptr ?  area->GetPlace() : UNDEFINED_PLACE;
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Load the place names from a file
 */
void CNavMesh::LoadPlaceDatabase( void )
{
	NavPlaceDatabaseLoader loader;

	m_placeMap.clear();

	ParseGlobalPlaceDatabase(loader);
	ParseModPlaceDatabase(loader);
	ParseMapPlaceDatabase(loader);

	auto& places = loader.GetEntries();

	Place index = 1U;

	for (auto& entry : places)
	{
		std::string placeKey{ entry.first };

		auto pair = std::make_pair(placeKey, entry.second);

		m_placeMap[index] = pair;
		index++;
	}

#ifdef EXT_DEBUG
	smutils->LogMessage(myself, "Loaded place database with %i places.", m_placeMap.size());
#endif // EXT_DEBUG
}

void CNavMesh::ParseGlobalPlaceDatabase(NavPlaceDatabaseLoader& loader)
{
	std::unique_ptr<char[]> szPath = std::make_unique<char[]>(PLATFORM_MAX_PATH);
	smutils->BuildPath(SourceMod::Path_SM, szPath.get(), PLATFORM_MAX_PATH, "configs/navbot/nav_places.cfg");
	std::filesystem::path path{ szPath.get() };

	if (!loader.ParseFile(path))
	{
		smutils->LogError(myself, "Failed to parse Nav global place database.");
	}
}

void CNavMesh::ParseModPlaceDatabase(NavPlaceDatabaseLoader& loader)
{
	std::unique_ptr<char[]> szPath = std::make_unique<char[]>(PLATFORM_MAX_PATH);
	const char* mod = smutils->GetGameFolderName();
	smutils->BuildPath(SourceMod::Path_SM, szPath.get(), PLATFORM_MAX_PATH, "configs/navbot/%s/nav_places.cfg", mod);
	std::filesystem::path path{ szPath.get() };

	if (!loader.ParseFile(path))
	{
		rootconsole->ConsolePrint("Could not parse Nav mod specific place database. <%s>", szPath.get());
	}
}

void CNavMesh::ParseMapPlaceDatabase(NavPlaceDatabaseLoader& loader)
{
	std::unique_ptr<char[]> szPath = std::make_unique<char[]>(PLATFORM_MAX_PATH);
	std::string mapname = GetMapFileName();
	const char* mod = smutils->GetGameFolderName();
	smutils->BuildPath(SourceMod::Path_SM, szPath.get(), PLATFORM_MAX_PATH, "configs/navbot/%s/maps/nav_places_%s.cfg", mod, mapname.c_str());
	std::filesystem::path path{ szPath.get() };

	if (!loader.ParseFile(path))
	{
		rootconsole->ConsolePrint("Could not parse Nav map specific place database. <%s>", szPath.get());
	}
}

//--------------------------------------------------------------------------------------------------------------

const std::string* CNavMesh::GetPlaceName(const Place place) const
{
	auto it = m_placeMap.find(place);

	if (it == m_placeMap.end())
	{
		return nullptr;
	}

	auto& name = it->second.second;
	return &name;
}

Place CNavMesh::GetPlaceFromName(const std::string& name) const
{
	auto it = std::find_if(m_placeMap.begin(), m_placeMap.end(), [&name](const std::pair<const Place, std::pair<std::string, std::string>>& object) {
		if (object.second.first == name)
		{
			return true;
		}

		return false;
	});

	if (it != m_placeMap.end())
	{
		return it->first;
	}

	return UNDEFINED_PLACE;
}

//--------------------------------------------------------------------------------------------------------------
Place CNavMesh::NameToPlace(const char* name) const
{
	for (auto& obj : m_placeMap)
	{
		const auto& str = obj.second.first; // first is the key name

		if (FStrEq(str.c_str(), name))
		{
			return obj.first;
		}
	}

	return UNDEFINED_PLACE;
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Given the first part of a place name, return a place ID or zero if no place is defined, or the partial match is ambiguous
 */
Place CNavMesh::PartialNameToPlace( const char *name ) const
{
	Place found = UNDEFINED_PLACE;
	bool isAmbiguous = false;

	for (auto& obj : m_placeMap)
	{
		if (!strncasecmp(obj.second.first.c_str(), name, std::strlen(name)))
		{
			// check for exact match in case of subsets of other strings
			if (!strcasecmp(obj.second.first.c_str(), name))
			{
				found = NameToPlace(obj.second.first.c_str());
				isAmbiguous = false;
				break;
			}

			if (found != UNDEFINED_PLACE)
			{
				isAmbiguous = true;
			}
			else
			{
				found = NameToPlace(obj.second.first.c_str());
			}
		}
	}

	if (isAmbiguous)
		return UNDEFINED_PLACE;

	return found;
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Given a partial place name, fill in possible place names for ConCommand autocomplete
 */
int CNavMesh::PlaceNameAutocomplete( char const *partial, char commands[ COMMAND_COMPLETION_MAXITEMS ][ COMMAND_COMPLETION_ITEM_LENGTH ] )
{
	/*
	* Broken, doesn't work. Crashes the game when auto complete is called.
	int numMatches = 0;
	partial += Q_strlen( "sm_nav_use_place " );
	int partialLength = Q_strlen( partial );

	for (const auto& object : m_placeMap)
	{
		if (strncasecmp(object.second.c_str(), partial, partialLength) == 0)
		{
			// Add the place name to the autocomplete array
			ke::SafeSprintf(commands[numMatches], COMMAND_COMPLETION_ITEM_LENGTH, "sm_nav_use_place %s", object.second.c_str());
			++numMatches;

			// Make sure we don't try to return too many place names
			if (numMatches == COMMAND_COMPLETION_MAXITEMS)
				return numMatches;
		}
	}

	return numMatches;
	*/

	return 0;
}

std::optional<std::string> CNavMesh::GetPlaceDisplayName(Place place) const
{
	auto it = m_placeMap.find(place);

	if (it == m_placeMap.end())
	{
		return std::nullopt;
	}

	return it->second.second;
}

//--------------------------------------------------------------------------------------------------------------
typedef const char * SortStringType;
int StringSort (const SortStringType *s1, const SortStringType *s2)
{
	return strcmp( *s1, *s2 );
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Output a list of names to the console
 */
void CNavMesh::PrintAllPlaces( void ) const
{
	if (m_placeMap.empty())
	{
		Msg( "There are no entries in the Place database.\n" );
		return;
	}

	Msg("Nav place name list: \n");

	int i = 0;

	for (const auto& object : m_placeMap)
	{
		Msg(" %s ", object.second.first.c_str());
		
		if (++i > 4)
		{
			Msg("\n");
			i = 0;
		}
	}

	Msg("\n\n");
}

class CTraceFilterGroundEntities : public CTraceFilterWalkableEntities
{
	typedef CTraceFilterWalkableEntities BaseClass;

public:
	CTraceFilterGroundEntities(CBaseEntity* passentity, int collisionGroup, unsigned int flags)
		: BaseClass( passentity, collisionGroup, flags )
	{
	}

	bool ShouldHitEntity(int entity, CBaseEntity* pEntity, edict_t* pEdict, const int contentsMask) override
	{
		if (pEntity != nullptr)
		{
			if (UtilHelpers::FClassnameIs(pEntity, "prop_door") || 
				UtilHelpers::FClassnameIs(pEntity, "prop_door_rotating") || 
				UtilHelpers::FClassnameIs(pEntity, "func_breakable"))
			{
				return false;
			}
		}

		return BaseClass::ShouldHitEntity(entity, pEntity, pEdict, contentsMask);
	}
};

bool CNavMesh::GetGroundHeight( const Vector &pos, float *height, Vector *normal )
{
	const float flMaxOffset = 100.0f;

	CTraceFilterGroundEntities filter(nullptr, COLLISION_GROUP_NONE, WALK_THRU_EVERYTHING);

	trace_t result;
	Vector to( pos.x, pos.y, pos.z - 10000.0f );
	Vector from( pos.x, pos.y, pos.z + navgenparams->human_height + 1e-3 );

	while( to.z - pos.z < flMaxOffset ) 
	{
		trace::line(from, to, MASK_NPCSOLID_BRUSHONLY, &filter, result);

		if (!result.startsolid && ((result.fraction == 1.0f) || ((from.z - result.endpos.z) >= navgenparams->human_height)))
		{
			*height = result.endpos.z;

			if (normal)
			{
				*normal = !result.plane.normal.IsZero() ? result.plane.normal : Vector(0, 0, 1);
			}

			return true;
		}

		to.z = ( result.startsolid ) ? from.z : result.endpos.z;
		from.z = to.z + navgenparams->human_height + 1e-3;
	}

	*height = 0.0f;

	if ( normal )
	{
		normal->Init( 0.0f, 0.0f, 1.0f );
	}

	return false;
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Return the "simple" ground height below this point in "height".
 * This function is much faster, but less tolerant. Make sure the give position is "well behaved".
 * Return false if position is invalid (outside of map, in a solid area, etc).
 */
bool CNavMesh::GetSimpleGroundHeight( const Vector &pos, float *height, Vector *normal ) const
{
	Vector to;
	to.x = pos.x;
	to.y = pos.y;
	to.z = pos.z - 9999.9f;

	trace_t result;

	trace::line(pos, to, MASK_NPCSOLID_BRUSHONLY, nullptr, COLLISION_GROUP_NONE, result);

	if (result.startsolid)
		return false;

	*height = result.endpos.z;

	if (normal)
		*normal = result.plane.normal;

	return true;
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Show danger levels for debugging
 */
void CNavMesh::DrawDanger( void ) const
{
	FOR_EACH_VEC( TheNavAreas, it )
	{
		CNavArea *area = TheNavAreas[ it ];

		Vector center = area->GetCenter();
		Vector top;
		center.z = area->GetZ( center );

		float danger = area->GetDanger( 0 );
		if (danger > 0.1f)
		{
			top.x = center.x;
			top.y = center.y;
			top.z = center.z + 10.0f * danger;
			DrawLine( center, top, 3, 255, 0, 0 );
		}

		danger = area->GetDanger( 1 );
		if (danger > 0.1f)
		{
			top.x = center.x;
			top.y = center.y;
			top.z = center.z + 10.0f * danger;
			DrawLine( center, top, 3, 0, 0, 255 );
		}
	}
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Show current player counts for debugging.
 * NOTE: Assumes two teams.
 */
void CNavMesh::DrawPlayerCounts( void ) const
{
	/*
	CFmtStr msg;

	FOR_EACH_VEC( TheNavAreas, it )
	{
		CNavArea *area = TheNavAreas[ it ];

		if (area->GetPlayerCount() > 0)
		{
			Text(area->GetCenter(),
					msg.sprintf("%d (%d/%d)", area->GetPlayerCount(),
							area->GetPlayerCount(1), area->GetPlayerCount(2)),
					false, NDEBUG_PERSIST_FOR_ONE_TICK);
		}
	}
	*/
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Draw bot avoidance areas from func_nav_avoid entities
 */
void CNavMesh::DrawFuncNavAvoid( void ) const
{
	FOR_EACH_VEC( TheNavAreas, it )
	{
		CNavArea *area = TheNavAreas[ it ];

		if ( area->HasFuncNavAvoid() )
		{
			area->DrawFilled( 255, 0, 0, 255 );
		}
	}
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Draw bot preference areas from func_nav_prefer entities
 */
void CNavMesh::DrawFuncNavPrefer( void ) const
{
	FOR_EACH_VEC( TheNavAreas, it )
	{
		CNavArea *area = TheNavAreas[ it ];

		if ( area->HasFuncNavPrefer() )
		{
			area->DrawFilled( 0, 255, 0, 255 );
		}
	}
}


#ifdef NEXT_BOT
//--------------------------------------------------------------------------------------------------------------
/**
 * Draw bot preference areas from func_nav_prerequisite entities
 */
void CNavMesh::DrawFuncNavPrerequisite( void ) const
{
	FOR_EACH_VEC( TheNavAreas, it )
	{
		CNavArea *area = TheNavAreas[ it ];

		if ( area->HasPrerequisite() )
		{
			area->DrawFilled( 0, 0, 255, 255 );
		}
	}
}
#endif


//--------------------------------------------------------------------------------------------------------------
/**
 * Increase the danger of nav areas containing and near the given position
 */
void CNavMesh::IncreaseDangerNearby( int teamID, float amount, CNavArea *startArea, const Vector &pos, float maxRadius, float dangerLimit )
{
	if (startArea == NULL)
		return;

	CNavArea::MakeNewMarker();
	CNavArea::ClearSearchLists();

	startArea->AddToOpenList();
	startArea->SetTotalCost( 0.0f );
	startArea->Mark();

	float finalDanger = amount;

	if ( dangerLimit > 0.0f && startArea->GetDanger( teamID ) + finalDanger > dangerLimit )
	{
		// clamp danger to given limit
		finalDanger = dangerLimit - startArea->GetDanger( teamID );
	}

	startArea->IncreaseDanger( teamID, finalDanger );

	
	while( !CNavArea::IsOpenListEmpty() )
	{
		// get next area to check
		CNavArea *area = CNavArea::PopOpenList();
		
		// explore adjacent areas
		for( int dir=0; dir<NUM_DIRECTIONS; ++dir )
		{
			int count = area->GetAdjacentCount( (NavDirType)dir );
			for( int i=0; i<count; ++i )
			{
				CNavArea *adjArea = area->GetAdjacentArea( (NavDirType)dir, i );

				if (!adjArea->IsMarked())
				{
					// compute distance from danger source
					float cost = (adjArea->GetCenter() - pos).Length();
					if (cost <= maxRadius)
					{
						adjArea->AddToOpenList();
						adjArea->SetTotalCost( cost );
						adjArea->Mark();

						finalDanger = amount * cost/maxRadius;

						if ( dangerLimit > 0.0f && adjArea->GetDanger( teamID ) + finalDanger > dangerLimit )
						{
							// clamp danger to given limit
							finalDanger = dangerLimit - adjArea->GetDanger( teamID );
						}

						adjArea->IncreaseDanger( teamID, finalDanger );
					}
				}
			}
		}
	}
}


//--------------------------------------------------------------------------------------------------------------
void CommandNavRemoveJumpAreas( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavRemoveJumpAreas();
}
static ConCommand sm_nav_remove_jump_areas( "sm_nav_remove_jump_areas", CommandNavRemoveJumpAreas, "Removes legacy jump areas, replacing them with connections.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavDelete( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() || !sm_nav_edit.GetBool() )
		return;

	TheNavMesh->CommandNavDelete();
}
static ConCommand sm_nav_delete( "sm_nav_delete", CommandNavDelete, "Deletes the currently highlighted Area.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//-------------------------------------------------------------------------------------------------------------- 
void CommandNavDeleteMarked( void ) 
{ 
	if ( !UTIL_IsCommandIssuedByServerAdmin() || !sm_nav_edit.GetBool() )
		return; 

	TheNavMesh->CommandNavDeleteMarked(); 
} 
static ConCommand sm_nav_delete_marked( "sm_nav_delete_marked", CommandNavDeleteMarked, "Deletes the currently marked Area (if any).", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
CON_COMMAND_F(sm_nav_flood_select, "Selects the current Area and all Areas connected to it, recursively. To clear a selection, use this command again.", FCVAR_GAMEDLL | FCVAR_CHEAT )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavFloodSelect( args );
}


//--------------------------------------------------------------------------------------------------------------
void CommandNavToggleSelectedSet( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavToggleSelectedSet();
}
static ConCommand sm_nav_toggle_selected_set( "sm_nav_toggle_selected_set", CommandNavToggleSelectedSet, "Toggles all areas into/out of the selected set.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavStoreSelectedSet( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavStoreSelectedSet();
}
static ConCommand sm_nav_store_selected_set( "sm_nav_store_selected_set", CommandNavStoreSelectedSet, "Stores the current selected set for later retrieval.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavRecallSelectedSet( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavRecallSelectedSet();
}
static ConCommand sm_nav_recall_selected_set( "sm_nav_recall_selected_set", CommandNavRecallSelectedSet, "Re-selects the stored selected set.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavAddToSelectedSet( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavAddToSelectedSet();
}
static ConCommand sm_nav_add_to_selected_set( "sm_nav_add_to_selected_set", CommandNavAddToSelectedSet, "Add current area to the selected set.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
CON_COMMAND_F(sm_nav_add_to_selected_set_by_id, "Add specified area id to the selected set.", FCVAR_GAMEDLL | FCVAR_CHEAT )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavAddToSelectedSetByID( args );
}


//--------------------------------------------------------------------------------------------------------------
void CommandNavRemoveFromSelectedSet( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavRemoveFromSelectedSet();
}
static ConCommand sm_nav_remove_from_selected_set( "sm_nav_remove_from_selected_set", CommandNavRemoveFromSelectedSet, "Remove current area from the selected set.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavToggleInSelectedSet( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavToggleInSelectedSet();
}
static ConCommand sm_nav_toggle_in_selected_set( "sm_nav_toggle_in_selected_set", CommandNavToggleInSelectedSet, "Remove current area from the selected set.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavClearSelectedSet( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavClearSelectedSet();
}
static ConCommand sm_nav_clear_selected_set( "sm_nav_clear_selected_set", CommandNavClearSelectedSet, "Clear the selected set.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//----------------------------------------------------------------------------------
CON_COMMAND_F(sm_nav_dump_selected_set_positions, "Write the (x,y,z) coordinates of the centers of all selected nav areas to a file.", FCVAR_GAMEDLL | FCVAR_CHEAT )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	const NavAreaVector &selectedSet = TheNavMesh->GetSelectedSet();

	CUtlBuffer fileBuffer( 4096, 1024*1024, CUtlBuffer::TEXT_BUFFER );

	for( int i=0; i<selectedSet.Count(); ++i )
	{
		const Vector &center = selectedSet[i]->GetCenter();

		fileBuffer.Printf( "%f %f %f\n", center.x, center.y, center.z );
	}

	// filename is local to game dir for Steam, so we need to prepend game dir for regular file save
	char gamePath[256];
	engine->GetGameDir( gamePath, 256 );

	char filename[256];
	Q_snprintf(filename, sizeof(filename), "%s\\maps\\%s_xyz.txt", gamePath,
			STRING(gpGlobals->mapname));

	if ( !filesystem->WriteFile( filename, "MOD", fileBuffer ) )
	{
		Warning( "Unable to save %d bytes to %s\n", fileBuffer.Size(), filename );
	}
	else
	{
		DevMsg( "Write %d nav area center positions to '%s'.\n", selectedSet.Count(), filename );
	}
};


//----------------------------------------------------------------------------------
CON_COMMAND_F(sm_nav_show_dumped_positions, "Show the (x,y,z) coordinate positions of the given dump file.", FCVAR_GAMEDLL | FCVAR_CHEAT )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	CUtlBuffer fileBuffer( 4096, 1024*1024, CUtlBuffer::TEXT_BUFFER );

	// filename is local to game dir for Steam, so we need to prepend game dir for regular file save
	char gamePath[256];
	engine->GetGameDir( gamePath, 256 );

	char filename[256];
	Q_snprintf(filename, sizeof(filename), "%s\\maps\\%s_xyz.txt", gamePath,
			STRING(gpGlobals->mapname));

	if ( !filesystem->ReadFile( filename, "MOD", fileBuffer ) )
	{
		Warning( "Unable to read %s\n", filename );
	}
	else
	{
		while( true )
		{
			Vector center;
			if ( fileBuffer.Scanf( "%f %f %f", &center.x, &center.y, &center.z ) <= 0 )
			{
				break;
			}


			Cross3D( center, 5.0f, 255, 255, 0, true, 99999.9f );
		}
	}
};


//----------------------------------------------------------------------------------
CON_COMMAND_F(sm_nav_select_larger_than, "Select nav areas where both dimensions are larger than the given size.", FCVAR_GAMEDLL | FCVAR_CHEAT )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	if ( args.ArgC() > 1 )
	{
		float minSize = atof( args[1] );

		int selectedCount = 0;

		for( int i=0; i<TheNavAreas.Count(); ++i )
		{
			CNavArea *area = TheNavAreas[i];

			if ( area->GetSizeX() > minSize && area->GetSizeY() > minSize )
			{
				TheNavMesh->AddToSelectedSet( area );
				++selectedCount;
			}
		}

		DevMsg( "Selected %d areas with dimensions larger than %3.2f units.\n", selectedCount, minSize );
	}
}


//--------------------------------------------------------------------------------------------------------------
void CommandNavBeginSelecting( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavBeginSelecting();
}
static ConCommand sm_nav_begin_selecting( "sm_nav_begin_selecting", CommandNavBeginSelecting, "Start continuously adding to the selected set.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavEndSelecting( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavEndSelecting();
}
static ConCommand sm_nav_end_selecting( "sm_nav_end_selecting", CommandNavEndSelecting, "Stop continuously adding to the selected set.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavBeginDragSelecting( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavBeginDragSelecting();
}
static ConCommand sm_nav_begin_drag_selecting( "sm_nav_begin_drag_selecting", CommandNavBeginDragSelecting, "Start dragging a selection area.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavEndDragSelecting( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavEndDragSelecting();
}
static ConCommand sm_nav_end_drag_selecting( "sm_nav_end_drag_selecting", CommandNavEndDragSelecting, "Stop dragging a selection area.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavBeginDragDeselecting( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavBeginDragDeselecting();
}
static ConCommand sm_nav_begin_drag_deselecting( "sm_nav_begin_drag_deselecting", CommandNavBeginDragDeselecting, "Start dragging a selection area.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavEndDragDeselecting( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavEndDragDeselecting();
}
static ConCommand sm_nav_end_drag_deselecting( "sm_nav_end_drag_deselecting", CommandNavEndDragDeselecting, "Stop dragging a selection area.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavRaiseDragVolumeMax( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavRaiseDragVolumeMax();
}
static ConCommand sm_nav_raise_drag_volume_max( "sm_nav_raise_drag_volume_max", CommandNavRaiseDragVolumeMax, "Raise the top of the drag select volume.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavLowerDragVolumeMax( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavLowerDragVolumeMax();
}
static ConCommand sm_nav_lower_drag_volume_max( "sm_nav_lower_drag_volume_max", CommandNavLowerDragVolumeMax, "Lower the top of the drag select volume.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavRaiseDragVolumeMin( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavRaiseDragVolumeMin();
}
static ConCommand sm_nav_raise_drag_volume_min( "sm_nav_raise_drag_volume_min", CommandNavRaiseDragVolumeMin, "Raise the bottom of the drag select volume.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavLowerDragVolumeMin( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavLowerDragVolumeMin();
}
static ConCommand sm_nav_lower_drag_volume_min( "sm_nav_lower_drag_volume_min", CommandNavLowerDragVolumeMin, "Lower the bottom of the drag select volume.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavToggleSelecting( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavToggleSelecting();
}
static ConCommand sm_nav_toggle_selecting( "sm_nav_toggle_selecting", CommandNavToggleSelecting, "Start or stop continuously adding to the selected set.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavBeginDeselecting( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavBeginDeselecting();
}
static ConCommand sm_nav_begin_deselecting( "sm_nav_begin_deselecting", CommandNavBeginDeselecting, "Start continuously removing from the selected set.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavEndDeselecting( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavEndDeselecting();
}
static ConCommand sm_nav_end_deselecting( "sm_nav_end_deselecting", CommandNavEndDeselecting, "Stop continuously removing from the selected set.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavToggleDeselecting( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavToggleDeselecting();
}
static ConCommand sm_nav_toggle_deselecting( "sm_nav_toggle_deselecting", CommandNavToggleDeselecting, "Start or stop continuously removing from the selected set.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
CON_COMMAND_F(sm_nav_select_half_space, "Selects any areas that intersect the given half-space.", FCVAR_GAMEDLL | FCVAR_CHEAT )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavSelectHalfSpace( args );
}


//--------------------------------------------------------------------------------------------------------------
void CommandNavBeginShiftXY( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavBeginShiftXY();
}
static ConCommand sm_nav_begin_shift_xy( "sm_nav_begin_shift_xy", CommandNavBeginShiftXY, "Begin shifting the Selected Set.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavEndShiftXY( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavEndShiftXY();
}
static ConCommand sm_nav_end_shift_xy( "sm_nav_end_shift_xy", CommandNavEndShiftXY, "Finish shifting the Selected Set.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavSelectInvalidAreas( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavSelectInvalidAreas();
}
static ConCommand sm_nav_select_invalid_areas( "sm_nav_select_invalid_areas", CommandNavSelectInvalidAreas, "Adds all invalid areas to the Selected Set.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
CON_COMMAND_F(sm_nav_select_blocked_areas, "Adds all blocked areas to the selected set", FCVAR_CHEAT )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavSelectBlockedAreas();
}


//--------------------------------------------------------------------------------------------------------------
CON_COMMAND_F(sm_nav_select_obstructed_areas, "Adds all obstructed areas to the selected set", FCVAR_CHEAT )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavSelectObstructedAreas();
}


//--------------------------------------------------------------------------------------------------------------
CON_COMMAND_F(sm_nav_select_damaging_areas, "Adds all damaging areas to the selected set", FCVAR_CHEAT )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavSelectDamagingAreas();
}


//--------------------------------------------------------------------------------------------------------------
CON_COMMAND_F(sm_nav_select_stairs, "Adds all stairway areas to the selected set", FCVAR_CHEAT )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavSelectStairs();
}


//--------------------------------------------------------------------------------------------------------------
CON_COMMAND_F(sm_nav_select_orphans, "Adds all orphan areas to the selected set (highlight a valid area first).", FCVAR_CHEAT )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavSelectOrphans();
}


//--------------------------------------------------------------------------------------------------------------
void CommandNavSplit( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavSplit();
}
static ConCommand sm_nav_split( "sm_nav_split", CommandNavSplit, "To split an Area into two, align the split line using your cursor and invoke the split command.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavMakeSniperSpots( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavMakeSniperSpots();
}
static ConCommand sm_nav_make_sniper_spots( "sm_nav_make_sniper_spots", CommandNavMakeSniperSpots, "Chops the marked area into disconnected sub-areas suitable for sniper spots.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavMerge( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavMerge();
}
static ConCommand sm_nav_merge( "sm_nav_merge", CommandNavMerge, "To merge two Areas into one, mark the first Area, highlight the second by pointing your cursor at it, and invoke the merge command.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavMark( const CCommand &args )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavMark( args );
}
static ConCommand sm_nav_mark( "sm_nav_mark", CommandNavMark, "Marks the Area or Ladder under the cursor for manipulation by subsequent editing commands.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavUnmark( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavUnmark();
}
static ConCommand sm_nav_unmark( "sm_nav_unmark", CommandNavUnmark, "Clears the marked Area or Ladder.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavBeginArea( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavBeginArea();
}
static ConCommand sm_nav_begin_area( "sm_nav_begin_area", CommandNavBeginArea, "Defines a corner of a new Area or Ladder. To complete the Area or Ladder, drag the opposite corner to the desired location and issue a 'nav_end_area' command.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavEndArea( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavEndArea();
}
static ConCommand sm_nav_end_area( "sm_nav_end_area", CommandNavEndArea, "Defines the second corner of a new Area or Ladder and creates it.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavConnect( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavConnect();
}
static ConCommand sm_nav_connect( "sm_nav_connect", CommandNavConnect, "To connect two Areas, mark the first Area, highlight the second Area, then invoke the connect command. Note that this creates a ONE-WAY connection from the first to the second Area. To make a two-way connection, also connect the second area to the first.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavDisconnect( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavDisconnect();
}
static ConCommand sm_nav_disconnect( "sm_nav_disconnect", CommandNavDisconnect, "To disconnect two Areas, mark an Area, highlight a second Area, then invoke the disconnect command. This will remove all connections between the two Areas.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavDisconnectOutgoingOneWays( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavDisconnectOutgoingOneWays();
}
static ConCommand sm_nav_disconnect_outgoing_oneways( "sm_nav_disconnect_outgoing_oneways", CommandNavDisconnectOutgoingOneWays, "For each area in the selected set, disconnect all outgoing one-way connections.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavSplice( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavSplice();
}
static ConCommand sm_nav_splice( "sm_nav_splice", CommandNavSplice, "To splice, mark an area, highlight a second area, then invoke the splice command to create a new, connected area between them.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavCrouch( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavToggleAttribute( NAV_MESH_CROUCH );
}
static ConCommand sm_nav_crouch( "sm_nav_crouch", CommandNavCrouch, "Toggles the 'must crouch in this area' flag used by the AI system.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavPrecise( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavToggleAttribute( NAV_MESH_PRECISE );
}
static ConCommand sm_nav_precise( "sm_nav_precise", CommandNavPrecise, "Toggles the 'dont avoid obstacles' flag used by the AI system.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavJump( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavToggleAttribute( NAV_MESH_JUMP );
}
static ConCommand sm_nav_jump( "sm_nav_jump", CommandNavJump, "Toggles the 'traverse this area by jumping' flag used by the AI system.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavNoJump( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavToggleAttribute( NAV_MESH_NO_JUMP );
}
static ConCommand sm_nav_no_jump( "sm_nav_no_jump", CommandNavNoJump, "Toggles the 'dont jump in this area' flag used by the AI system.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavStop( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavToggleAttribute( NAV_MESH_STOP );
}
static ConCommand sm_nav_stop( "sm_nav_stop", CommandNavStop, "Toggles the 'must stop when entering this area' flag used by the AI system.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavWalk( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavToggleAttribute( NAV_MESH_WALK );
}
static ConCommand sm_nav_walk( "sm_nav_walk", CommandNavWalk, "Toggles the 'traverse this area by walking' flag used by the AI system.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavRun( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavToggleAttribute( NAV_MESH_RUN );
}
static ConCommand sm_nav_run( "sm_nav_run", CommandNavRun, "Toggles the 'traverse this area by running' flag used by the AI system.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavAvoid( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavToggleAttribute( NAV_MESH_AVOID );
}
static ConCommand sm_nav_avoid( "sm_nav_avoid", CommandNavAvoid, "Toggles the 'avoid this area when possible' flag used by the AI system.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavTransient( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavToggleAttribute( NAV_MESH_TRANSIENT );
}
static ConCommand sm_nav_transient( "sm_nav_transient", CommandNavTransient, "Toggles the 'area is transient and may become blocked' flag used by the AI system.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavDontHide( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavToggleAttribute( NAV_MESH_DONT_HIDE );
}
static ConCommand sm_nav_dont_hide( "sm_nav_dont_hide", CommandNavDontHide, "Toggles the 'area is not suitable for hiding spots' flag used by the AI system.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavStand( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavToggleAttribute( NAV_MESH_STAND );
}
static ConCommand sm_nav_stand( "sm_nav_stand", CommandNavStand, "Toggles the 'stand while hiding' flag used by the AI system.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavNoHostages( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavToggleAttribute( NAV_MESH_NO_HOSTAGES );
}
static ConCommand sm_nav_no_hostages( "sm_nav_no_hostages", CommandNavNoHostages, "Toggles the 'hostages cannot use this area' flag used by the AI system.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavStrip( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->StripNavigationAreas();
}
static ConCommand sm_nav_strip( "sm_nav_strip", CommandNavStrip, "Strips all Hiding Spots, Approach Points, and Encounter Spots from the current Area.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavSave( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	if (TheNavMesh->Save())
	{
		Msg( "Navigation map '%s' saved.\n", TheNavMesh->GetFilename() );
	}
	else
	{
		const char *filename = TheNavMesh->GetFilename();
		Msg( "ERROR: Cannot save navigation map '%s'.\n", (filename) ? filename : "(null)" );
	}
}
static ConCommand sm_nav_save( "sm_nav_save", CommandNavSave, "Saves the current Navigation Mesh to disk.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavLoad( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	if (TheNavMesh->Load() != NAV_OK)
	{
		Msg( "ERROR: Navigation Mesh load failed.\n" );
	}
}
static ConCommand sm_nav_load( "sm_nav_load", CommandNavLoad, "Loads the Navigation Mesh for the current map.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
static int PlaceNameAutocompleteCallback( char const *partial, char commands[ COMMAND_COMPLETION_MAXITEMS ][ COMMAND_COMPLETION_ITEM_LENGTH ] )
{
	return 0; // this is causing crashes
	// return TheNavMesh->PlaceNameAutocomplete( partial, commands );
}


//--------------------------------------------------------------------------------------------------------------
void CommandNavUsePlace( const CCommand &args )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	if (args.ArgC() == 1)
	{
		// no arguments = list all available places
		TheNavMesh->PrintAllPlaces();
	}
	else
	{
		// single argument = set current place
		Place place = TheNavMesh->PartialNameToPlace( args[ 1 ] );

		if (place == UNDEFINED_PLACE)
		{
			Msg( "Ambiguous\n" );
		}
		else
		{
			Msg("Current place set to '%s'\n", TheNavMesh->GetPlaceName(place)->c_str());
			TheNavMesh->SetNavPlace( place );
		}
	}
}
static ConCommand sm_nav_use_place( "sm_nav_use_place", CommandNavUsePlace, "If used without arguments, all available Places will be listed. If a Place argument is given, the current Place is set.", FCVAR_GAMEDLL | FCVAR_CHEAT, PlaceNameAutocompleteCallback );


//--------------------------------------------------------------------------------------------------------------
void CommandNavPlaceReplace( const CCommand &args )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	if (args.ArgC() != 3)
	{
		// no arguments
		Msg( "Usage: nav_place_replace <OldPlace> <NewPlace>\n" );
	}
	else
	{
		// two arguments - replace the first place with the second
		Place oldPlace = TheNavMesh->PartialNameToPlace( args[ 1 ] );
		Place newPlace = TheNavMesh->PartialNameToPlace( args[ 2 ] );

		if ( oldPlace == UNDEFINED_PLACE || newPlace == UNDEFINED_PLACE )
		{
			Msg( "Ambiguous\n" );
		}
		else
		{
			FOR_EACH_VEC( TheNavAreas, it )
			{
				CNavArea *area = TheNavAreas[it];
				if ( area->GetPlace() == oldPlace )
				{
					area->SetPlace( newPlace );
				}
			}
		}
	}
}
static ConCommand sm_nav_place_replace( "sm_nav_place_replace", CommandNavPlaceReplace, "Replaces all instances of the first place with the second place.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavPlaceList( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	std::unordered_set<Place> allplaces;
	allplaces.reserve(512);

	FOR_EACH_VEC( TheNavAreas, nit )
	{
		Place place = TheNavAreas[ nit ]->GetPlace();

		if (place > 0)
		{
			allplaces.emplace(place);
		}
	}

	Msg("Map uses %d place names:\n", static_cast<int>(allplaces.size()));

	for (auto& place : allplaces)
	{
		Msg("    %s\n", TheNavMesh->GetPlaceName(place)->c_str());
	}
}
static ConCommand sm_nav_place_list( "sm_nav_place_list", CommandNavPlaceList, "Lists all place names used in the map.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavTogglePlaceMode( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavTogglePlaceMode();
}
static ConCommand sm_nav_toggle_place_mode( "sm_nav_toggle_place_mode", CommandNavTogglePlaceMode, "Toggle the editor into and out of Place mode. Place mode allows labelling of Area with Place names.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavSetPlaceMode( const CCommand &args )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	bool on = true;
	if ( args.ArgC() == 2 )
	{
		on = (atoi( args[ 1 ] ) != 0);
	}

	if ( on != TheNavMesh->IsEditMode( CNavMesh::PLACE_PAINTING ) )
	{
		TheNavMesh->CommandNavTogglePlaceMode();
	}
}
static ConCommand sm_nav_set_place_mode( "sm_nav_set_place_mode", CommandNavSetPlaceMode, "Sets the editor into or out of Place mode. Place mode allows labelling of Area with Place names.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavPlaceFloodFill( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavPlaceFloodFill();
}
static ConCommand sm_nav_place_floodfill( "sm_nav_place_floodfill", CommandNavPlaceFloodFill, "Sets the Place of the Area under the cursor to the curent Place, and 'flood-fills' the Place to all adjacent Areas. Flood-filling stops when it hits an Area with the same Place, or a different Place than that of the initial Area.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavPlaceSet( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavPlaceSet();
}
static ConCommand sm_nav_place_set( "sm_nav_place_set", CommandNavPlaceSet, "Sets the Place of all selected areas to the current Place.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavPlacePick( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavPlacePick();
}
static ConCommand sm_nav_place_pick( "sm_nav_place_pick", CommandNavPlacePick, "Sets the current Place to the Place of the Area under the cursor.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavTogglePlacePainting( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavTogglePlacePainting();
}
static ConCommand sm_nav_toggle_place_painting( "sm_nav_toggle_place_painting", CommandNavTogglePlacePainting, "Toggles Place Painting mode. When Place Painting, pointing at an Area will 'paint' it with the current Place.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavMarkUnnamed( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavMarkUnnamed();
}
static ConCommand sm_nav_mark_unnamed( "sm_nav_mark_unnamed", CommandNavMarkUnnamed, "Mark an Area with no Place name. Useful for finding stray areas missed when Place Painting.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavCornerSelect( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavCornerSelect();
}
static ConCommand sm_nav_corner_select( "sm_nav_corner_select", CommandNavCornerSelect, "Select a corner of the currently marked Area. Use multiple times to access all four corners.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
CON_COMMAND_F(sm_nav_corner_raise, "Raise the selected corner of the currently marked Area.", FCVAR_GAMEDLL | FCVAR_CHEAT )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavCornerRaise( args );
}


//--------------------------------------------------------------------------------------------------------------
CON_COMMAND_F(sm_nav_corner_lower, "Lower the selected corner of the currently marked Area.", FCVAR_GAMEDLL | FCVAR_CHEAT )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavCornerLower( args );
}


//--------------------------------------------------------------------------------------------------------------
CON_COMMAND_F(sm_nav_corner_place_on_ground, "Places the selected corner of the currently marked Area on the ground.", FCVAR_GAMEDLL | FCVAR_CHEAT )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavCornerPlaceOnGround( args );
}


//--------------------------------------------------------------------------------------------------------------
void CommandNavWarpToMark( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavWarpToMark();
}
static ConCommand sm_nav_warp_to_mark( "sm_nav_warp_to_mark", CommandNavWarpToMark, "Warps the player to the marked area.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavLadderFlip( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavLadderFlip();
}
static ConCommand sm_nav_ladder_flip( "sm_nav_ladder_flip", CommandNavLadderFlip, "Flips the selected ladder's direction.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavGenerate( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->BeginGeneration();
}
static ConCommand sm_nav_generate( "sm_nav_generate", CommandNavGenerate, "Generate a Navigation Mesh for the current map and save it to disk.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavGenerateIncremental( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->BeginGeneration( INCREMENTAL_GENERATION );
}
static ConCommand sm_nav_generate_incremental( "sm_nav_generate_incremental", CommandNavGenerateIncremental, "Generate a Navigation Mesh for the current map and save it to disk.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavAnalyze( void )
{
	if ( UTIL_IsCommandIssuedByServerAdmin() && sm_nav_edit.GetBool() )
	{
		TheNavMesh->BeginAnalysis();
	}
}
static ConCommand sm_nav_analyze( "sm_nav_analyze", CommandNavAnalyze, "Re-analyze the current Navigation Mesh and save it to disk.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavAnalyzeScripted( const CCommand &args )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	const char *pszCmd = NULL;
	int count = args.ArgC();
	if ( count > 0 )
	{
		pszCmd = args[1];
	}

	bool bForceAnalyze = pszCmd && !Q_stricmp( pszCmd, "force" );

	if ( TheNavMesh->IsAnalyzed() && !bForceAnalyze )
	{
		engine->ServerCommand( "quit\n" );
		return;
	}

	if (sm_nav_edit.GetBool())
	{
		TheNavMesh->BeginAnalysis( true );
	}
}
static ConCommand sm_nav_analyze_scripted( "sm_nav_analyze_scripted", CommandNavAnalyzeScripted, "commandline hook to run a nav_analyze and then quit.", FCVAR_GAMEDLL | FCVAR_CHEAT | FCVAR_HIDDEN );


//--------------------------------------------------------------------------------------------------------------
void CommandNavMarkWalkable( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavMarkWalkable();
}


//--------------------------------------------------------------------------------------------------------
void CNavMesh::CommandNavMarkWalkable( void )
{
	Vector pos;

	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	if (sm_nav_edit.GetBool())
	{
		// we are in edit mode, use the edit cursor's location
		pos = GetEditCursorPosition();
	}
	else
	{
		// we are not in edit mode, use the position of the local player
		entities::HBaseEntity be(gamehelpers->EdictOfIndex(1));
		pos = be.GetAbsOrigin();
	}

	// snap position to the sampling grid
	pos.x = SnapToGrid( pos.x, true );
	pos.y = SnapToGrid( pos.y, true );

	Vector normal;
	if ( !FindGroundForNode( &pos, &normal ) )
	{
		Msg( "ERROR: Invalid ground position.\n" );
		return;
	}

	AddWalkableSeed( pos, normal );

	Msg( "Walkable position marked.\n" );
}

static ConCommand sm_nav_mark_walkable( "sm_nav_mark_walkable", CommandNavMarkWalkable, "Mark the current location as a walkable position. These positions are used as seed locations when sampling the map to generate a Navigation Mesh.", FCVAR_GAMEDLL | FCVAR_CHEAT );

static void CommandNavSeedWalkableSpots()
{
	TheNavMesh->CommandNavSeedWalkableSpots();
	Msg("Walkable spots added!\n");
}

void CNavMesh::CommandNavSeedWalkableSpots(void)
{
	AddWalkableSeeds();
}

static ConCommand sm_nav_seed_walkable_spots("sm_nav_seed_walkable_spots", CommandNavSeedWalkableSpots, "Automatically adds walkable positions.", FCVAR_GAMEDLL | FCVAR_CHEAT);

//--------------------------------------------------------------------------------------------------------------
void CommandNavClearWalkableMarks( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->ClearWalkableSeeds();
}
static ConCommand sm_nav_clear_walkable_marks( "sm_nav_clear_walkable_marks", CommandNavClearWalkableMarks, "Erase any previously placed walkable positions.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavCompressID( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	CNavArea::CompressIDs(TheNavMesh);
	CNavLadder::CompressIDs(TheNavMesh);
	TheNavMesh->CompressWaypointsIDs();
	TheNavMesh->CompressVolumesIDs();
	TheNavMesh->CompressElevatorsIDs();
	TheNavMesh->CompressPrerequisiteIDs();
}
static ConCommand sm_nav_compress_id( "sm_nav_compress_id", CommandNavCompressID, "Re-orders area and ladder ID's so they are continuous.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
#ifdef TERROR
void CommandNavShowLadderBounds( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	CFuncSimpleLadder *ladder = NULL;
	while( (ladder = dynamic_cast< CFuncSimpleLadder * >(gEntList.FindEntityByClassname( ladder, "func_simpleladder" ))) != NULL )
	{
		Vector mins, maxs;
		ladder->CollisionProp()->WorldSpaceSurroundingBounds( &mins, &maxs );
		ladder->m_debugOverlays |= OVERLAY_TEXT_BIT | OVERLAY_ABSBOX_BIT;
		NDebugOverlay::Box( vec3_origin, mins, maxs, 0, 255, 0, 0, 600 );
	}
}
static ConCommand nav_show_ladder_bounds( "nav_show_ladder_bounds", CommandNavShowLadderBounds, "Draws the bounding boxes of all func_ladders in the map.", FCVAR_GAMEDLL | FCVAR_CHEAT );
#endif

//--------------------------------------------------------------------------------------------------------------
void CommandNavBuildLadder( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavBuildLadder();
}
static ConCommand sm_nav_build_ladder( "sm_nav_build_ladder", CommandNavBuildLadder, "Attempts to build a nav ladder on the climbable surface under the cursor.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------
void NavEditClearAllAttributes( void )
{
	NavAttributeClearer clear( (NavAttributeType)0xFFFF );
	TheNavMesh->ForAllSelectedAreas( clear );
	TheNavMesh->ClearSelectedSet();
}
static ConCommand SMClearAllNavAttributes( "sm_wipe_nav_attributes", NavEditClearAllAttributes, "Clear all nav attributes of selected area.", FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------
bool NavAttributeToggler::operator() ( CNavArea *area )
{
	// only toggle if dealing with a single selected area
	if ( TheNavMesh->IsSelectedSetEmpty() && (area->GetAttributes() & m_attribute) != 0 )
	{
		area->SetAttributes( area->GetAttributes() & (~m_attribute) );
	}
	else
	{
		area->SetAttributes( area->GetAttributes() | m_attribute );
	}

	return true;
}


//--------------------------------------------------------------------------------------------------------
NavAttributeLookup TheNavAttributeTable[] =
{
	{ "CROUCH", NAV_MESH_CROUCH },
	{ "JUMP", NAV_MESH_JUMP },
	{ "PRECISE", NAV_MESH_PRECISE },
	{ "NO_JUMP", NAV_MESH_NO_JUMP },
	{ "STOP", NAV_MESH_STOP },
	{ "RUN", NAV_MESH_RUN },
	{ "WALK", NAV_MESH_WALK },
	{ "AVOID", NAV_MESH_AVOID },
	{ "TRANSIENT", NAV_MESH_TRANSIENT },
	{ "DONT_HIDE", NAV_MESH_DONT_HIDE },
	{ "STAND", NAV_MESH_STAND },
	{ "NO_HOSTAGES", NAV_MESH_NO_HOSTAGES },
	{ "STAIRS", NAV_MESH_STAIRS },
	{ "NO_MERGE", NAV_MESH_NO_MERGE },
	{ "OBSTACLE_TOP", NAV_MESH_OBSTACLE_TOP },
#ifdef TERROR
	{ "PLAYERCLIP", (NavAttributeType)CNavArea::NAV_PLAYERCLIP },
	{ "BREAKABLEWALL", (NavAttributeType)CNavArea::NAV_BREAKABLEWALL },
#endif
	{ NULL, NAV_MESH_INVALID }
};


/**
 * Can be used with any command that takes an attribute as its 2nd argument
 */
static int NavAttributeAutocomplete( const char *input, char commands[ COMMAND_COMPLETION_MAXITEMS ][ COMMAND_COMPLETION_ITEM_LENGTH ] )
{
	return 0; // disable this until the crash is resolved

	//if ( Q_strlen( input ) >= COMMAND_COMPLETION_ITEM_LENGTH )
	//{
	//	return 0;
	//}
	//
	//char command[ COMMAND_COMPLETION_ITEM_LENGTH+1 ];
	//Q_strncpy( command, input, sizeof( command ) );
	//
	//// skip to start of argument
	//char *partialArg = Q_strrchr( command, ' ' );
	//if ( partialArg == NULL )
	//{
	//	return 0;
	//}
	//
	//// chop command from partial argument
	//*partialArg = '\000';
	//++partialArg;
	//
	//int partialArgLength = Q_strlen( partialArg );

	//int count = 0;
	//for( unsigned int i=0; TheNavAttributeTable[i].name && count < COMMAND_COMPLETION_MAXITEMS; ++i )
	//{
	//	if ( !Q_strnicmp( TheNavAttributeTable[i].name, partialArg, partialArgLength ) )
	//	{
	//		// Add to the autocomplete array
	//		Q_snprintf( commands[ count++ ], COMMAND_COMPLETION_ITEM_LENGTH, "%s %s", command, TheNavAttributeTable[i].name );
	//	}
	//}

	//return count;
}


//--------------------------------------------------------------------------------------------------------
NavAttributeType NameToNavAttribute( const char *name )
{
	for( unsigned int i=0; TheNavAttributeTable[i].name; ++i )
	{
		if ( !Q_stricmp( TheNavAttributeTable[i].name, name ) )
		{
			return TheNavAttributeTable[i].attribute;
		}
	}
	
	return (NavAttributeType)0;
}


//--------------------------------------------------------------------------------------------------------
void NavEditClearAttribute( const CCommand &args )
{
	if ( args.ArgC() != 2 )
	{
		Msg( "Usage: %s <attribute>\n", args[0] );
		return;
	}
	
	NavAttributeType attribute = NameToNavAttribute( args[1] );

	if ( attribute != 0 )
	{
		NavAttributeClearer clear( attribute );
		TheNavMesh->ForAllSelectedAreas( clear );
		TheNavMesh->ClearSelectedSet();
		return;
	}
	
	Msg( "Unknown attribute '%s'", args[1] );		
}
static ConCommand SMNavClearAttribute( "sm_nav_clear_attribute", NavEditClearAttribute, "Remove given nav attribute from all areas in the selected set.", FCVAR_CHEAT, NavAttributeAutocomplete );


//--------------------------------------------------------------------------------------------------------
void NavEditMarkAttribute( const CCommand &args )
{
	if ( args.ArgC() != 2 )
	{
		Msg( "Usage: %s <attribute>\n", args[0] );
		return;
	}

	NavAttributeType attribute = NameToNavAttribute( args[1] );

	if ( attribute != 0 )
	{
		NavAttributeSetter setter( attribute );	
		TheNavMesh->ForAllSelectedAreas( setter );
		TheNavMesh->ClearSelectedSet();
		return;
	}

	Msg( "Unknown attribute '%s'", args[1] );		
}
static ConCommand SMNavMarkAttribute( "sm_nav_mark_attribute", NavEditMarkAttribute, "Set nav attribute for all areas in the selected set.", FCVAR_CHEAT, NavAttributeAutocomplete );


/* IN PROGRESS:
//--------------------------------------------------------------------------------------------------------------
void CommandNavPickArea( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavPickArea();
}
static ConCommand nav_pick_area( "nav_pick_area", CommandNavPickArea, "Marks an area (and corner) based on the surface under the cursor.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavResizeHorizontal( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavResizeHorizontal();
}
static ConCommand nav_resize_horizontal( "nav_resize_horizontal", CommandNavResizeHorizontal, "TODO", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavResizeVertical( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavResizeVertical();
}
static ConCommand nav_resize_vertical( "nav_resize_vertical", CommandNavResizeVertical, "TODO", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavResizeEnd( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavResizeEnd();
}
static ConCommand nav_resize_end( "nav_resize_end", CommandNavResizeEnd, "TODO", FCVAR_GAMEDLL | FCVAR_CHEAT );
*/


//--------------------------------------------------------------------------------------------------------------
/**
 * Destroy ladder representations
 */
void CNavMesh::DestroyLadders( void )
{
	for ( int i=0; i<m_ladders.Count(); ++i )
	{
		OnEditDestroyNotify( m_ladders[i] );
		delete m_ladders[i];
	}

	m_ladders.RemoveAll();

	m_markedLadder = NULL;
	m_selectedLadder = NULL;
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Strip the "analyzed" data out of all navigation areas
 */
void CNavMesh::StripNavigationAreas( void )
{
	FOR_EACH_VEC( TheNavAreas, it )
	{
		CNavArea *area = TheNavAreas[ it ];

		area->Strip();
	}

	m_isAnalyzed = false;
}

//--------------------------------------------------------------------------------------------------------------

HidingSpotVector TheHidingSpots;
unsigned int HidingSpot::m_nextID = 1;
unsigned int HidingSpot::m_masterMarker = 0;


//--------------------------------------------------------------------------------------------------------------
/**
 * Hiding Spot factory
 */
HidingSpot *CNavMesh::CreateHidingSpot( void ) const
{
	return new HidingSpot;
}


//--------------------------------------------------------------------------------------------------------------
void CNavMesh::DestroyHidingSpots( void )
{
	// remove all hiding spot references from the nav areas
	FOR_EACH_VEC( TheNavAreas, it )
	{
		CNavArea *area = TheNavAreas[ it ];

		area->m_hidingSpots.RemoveAll();
	}

	HidingSpot::m_nextID = 0;

	// free all the HidingSpots
	FOR_EACH_VEC( TheHidingSpots, hit )
	{
		delete TheHidingSpots[ hit ];
	}

	TheHidingSpots.RemoveAll();
}

//--------------------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------------------
/**
 * Construct a Hiding Spot.  Assign a unique ID which may be overwritten if loaded.
 */
HidingSpot::HidingSpot( void )
{
	m_pos = Vector( 0, 0, 0 );
	m_id = m_nextID++;
	m_flags = 0;
	m_area = NULL;

	TheHidingSpots.AddToTail( this );
}


//--------------------------------------------------------------------------------------------------------------
void HidingSpot::Save(std::fstream& filestream, uint32_t version)
{
	filestream.write(reinterpret_cast<char*>(&m_id), sizeof(m_id));
	filestream.write(reinterpret_cast<char*>(&m_pos), sizeof(Vector));
	filestream.write(reinterpret_cast<char*>(&m_flags), sizeof(m_flags));
}


//--------------------------------------------------------------------------------------------------------------
void HidingSpot::Load(std::fstream& filestream, uint32_t version)
{
	filestream.read(reinterpret_cast<char*>(&m_id), sizeof(m_id));
	filestream.read(reinterpret_cast<char*>(&m_pos), sizeof(Vector));
	filestream.read(reinterpret_cast<char*>(&m_flags), sizeof(m_flags));

	// update next ID to avoid ID collisions by later spots
	if (m_id >= m_nextID)
		m_nextID = m_id+1;
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Hiding Spot post-load processing
 */
NavErrorType HidingSpot::PostLoad( void )
{
	// set our area
	m_area = TheNavMesh->GetNavArea( m_pos + Vector( 0, 0, navgenparams->human_height ) );

	if ( !m_area )
	{
		DevWarning( "A Hiding Spot is off of the Nav Mesh at setpos %.0f %.0f %.0f\n", m_pos.x, m_pos.y, m_pos.z );
	}

	return NAV_OK;
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Given a HidingSpot ID, return the associated HidingSpot
 */
HidingSpot *GetHidingSpotByID( unsigned int id )
{
	FOR_EACH_VEC( TheHidingSpots, it )
	{
		HidingSpot *spot = TheHidingSpots[ it ];

		if (spot->GetID() == id)
			return spot;
	}	

	return NULL;
}


//--------------------------------------------------------------------------------------------------------
// invoked when the area becomes blocked
void CNavMesh::OnAreaBlocked( CNavArea *area )
{
	if ( !m_blockedAreas.HasElement( area ) )
	{
		m_blockedAreas.AddToTail( area );
	}
}


//--------------------------------------------------------------------------------------------------------
// invoked when the area becomes un-blocked
void CNavMesh::OnAreaUnblocked( CNavArea *area )
{
	m_blockedAreas.FindAndRemove( area );
}


//--------------------------------------------------------------------------------------------------------
void CNavMesh::UpdateBlockedAreas( void )
{
	for ( int i=0; i<m_blockedAreas.Count(); ++i )
	{
		CNavArea *area = m_blockedAreas[i];
		area->UpdateBlocked();
	}
}


//--------------------------------------------------------------------------------------------------------
void CNavMesh::RegisterAvoidanceObstacle( INavAvoidanceObstacle *obstruction )
{
	m_avoidanceObstacles.FindAndRemove( obstruction );
	m_avoidanceObstacles.AddToTail( obstruction );
}


//--------------------------------------------------------------------------------------------------------
void CNavMesh::UnregisterAvoidanceObstacle( INavAvoidanceObstacle *obstruction )
{
	m_avoidanceObstacles.FindAndRemove( obstruction );
}


//--------------------------------------------------------------------------------------------------------
// invoked when the area becomes blocked
void CNavMesh::OnAvoidanceObstacleEnteredArea( CNavArea *area )
{
	if ( !m_avoidanceObstacleAreas.HasElement( area ) )
	{
		m_avoidanceObstacleAreas.AddToTail( area );
	}
}


//--------------------------------------------------------------------------------------------------------
// invoked when the area becomes un-blocked
void CNavMesh::OnAvoidanceObstacleLeftArea( CNavArea *area )
{
	m_avoidanceObstacleAreas.FindAndRemove( area );
}


//--------------------------------------------------------------------------------------------------------
void CNavMesh::UpdateAvoidanceObstacleAreas( void )
{
	for ( int i=0; i<m_avoidanceObstacleAreas.Count(); ++i )
	{
		CNavArea *area = m_avoidanceObstacleAreas[i];
		area->UpdateAvoidanceObstacles();
	}
}


//--------------------------------------------------------------------------------------------------------
void CNavMesh::BeginVisibilityComputations( void )
{
}

//--------------------------------------------------------------------------------------------------------
/**
 * Invoked when custom analysis step is complete
 */
void CNavMesh::EndVisibilityComputations( void )
{
}


//--------------------------------------------------------------------------------------------------------------
unsigned int CNavMesh::GetGenerationTraceMask( void ) const
{
	return MASK_NPCSOLID_BRUSHONLY;
}

//--------------------------------------------------------------------------------------------------------------
CNavArea *CNavMesh::CreateArea( void ) const
{
	return new CNavArea(GetNavPlace());
}

//--------------------------------------------------------------------------------------------------------------
void CNavMesh::DestroyArea( CNavArea *pArea ) const
{
	delete pArea;
}

std::shared_ptr<CWaypoint> CNavMesh::CreateWaypoint() const
{
	return std::make_shared<CWaypoint>();
}

std::shared_ptr<CNavVolume> CNavMesh::CreateVolume() const
{
	return std::make_shared<CNavVolume>();
}

std::shared_ptr<CNavElevator> CNavMesh::CreateElevator() const
{
	return std::make_shared<CNavElevator>();
}

std::shared_ptr<CNavPrerequisite> CNavMesh::CreatePrerequisite() const
{
	return std::make_shared<CNavPrerequisite>();
}

void CNavMesh::RebuildWaypointMap()
{
	std::unordered_map<WaypointID, std::shared_ptr<CWaypoint>> temp;
	temp.reserve(m_waypoints.size());
	temp.swap(m_waypoints);

	m_waypoints.clear();

	for (auto& wpt : temp)
	{
		m_waypoints[wpt.second->GetID()] = std::move(wpt.second);
	}

	temp.clear();
}

void CNavMesh::RebuildVolumeMap()
{
	std::unordered_map<unsigned int, std::shared_ptr<CNavVolume>> temp;
	temp.reserve(m_volumes.size());
	temp.swap(m_volumes);

	m_volumes.clear();

	for (auto& volume : temp)
	{
		m_volumes[volume.second->GetID()] = std::move(volume.second);
	}

	temp.clear();
}

void CNavMesh::RebuildElevatorMap()
{
	std::unordered_map<unsigned int, std::shared_ptr<CNavElevator>> temp;
	temp.reserve(m_elevators.size());
	temp.swap(m_elevators);

	m_elevators.clear();

	for (auto& elevator : temp)
	{
		m_elevators[elevator.second->GetID()] = std::move(elevator.second);
	}

	temp.clear();
}

void CNavMesh::RebuildPrerequisiteMap()
{
	std::unordered_map<unsigned int, std::shared_ptr<CNavPrerequisite>> temp;
	temp.reserve(m_prerequisites.size());
	temp.swap(m_prerequisites);

	m_prerequisites.clear();

	for (auto& prerequisite : temp)
	{
		m_prerequisites[prerequisite.second->GetID()] = std::move(prerequisite.second);
	}

	temp.clear();
}

std::optional<const std::shared_ptr<CWaypoint>> CNavMesh::AddWaypoint(const Vector& origin)
{
	std::shared_ptr<CWaypoint> wpt = CreateWaypoint();

	if (m_waypoints.count(wpt->GetID()) > 0)
	{
#ifdef EXT_DEBUG
		Warning("CNavMesh::AddWaypoint duplicate waypoint ID %i\n", wpt->GetID());
#endif // EXT_DEBUG

		return std::nullopt;
	}

	wpt->SetOrigin(origin);

	m_waypoints[wpt->GetID()] = wpt;

	return wpt;
}

std::optional<const std::shared_ptr<CNavVolume>> CNavMesh::AddNavVolume(const Vector& origin)
{
	std::shared_ptr<CNavVolume> volume = CreateVolume();

	if (m_volumes.count(volume->GetID()) > 0)
	{
#ifdef EXT_DEBUG
		Warning("CNavMesh::AddNavVolume duplicate volume ID %i\n", volume->GetID());
#endif // EXT_DEBUG

		return std::nullopt;
	}

	volume->SetOrigin(origin);
	Vector mins(-64.0f, -64.0f, 0.0f);
	Vector maxs(64.0f, 64.0f, 64.0f);
	volume->SetBounds(mins, maxs);

	m_volumes[volume->GetID()] = volume;
	return volume;
}

std::optional<const std::shared_ptr<CNavElevator>> CNavMesh::AddNavElevator(CBaseEntity* elevator)
{
	std::shared_ptr<CNavElevator> navelev = CreateElevator();

	if (m_elevators.count(navelev->GetID()) > 0)
	{
#ifdef EXT_DEBUG
		Warning("CNavMesh::AddNavElevator duplicate elevator ID %i\n", navelev->GetID());
#endif // EXT_DEBUG

		return std::nullopt;
	}

	// entity will be null when loading from a save
	if (elevator != nullptr)
	{
		navelev->AssignElevatorEntity(elevator);
	}

	m_elevators[navelev->GetID()] = navelev;
	return navelev;
}

void CNavMesh::RemoveWaypoint(CWaypoint* wpt)
{
	WaypointID key = wpt->GetID();

	if (m_waypoints.count(key) == 0)
	{
		Warning("Waypoint %i is a stray!\n", key);
		return; // Stray waypoint? (Reminder that waypoints are always created as shared_ptr)
	}

	for (auto& pair : m_waypoints)
	{
		auto& other = pair.second;

		if (other->GetID() == key)
			continue; // skip self

		other->NotifyWaypointDestruction(wpt);
	}

	m_waypoints.erase(key);
}

void CNavMesh::RemoveSelectedVolume()
{
	if (!m_selectedVolume)
	{
		return;
	}

	unsigned int key = m_selectedVolume->GetID();
	m_volumes.erase(key);
	m_selectedVolume = nullptr;
}

void CNavMesh::SetSelectedVolume(CNavVolume* volume)
{
	if (volume == nullptr)
	{
		m_selectedVolume = nullptr;
	}

	for (auto& i : m_volumes)
	{
		if (i.second.get() == volume)
		{
			m_selectedVolume = i.second;
			return;
		}
	}
}

std::optional<const std::shared_ptr<CNavPrerequisite>> CNavMesh::AddNavPrerequisite(const Vector* origin)
{
	std::shared_ptr<CNavPrerequisite> prereq = CreatePrerequisite();

	if (m_prerequisites.count(prereq->GetID()) > 0)
	{
#ifdef EXT_DEBUG
		Warning("CNavMesh::AddNavPrerequisite duplicate prerequisite ID %i\n", prereq->GetID());
#endif // EXT_DEBUG

		return std::nullopt;
	}

	if (origin)
	{
		prereq->SetOrigin(*origin);
	}

	m_prerequisites[prereq->GetID()] = prereq;
	return prereq;
}

void CNavMesh::CompressWaypointsIDs()
{
	CWaypoint::g_NextWaypointID = 0;

	for (auto& pair : m_waypoints)
	{
		auto& wpt = pair.second;
		wpt->m_ID = CWaypoint::g_NextWaypointID;
		CWaypoint::g_NextWaypointID++;
	}

	RebuildWaypointMap();
}

void CNavMesh::CompressVolumesIDs()
{
	CNavVolume::s_nextID = 0;

	for (auto& pair : m_volumes)
	{
		auto& volume = pair.second;
		volume->m_id = CNavVolume::s_nextID;
		CNavVolume::s_nextID++;
	}


	RebuildVolumeMap();
}

void CNavMesh::CompressElevatorsIDs()
{
	CNavElevator::s_nextID = 0;

	for (auto& pair : m_elevators)
	{
		auto& elevator = pair.second;
		elevator->m_id = CNavElevator::s_nextID;
		CNavElevator::s_nextID++;
	}

	RebuildElevatorMap();
}

void CNavMesh::SetSelectedElevator(CNavElevator* elevator)
{
	if (elevator == nullptr)
	{
		m_selectedElevator = nullptr;
	}

	if (m_selectedElevator.get() == elevator)
	{
		m_selectedElevator = nullptr;
	}

	for (auto& pair : m_elevators)
	{
		if (pair.second.get() == elevator)
		{
			m_selectedElevator = pair.second;
			Msg("Selected elevator %i\n", pair.second->GetID());
			return;
		}
	}
}

void CNavMesh::SelectNearestWaypoint(const Vector& start)
{
	std::shared_ptr<CWaypoint> nearest = nullptr;
	float best = 99999999.0f;

	std::for_each(m_waypoints.begin(), m_waypoints.end(), [&nearest, &best, &start](const std::pair<WaypointID, std::shared_ptr<CWaypoint>>& object) {

		float distance = (object.second->GetOrigin() - start).Length();

		if (distance <= CWaypoint::WAYPOINT_DELETE_SEARCH_DIST && distance < best)
		{
			nearest = object.second;
			best = distance;
		}
	});

	if (nearest)
	{
		m_selectedWaypoint = nearest;
		Msg("Selected waypoint %i\n", nearest->GetID());
	}
	else
	{
		Msg("No waypoint found!\n");
	}

}

void CNavMesh::SelectWaypointofID(WaypointID id)
{
	auto wpt = GetWaypointOfID<CWaypoint>(id);

	if (wpt.has_value())
	{
		m_selectedWaypoint = wpt.value();
		Msg("Selected waypoint %i \n", id);
		return;
	}

	Warning("Waypoint of ID %i not found! \n", id);
}

void CNavMesh::SelectNearestElevator(const Vector& point)
{
	float dist = FLT_MAX;
	std::shared_ptr<CNavElevator> nearest = nullptr;

	std::for_each(m_elevators.begin(), m_elevators.end(), [&dist, &nearest, &point](const std::pair<unsigned int, std::shared_ptr<CNavElevator>>& object) {
		float d = (object.second->GetElevatorOrigin() - point).Length();

		if (d < dist)
		{
			dist = d;
			nearest = object.second;
		}
	});

	if (nearest != nullptr)
	{
		Msg("Selected elevator #%i\n", nearest->GetID());
	}

	m_selectedElevator = nearest;
}

void CNavMesh::DeleteElevator(CNavElevator* elevator)
{
	m_selectedElevator = nullptr;

	unsigned int id = elevator->GetID();
	m_elevators.erase(id);
}

void CNavMesh::SetSelectedPrerequisite(CNavPrerequisite* prereq)
{
	if (prereq == nullptr)
	{
		m_selectedPrerequisite = nullptr;
		return;
	}

	for (auto& pair : m_prerequisites)
	{
		if (pair.second.get() == prereq)
		{
			m_selectedPrerequisite = pair.second;
			return;
		}
	}
}

bool CNavMesh::SelectPrerequisiteByID(unsigned int id)
{
	auto it = m_prerequisites.find(id);

	if (it == m_prerequisites.end())
	{
		return false;
	}

	m_selectedPrerequisite = it->second;
	return true;
}

void CNavMesh::SelectNearestPrerequisite(const Vector& point)
{
	float best = 9999999.0f;

	for (auto& pair : m_prerequisites)
	{
		const Vector& end = pair.second->GetOrigin();
		float range = (point - end).Length();
		
		if (range < best)
		{
			m_selectedPrerequisite = pair.second;
			best = range;
		}
	}
}

void CNavMesh::DeletePrerequisite(CNavPrerequisite* prereq)
{
	unsigned int id = prereq->GetID();
	m_prerequisites.erase(id);
	m_selectedPrerequisite = nullptr;
}

void CNavMesh::CompressPrerequisiteIDs()
{
	CNavPrerequisite::s_nextID = 0;

	for (auto& pair : m_prerequisites)
	{
		pair.second->m_id = CNavPrerequisite::s_nextID;
		CNavPrerequisite::s_nextID++;
	}

	RebuildPrerequisiteMap();
}

bool CNavMesh::IsClimbableSurface(const trace_t& tr)
{
	/*
	* This function is called by CNavMesh::FindActiveNavArea to set the value of CNavMesh::m_climbableSurface
	* Ladders are an example of a climable surface
	* Override per mod needs
	*/

	bool climbable = false;

	climbable = physprops->GetSurfaceData(tr.surface.surfaceProps)->game.climbable != 0;

	if (!climbable)
	{
		climbable = (tr.contents & CONTENTS_LADDER) != 0;
	}

	if (climbable)
	{
		// check if we're on the same plane as the original point when we're building a ladder
		if (IsEditMode(CREATING_LADDER))
		{
			if (m_surfaceNormal != m_ladderNormal)
			{
				climbable = false;
			}
		}

		if (m_surfaceNormal.z > 0.9f)
		{
			climbable = false; // don't try to build ladders on flat ground
		}
	}

	return climbable;
}