//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
// nav.h
// Data structures and constants for the Navigation Mesh system
// Author: Michael S. Booth (mike@turtlerockstudios.com), January 2003

#include "nav.h"
#include "nav_mesh.h"

// Update this function when creating mod specific nav mesh

CNavMesh* NavMeshFactory(void)
{
	return new CNavMesh;
}