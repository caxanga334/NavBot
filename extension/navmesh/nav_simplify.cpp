//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
// nav_simplify.cpp

#include "nav_mesh.h"
#include "nav_area.h"
#include "nav_node.h"
#include <eiface.h>

extern ConVar nav_snap_to_grid;
extern ConVar nav_split_place_on_ground;
extern ConVar nav_coplanar_slope_limit;
extern ConVar nav_coplanar_slope_limit_displacement;
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

	if ( sizeX > GenerationStepSize )
	{
		splitEdge = RoundToUnits( area->GetCorner( NORTH_WEST ).x, GenerationStepSize );
		if ( splitEdge < area->GetCorner( NORTH_WEST ).x + minSplitSize )
			splitEdge += GenerationStepSize;
		splitAlongX = false;

		didSplit = area->SplitEdit( splitAlongX, splitEdge, &first, &second );
	}

	if ( sizeY > GenerationStepSize )
	{
		splitEdge = RoundToUnits( area->GetCorner( NORTH_WEST ).y, GenerationStepSize );
		if ( splitEdge < area->GetCorner( NORTH_WEST ).y + minSplitSize )
			splitEdge += GenerationStepSize;
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
CON_COMMAND_F( nav_chop_selected, "Chops all selected areas into their component 1x1 areas", FCVAR_CHEAT )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() || engine->IsDedicatedServer() )
		return;

	TheNavMesh->StripNavigationAreas();
	TheNavMesh->SetMarkedArea(nullptr);

	NavAreaCollector collector;
	TheNavMesh->ForAllSelectedAreas( collector );

	for (auto area : collector.m_area)
	{
		ReduceToComponentAreas(area, true);
	}

	Msg( "%d areas chopped into %d\n", collector.m_area.size(), TheNavMesh->GetSelecteSetSize());
}


//--------------------------------------------------------------------------------------------------------
void CNavMesh::RemoveNodes( void )
{

	for (auto area : TheNavAreas)
	{
		area->ResetNodes();
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
	bool savedSplitPlaceOnGround = nav_split_place_on_ground.GetBool();
	nav_split_place_on_ground.SetValue( 1 );

	float savedCoplanarSlopeDisplacementLimit = nav_coplanar_slope_limit_displacement.GetFloat();
	nav_coplanar_slope_limit_displacement.SetValue( MIN( 0.5f, savedCoplanarSlopeDisplacementLimit ) );

	float savedCoplanarSlopeLimit = nav_coplanar_slope_limit.GetFloat();
	nav_coplanar_slope_limit.SetValue( MIN( 0.5f, savedCoplanarSlopeLimit ) );

	int savedGrid = nav_snap_to_grid.GetInt();
	nav_snap_to_grid.SetValue( 1 );

	StripNavigationAreas();
	SetMarkedArea( NULL );

	NavAreaCollector collector;
	ForAllSelectedAreas( collector );

	// Select walkable seeds and re-generate nodes in the bounds
	ClearWalkableSeeds();

	Extent bounds;
	bounds.lo.Init( FLT_MAX, FLT_MAX, FLT_MAX );
	bounds.hi.Init( -FLT_MAX, -FLT_MAX, -FLT_MAX );

	for (auto area : collector.m_area)
	{
		Extent areaExtent;
		area->GetExtent(&areaExtent);
		areaExtent.lo.z -= HalfHumanHeight;
		areaExtent.hi.z += 2 * HumanHeight;
		bounds.Encompass(areaExtent);

		Vector center = area->GetCenter();
		center.x = SnapToGrid(center.x);
		center.y = SnapToGrid(center.y);

		Vector normal;
		if (FindGroundForNode(&center, &normal))
		{
			AddWalkableSeed(center, normal);

			center.z += HumanHeight;
			bounds.Encompass(center);
		}
	}

	RemoveNodes();
	GenerateNodes( bounds );
	ClearWalkableSeeds();

	// Split nav areas up into 1x1 component areas
	for (auto area : collector.m_area)
	{
		ReduceToComponentAreas(area, true);
	}

	// Assign nodes to each component area

	for (auto it : m_selectedSet)
	{
		CNavArea* area = it;

		Vector corner = area->GetCorner(NORTH_EAST);
		Vector normal;
		if (FindGroundForNode(&corner, &normal))
		{
			area->m_node[NORTH_EAST] = CNavNode::GetNode(corner);
			if (area->m_node[NORTH_EAST])
			{
				area->m_node[NORTH_WEST] = area->m_node[NORTH_EAST]->GetConnectedNode(WEST);
				area->m_node[SOUTH_EAST] = area->m_node[NORTH_EAST]->GetConnectedNode(SOUTH);
				if (area->m_node[SOUTH_EAST])
				{
					area->m_node[SOUTH_WEST] = area->m_node[SOUTH_EAST]->GetConnectedNode(WEST);

					if (area->m_node[NORTH_WEST] && area->m_node[SOUTH_WEST])
					{
						area->AssignNodes(area);
					}
				}
			}
		}

		// Assert(area->m_node[NORTH_EAST] && area->m_node[NORTH_WEST] && area->m_node[SOUTH_EAST] && area->m_node[SOUTH_WEST]);
		if (!(area->m_node[NORTH_EAST] && area->m_node[NORTH_WEST] && area->m_node[SOUTH_EAST] && area->m_node[SOUTH_WEST]))
		{
			Warning("Area %d didn't get any nodes!\n", area->GetID());
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

	for (auto area : TheNavAreas)
	{
		if (area->HasNodes())
		{
			AddToSelectedSet(area);
		}
	}

#ifndef _DEBUG
	// leave nodes in debug for testing
	RemoveNodes();
#endif

	m_generationMode = GENERATE_NONE;
	nav_split_place_on_ground.SetValue( savedSplitPlaceOnGround );
	nav_coplanar_slope_limit_displacement.SetValue( savedCoplanarSlopeDisplacementLimit );
	nav_coplanar_slope_limit.SetValue( savedCoplanarSlopeLimit );
	nav_snap_to_grid.SetValue( savedGrid );
}


//--------------------------------------------------------------------------------------------------------
CON_COMMAND_F( nav_simplify_selected, "Chops all selected areas into their component 1x1 areas and re-merges them together into larger areas", FCVAR_CHEAT )
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

