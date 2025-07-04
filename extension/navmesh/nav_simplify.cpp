//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
// nav_simplify.cpp

#include NAVBOT_PCH_FILE
#include "nav_mesh.h"
#include "nav_area.h"
#include "nav_node.h"
#include <eiface.h>

extern ConVar sm_nav_snap_to_grid;
extern ConVar sm_nav_split_place_on_ground;
extern ConVar sm_nav_coplanar_slope_limit;
extern ConVar sm_nav_coplanar_slope_limit_displacement;
extern IVEngineServer* engine;
extern NavAreaVector TheNavAreas;

//--------------------------------------------------------------------------------------------------------
static bool ReduceToComponentAreas( CNavArea *area, bool addToSelectedSet )
{
	if ( !area )
		return false;

	bool splitAlongX;
	float splitEdge;

	const float minSplitSize = 2.0f; // ensure the first split is larger than this

	float sizeX = area->GetSizeX();
	float sizeY = area->GetSizeY();

	CNavArea *first = NULL;
	CNavArea *second = NULL;
	CNavArea *third = NULL;
	CNavArea *fourth = NULL;

	bool didSplit = false;

	if ( sizeX > navgenparams->generation_step_size )
	{
		splitEdge = RoundToUnits( area->GetCorner( NORTH_WEST ).x, navgenparams->generation_step_size );
		if ( splitEdge < area->GetCorner( NORTH_WEST ).x + minSplitSize )
			splitEdge += navgenparams->generation_step_size;
		splitAlongX = false;

		didSplit = area->SplitEdit( splitAlongX, splitEdge, &first, &second );
	}

	if ( sizeY > navgenparams->generation_step_size )
	{
		splitEdge = RoundToUnits( area->GetCorner( NORTH_WEST ).y, navgenparams->generation_step_size );
		if ( splitEdge < area->GetCorner( NORTH_WEST ).y + minSplitSize )
			splitEdge += navgenparams->generation_step_size;
		splitAlongX = true;

		if ( didSplit )
		{
			didSplit = first->SplitEdit( splitAlongX, splitEdge, &third, &fourth );
			didSplit = second->SplitEdit( splitAlongX, splitEdge, &first, &second );
		}
		else
		{
			didSplit = area->SplitEdit( splitAlongX, splitEdge, &first, &second );
		}
	}

	if ( !didSplit )
		return false;

	if ( addToSelectedSet )
	{
		TheNavMesh->AddToSelectedSet( first );
		TheNavMesh->AddToSelectedSet( second );
		TheNavMesh->AddToSelectedSet( third );
		TheNavMesh->AddToSelectedSet( fourth );
	}

	ReduceToComponentAreas( first, addToSelectedSet );
	ReduceToComponentAreas( second, addToSelectedSet );
	ReduceToComponentAreas( third, addToSelectedSet );
	ReduceToComponentAreas( fourth, addToSelectedSet );

	return true;
}


//--------------------------------------------------------------------------------------------------------
CON_COMMAND_F(sm_nav_chop_selected, "Chops all selected areas into their component 1x1 areas", FCVAR_CHEAT )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() || engine->IsDedicatedServer() )
		return;

	TheNavMesh->StripNavigationAreas();
	TheNavMesh->SetMarkedArea( NULL );

	NavAreaCollector collector;
	TheNavMesh->ForAllSelectedAreas( collector );

	for ( int i=0; i<collector.m_area.Count(); ++i )
	{
		ReduceToComponentAreas( collector.m_area[i], true );
	}

	Msg( "%d areas chopped into %d\n", collector.m_area.Count(), TheNavMesh->GetSelecteSetSize() );
}


//--------------------------------------------------------------------------------------------------------
void CNavMesh::RemoveNodes( void )
{
	FOR_EACH_VEC( TheNavAreas, it )
	{
		TheNavAreas[ it ]->ResetNodes();
	}

	// destroy navigation nodes created during map generation
	CNavNode::CleanupGeneration();
}


//--------------------------------------------------------------------------------------------------------
void CNavMesh::GenerateNodes( const Extent &bounds )
{
	m_simplifyGenerationExtent = bounds;
	m_seedIdx = 0;

	Assert( m_generationMode == GENERATE_SIMPLIFY );
	while ( SampleStep() )
	{
		// do nothing
	}
}


//--------------------------------------------------------------------------------------------------------
// Simplifies the selected set by reducing to 1x1 areas and re-merging them up with loosened tolerances
void CNavMesh::SimplifySelectedAreas( void )
{
	// Save off somve cvars: we need to place nodes on ground, we need snap to grid set, and we loosen slope tolerances
	m_generationMode = GENERATE_SIMPLIFY;
	bool savedSplitPlaceOnGround = sm_nav_split_place_on_ground.GetBool();
	sm_nav_split_place_on_ground.SetValue( 1 );

	float savedCoplanarSlopeDisplacementLimit = sm_nav_coplanar_slope_limit_displacement.GetFloat();
	sm_nav_coplanar_slope_limit_displacement.SetValue( MIN( 0.5f, savedCoplanarSlopeDisplacementLimit ) );

	float savedCoplanarSlopeLimit = sm_nav_coplanar_slope_limit.GetFloat();
	sm_nav_coplanar_slope_limit.SetValue( MIN( 0.5f, savedCoplanarSlopeLimit ) );

	int savedGrid = sm_nav_snap_to_grid.GetInt();
	sm_nav_snap_to_grid.SetValue( 1 );

	StripNavigationAreas();
	SetMarkedArea( NULL );

	NavAreaCollector collector;
	ForAllSelectedAreas( collector );

	// Select walkable seeds and re-generate nodes in the bounds
	ClearWalkableSeeds();

	Extent bounds;
	bounds.lo.Init( FLT_MAX, FLT_MAX, FLT_MAX );
	bounds.hi.Init( -FLT_MAX, -FLT_MAX, -FLT_MAX );
	for ( int i=0; i<collector.m_area.Count(); ++i )
	{
		Extent areaExtent;
		CNavArea *area = collector.m_area[i];
		area->GetExtent( &areaExtent );
		areaExtent.lo.z -= navgenparams->human_height;
		areaExtent.hi.z += 2 * navgenparams->human_height;
		bounds.Encompass( areaExtent );

		Vector center = area->GetCenter();
		center.x = SnapToGrid( center.x );
		center.y = SnapToGrid( center.y );

		Vector normal;
		if ( FindGroundForNode( &center, &normal ) )
		{
			AddWalkableSeed( center, normal );

			center.z += navgenparams->human_height;
			bounds.Encompass( center );
		}
	}
	RemoveNodes();
	GenerateNodes( bounds );
	ClearWalkableSeeds();

	// Split nav areas up into 1x1 component areas
	for ( int i=0; i<collector.m_area.Count(); ++i )
	{
		ReduceToComponentAreas( collector.m_area[i], true );
	}

	// Assign nodes to each component area
	FOR_EACH_VEC( m_selectedSet, it )
	{
		CNavArea *area = m_selectedSet[ it ];

		Vector corner = area->GetCorner( NORTH_EAST );
		Vector normal;
		if ( FindGroundForNode( &corner, &normal ) )
		{
			area->m_node[ NORTH_EAST ] = CNavNode::GetNode( corner );
			if ( area->m_node[ NORTH_EAST ] )
			{
				area->m_node[ NORTH_WEST ] = area->m_node[ NORTH_EAST ]->GetConnectedNode( WEST );
				area->m_node[ SOUTH_EAST ] = area->m_node[ NORTH_EAST ]->GetConnectedNode( SOUTH );
				if ( area->m_node[ SOUTH_EAST ] )
				{
					area->m_node[ SOUTH_WEST ] = area->m_node[ SOUTH_EAST ]->GetConnectedNode( WEST );

					if ( area->m_node[ NORTH_WEST ] && area->m_node[ SOUTH_WEST ] )
					{
						area->AssignNodes( area );
					}
				}
			}
		}

		Assert ( area->m_node[ NORTH_EAST ] && area->m_node[ NORTH_WEST ] && area->m_node[ SOUTH_EAST ] && area->m_node[ SOUTH_WEST ] );
		if ( !( area->m_node[ NORTH_EAST ] && area->m_node[ NORTH_WEST ] && area->m_node[ SOUTH_EAST ] && area->m_node[ SOUTH_WEST ] ) )
		{
			Warning( "Area %d didn't get any nodes!\n", area->GetID() );
		}
	}

	// Run a subset of incremental generation on the component areas
	MergeGeneratedAreas();
	SquareUpAreas();
	MarkJumpAreas();
	SplitAreasUnderOverhangs();
	MarkStairAreas();
	StichAndRemoveJumpAreas();
	HandleObstacleTopAreas();
	FixUpGeneratedAreas();

	// Re-select the new areas
	ClearSelectedSet();
	FOR_EACH_VEC( TheNavAreas, i )
	{
		CNavArea *area = TheNavAreas[i];
		if ( area->HasNodes() )
		{
			AddToSelectedSet( area );
		}
	}

#ifndef _DEBUG
	// leave nodes in debug for testing
	RemoveNodes();
#endif

	m_generationMode = GENERATE_NONE;
	sm_nav_split_place_on_ground.SetValue( savedSplitPlaceOnGround );
	sm_nav_coplanar_slope_limit_displacement.SetValue( savedCoplanarSlopeDisplacementLimit );
	sm_nav_coplanar_slope_limit.SetValue( savedCoplanarSlopeLimit );
	sm_nav_snap_to_grid.SetValue( savedGrid );
}

//--------------------------------------------------------------------------------------------------------
CON_COMMAND_F(sm_nav_simplify_selected, "Chops all selected areas into their component 1x1 areas and re-merges them together into larger areas", FCVAR_CHEAT )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() || engine->IsDedicatedServer() )
		return;

	int selectedSetSize = TheNavMesh->GetSelecteSetSize();
	if ( selectedSetSize == 0 )
	{
		Msg( "nav_simplify_selected only works on the selected set\n" );
		return;
	}

	TheNavMesh->SimplifySelectedAreas();

	Msg( "%d areas simplified - %d remain\n", selectedSetSize, TheNavMesh->GetSelecteSetSize() );
}

