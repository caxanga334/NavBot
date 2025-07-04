//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
// nav_node.cpp
// AI Navigation Nodes
// Author: Michael S. Booth (mike@turtlerockstudios.com), January 2003

#include NAVBOT_PCH_FILE
#include <extension.h>
#include <sdkports/debugoverlay_shared.h>
#include <sdkports/sdk_traces.h>
#include <sdkports/sdk_utils.h>

#include "nav_trace.h"
#include "nav_node.h"
#include "nav_colors.h"
#include "nav_mesh.h"
#include "nav.h"
#include "tier1/utlhash.h"
#include "tier1/generichash.h"


NavDirType Opposite[ NUM_DIRECTIONS ] = { SOUTH, WEST, NORTH, EAST };

CNavNode *CNavNode::m_list = NULL;
unsigned int CNavNode::m_listLength = 0;
unsigned int CNavNode::m_nextID = 1;

extern Vector NavTraceMins;
extern Vector NavTraceMaxs;
extern IVDebugOverlay* debugoverlay;

//--------------------------------------------------------------------------------------------------------------
// Node hash

class CNodeHashFuncs
{
public:
	CNodeHashFuncs( int ) {}

	bool operator()( const CNavNode *pLhs, const CNavNode *pRhs ) const
	{
		return pRhs->GetPosition()->AsVector2D() == pLhs->GetPosition()->AsVector2D();
	}

	unsigned int operator()( const CNavNode *pItem ) const
	{
		return Hash8( &pItem->GetPosition()->AsVector2D() );	
	}
};

CUtlHash<CNavNode *, CNodeHashFuncs, CNodeHashFuncs> *g_pNavNodeHash;


//--------------------------------------------------------------------------------------------------------------
/**
 * Constructor
 */
CNavNode::CNavNode( const Vector &pos, const Vector &normal, CNavNode *parent, bool isOnDisplacement )
{
	m_pos = pos;
	m_normal = normal;

	m_id = m_nextID++;

	int i;
	for( i=0; i<NUM_DIRECTIONS; ++i )
	{
		m_to[ i ] = NULL;
		m_obstacleHeight[ i ] = 0;
		m_obstacleStartDist[ i ] = 0;
		m_obstacleEndDist[ i ] = 0;
	}

	for ( i=0; i<NUM_CORNERS; ++i )
	{
		m_crouch[ i ] = false;
		m_isBlocked[ i ] = false;
	}

	m_visited = 0;
	m_parent = parent;

	m_next = m_list;
	m_list = this;
	m_listLength++;

	m_isCovered = false;
	m_area = NULL;

	m_attributeFlags = 0;

	m_isOnDisplacement = isOnDisplacement;

	if ( !g_pNavNodeHash )
	{
		g_pNavNodeHash = new CUtlHash<CNavNode *, CNodeHashFuncs, CNodeHashFuncs>( 16*1024 );
	}

	bool bDidInsert;
	UtlHashHandle_t hHash = g_pNavNodeHash->Insert( this, &bDidInsert );
	if ( !bDidInsert )
	{
		CNavNode *pExistingNode = g_pNavNodeHash->Element( hHash );
		m_nextAtXY = pExistingNode;
		g_pNavNodeHash->Element( hHash ) = this;
	}
	else
	{
		m_nextAtXY = NULL;
	}
}

CNavNode::~CNavNode()
{
}


//--------------------------------------------------------------------------------------------------------------
void CNavNode::CleanupGeneration()
{
	delete g_pNavNodeHash;
	g_pNavNodeHash = NULL;

	CNavNode *node, *next;
	for( node = CNavNode::m_list; node; node = next )
	{
		next = node->m_next;
		delete node;
	}
	CNavNode::m_list = NULL;
	CNavNode::m_listLength = 0;
	CNavNode::m_nextID = 1;
}

//--------------------------------------------------------------------------------------------------------------
#if DEBUG_NAV_NODES
ConVar sm_nav_show_nodes( "sm_nav_show_nodes", "0", FCVAR_CHEAT );
ConVar sm_nav_show_node_id( "sm_nav_show_node_id", "0", FCVAR_CHEAT );
ConVar sm_nav_test_node( "sm_nav_test_node", "0", FCVAR_CHEAT );
ConVar sm_nav_test_node_crouch( "sm_nav_test_node_crouch", "0", FCVAR_CHEAT );
ConVar sm_nav_test_node_crouch_dir( "sm_nav_test_node_crouch_dir", "4", FCVAR_CHEAT );
ConVar sm_nav_show_node_grid( "sm_nav_show_node_grid", "0", FCVAR_CHEAT );
#endif // DEBUG_NAV_NODES


void Cross3D(const Vector &position, const Vector &mins, const Vector &maxs,
		int r, int g, int b, bool noDepthTest, float fDuration) {
	Vector start = mins + position;
	Vector end   = maxs + position;
	debugoverlay->AddLineOverlay(start,end, r, g, b, noDepthTest,fDuration);

	start.x += (maxs.x - mins.x);
	end.x	-= (maxs.x - mins.x);
	debugoverlay->AddLineOverlay(start,end, r, g, b, noDepthTest,fDuration);

	start.y += (maxs.y - mins.y);
	end.y	-= (maxs.y - mins.y);
	debugoverlay->AddLineOverlay(start,end, r, g, b, noDepthTest,fDuration);

	start.x -= (maxs.x - mins.x);
	end.x	+= (maxs.x - mins.x);
	debugoverlay->AddLineOverlay(start,end, r, g, b, noDepthTest,fDuration);
}

//-----------------------------------------------------------------------------
// Purpose: Draw a colored 3D cross of the given size at the given position
//-----------------------------------------------------------------------------
void Cross3D(const Vector &position, float size, int r, int g,
		int b, bool noDepthTest, float flDuration) {
	debugoverlay->AddLineOverlay(position + Vector(size, 0, 0),
			position - Vector(size, 0, 0), r, g, b, noDepthTest, flDuration);
	debugoverlay->AddLineOverlay(position + Vector(0, size, 0),
			position - Vector(0, size, 0), r, g, b, noDepthTest, flDuration);
	debugoverlay->AddLineOverlay(position + Vector(0, 0, size),
			position - Vector(0, 0, size), r, g, b, noDepthTest, flDuration);
}

//-----------------------------------------------------------------------------
// Purpose: Draw debug text at a position
//-----------------------------------------------------------------------------
void Text(const Vector &origin, const char *text, bool bViewCheck,
		float duration) {
	edict_t *ent = UTIL_GetListenServerEnt();
	extern IPlayerInfoManager* playerinfomanager;
	IPlayerInfo* player = playerinfomanager->GetPlayerInfo(ent);
	if (!player)
		return;
	const unsigned int MAX_OVERLAY_DIST_SQR	= 90000000;

	// Clip text that is far away
	if ((player->GetAbsOrigin() - origin).LengthSqr() > MAX_OVERLAY_DIST_SQR)
		return;
	extern IServerGameClients* gameclients;
	// Clip text that is behind the client
	Vector clientForward;
	gameclients->ClientEarPosition(ent, &clientForward);
	AngleVectors( player->GetAbsAngles(), &clientForward, nullptr, nullptr );


	Vector toText = origin - player->GetAbsOrigin();
	float dotPr = DotProduct(clientForward, toText);

	if (dotPr < 0)
		return;

	// Clip text that is obscured
	if (bViewCheck) {
		trace_t tr;

		trace::line(player->GetAbsOrigin(), origin, MASK_OPAQUE, nullptr, COLLISION_GROUP_NONE, tr);

		if ((tr.endpos - origin).Length() > 10)
			return;
	}

	if (debugoverlay)
	{
		debugoverlay->AddTextOverlay(origin, duration, "%s", text);
	}
}

//--------------------------------------------------------------------------------------------------------------
void CNavNode::Draw( void )
{
#if DEBUG_NAV_NODES

	if ( !sm_nav_show_nodes.GetBool() )
		return;

	int r = 0, g = 0, b = 0;

	if ( m_isCovered )
	{
		if ( GetAttributes() & NAV_MESH_CROUCH )
		{
			b = 255;
		}
		else
		{
			r = 255;
		}
	}
	else
	{
		if ( GetAttributes() & NAV_MESH_CROUCH )
		{
			b = 255;
		}
		g = 255;
	}

	Cross3D( m_pos, 2, r, g, b, true, 0.1f );

	if ( (!m_isCovered && sm_nav_show_node_id.GetBool()) || (m_isCovered && sm_nav_show_node_id.GetInt() < 0) )
	{
		char text[16];
		Q_snprintf( text, sizeof( text ), "%d", m_id );
		Text( m_pos, text, true, 0.1f );
	}

	if ( (unsigned int)(sm_nav_test_node.GetInt()) == m_id )
	{
		TheNavMesh->TestArea( this, 1, 1 );
		sm_nav_test_node.SetValue( 0 );
	}

	if ( (unsigned int)(sm_nav_test_node_crouch.GetInt()) == m_id )
	{
		CheckCrouch();
		sm_nav_test_node_crouch.SetValue( 0 );
	}

	if ( GetAttributes() & NAV_MESH_CROUCH )
	{
		int i;
		for( i=0; i<NUM_CORNERS; i++ )
		{
			if ( m_isBlocked[i] || m_crouch[i] )
			{
				Vector2D dir;
				CornerToVector2D( (NavCornerType)i, &dir );

				const float scale = 3.0f;
				Vector scaled( dir.x * scale, dir.y * scale, 0 );

				if ( m_isBlocked[i] )
				{
					HorzArrow( m_pos, m_pos + scaled, 0.5, 255, 0, 0, 255, true, 0.1f );
				}
				else
				{
					HorzArrow( m_pos, m_pos + scaled, 0.5, 0, 0, 255, 255, true, 0.1f );
				}
			}
		}
	}

	if (sm_nav_show_node_grid.GetBool() )
	{
		for ( int i = NORTH; i < NUM_DIRECTIONS; i++ )
		{
			CNavNode *nodeNext = GetConnectedNode( (NavDirType) i );
			if ( nodeNext )
			{
				debugoverlay->AddLineOverlay( *GetPosition(), *nodeNext->GetPosition(), 255, 255, 0, false, 0.1f );

				float obstacleHeight = m_obstacleHeight[i];
				if ( obstacleHeight > 0 )
				{
					float z = GetPosition()->z + obstacleHeight;
					Vector from = *GetPosition();
					Vector to = from;
					AddDirectionVector( &to, (NavDirType) i, m_obstacleStartDist[i] );
					debugoverlay->AddLineOverlay( from, to, 255, 0, 255, false, 0.1f );
					from = to;
					to.z = z;
					debugoverlay->AddLineOverlay( from, to, 255, 0, 255, false, 0.1f );
					from = to;
					to = *GetPosition();
					to.z = z;
					AddDirectionVector( &to, (NavDirType) i, m_obstacleEndDist[i] );
					debugoverlay->AddLineOverlay( from, to, 255, 0, 255, false, 0.1f );
				}				
			}
		}
	}
	

#endif // DEBUG_NAV_NODES
}


//--------------------------------------------------------------------------------------------------------
// return ground height above node in given corner direction (NUM_CORNERS for highest in any direction)
float CNavNode::GetGroundHeightAboveNode( NavCornerType cornerType ) const
{
	if ( cornerType >= 0 && cornerType < NUM_CORNERS )
		return m_groundHeightAboveNode[ cornerType ];

	float blockedHeight = 0.0f;
	for ( int i=0; i<NUM_CORNERS; ++i )
	{
		blockedHeight = MAX( blockedHeight, m_groundHeightAboveNode[i] );
	}

	return blockedHeight;
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Look up to JumpCrouchHeight in the air to see if we can fit a whole navgenparams->human_height box
 */
bool CNavNode::TestForCrouchArea( NavCornerType cornerNum, const Vector& mins, const Vector& maxs, float *groundHeightAboveNode )
{
	CTraceFilterWalkableEntities filter( NULL, COLLISION_GROUP_PLAYER_MOVEMENT, WALK_THRU_EVERYTHING);
	trace_t tr;

	Vector start( m_pos );
	Vector end( start );
	end.z += navgenparams->jump_crouch_height;
	trace::hull(start, end, NavTraceMins, NavTraceMaxs, MASK_PLAYERSOLID_BRUSHONLY, &filter, tr);

	float maxHeight = tr.endpos.z - start.z;

	Vector realMaxs( maxs );

	for ( float height = 0; height <= maxHeight; height += 1.0f )
	{
		start = m_pos;
		start.z += height;

		realMaxs.z = navgenparams->human_crouch_height;

		trace::hull(start, start, mins, realMaxs, MASK_PLAYERSOLID_BRUSHONLY, &filter, tr);

		if ( !tr.startsolid )
		{
			*groundHeightAboveNode = start.z - m_pos.z;

			// We found a crouch-sized space.  See if we can stand up.
			realMaxs.z = navgenparams->human_height;

			trace::hull(start, start, mins, realMaxs, MASK_PLAYERSOLID_BRUSHONLY, &filter, tr);

			if ( !tr.startsolid )
			{
				// We found a crouch-sized space.  See if we can stand up.
#if DEBUG_NAV_NODES
				if ( (unsigned int)(sm_nav_test_node_crouch.GetInt()) == GetID() )
				{
					debugoverlay->AddBoxOverlay(start, mins, maxs,
							QAngle(0.0f, 0.0f, 0.0f), 0, 255, 255, 100, 100);
				}
#endif // DEBUG_NAV_NODES
				return true;
			}
#if DEBUG_NAV_NODES
			if ( (unsigned int)(sm_nav_test_node_crouch.GetInt()) == GetID() )
			{
				debugoverlay->AddBoxOverlay(start, mins, maxs,
						QAngle(0.0f, 0.0f, 0.0f), 255, 0, 0, 100, 100);
			}
#endif // DEBUG_NAV_NODES

			return false;
		}
	}

	*groundHeightAboveNode = navgenparams->jump_crouch_height;
	m_isBlocked[ cornerNum ] = true;
	return false;
}


//--------------------------------------------------------------------------------------------------------------
void CNavNode::CheckCrouch( void )
{
	// For each direction, trace upwards from our best ground height to VEC_HULL_MAX.z to see if we have standing room.
	for ( int i=0; i<NUM_CORNERS; ++i )
	{
#if DEBUG_NAV_NODES
		if (sm_nav_test_node_crouch_dir.GetInt() != NUM_CORNERS && i != sm_nav_test_node_crouch_dir.GetInt() )
			continue;
#endif // DEBUG_NAV_NODES

		NavCornerType corner = (NavCornerType)i;
		Vector2D cornerVec;
		CornerToVector2D( corner, &cornerVec );

		// Build a mins/maxs pair for the HumanWidth x navgenparams->half_human_width box facing the appropriate direction
		Vector mins( 0, 0, 0 );
		Vector maxs( 0, 0, 0 );
		if ( cornerVec.x < 0 )
		{
			mins.x = -navgenparams->half_human_width;
		}
		else if ( cornerVec.x > 0 )
		{
			maxs.x = navgenparams->half_human_width;
		}
		if ( cornerVec.y < 0 )
		{
			mins.y = -navgenparams->half_human_width;
		}
		else if ( cornerVec.y > 0 )
		{
			maxs.y = navgenparams->half_human_width;
		}
		maxs.z = navgenparams->human_height;

		// now make sure that mins is smaller than maxs
		for ( int j=0; j<3; ++j )
		{
			if ( mins[j] > maxs[j] )
			{
				float tmp = mins[j];
				mins[j] = maxs[j];
				maxs[j] = tmp;
			}
		}

		if ( !TestForCrouchArea( corner, mins, maxs, &m_groundHeightAboveNode[i] ) )
		{
			SetAttributes( NAV_MESH_CROUCH );
			m_crouch[corner] = true;
		}
	}
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Create a connection FROM this node TO the given node, in the given direction
 */
void CNavNode::ConnectTo( CNavNode *node, NavDirType dir, float obstacleHeight, float obstacleStartDist, float obstacleEndDist )
{
	m_to[ dir ] = node;
	m_obstacleHeight[ dir ] = obstacleHeight;
	m_obstacleStartDist[ dir ] = obstacleStartDist;
	m_obstacleEndDist[ dir ] = obstacleEndDist;
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Return node at given position.
 * @todo Need a hash table to make this lookup fast
 */
CNavNode *CNavNode::GetNode( const Vector &pos )
{
	const float tolerance = 0.45f * navgenparams->generation_step_size;			// 1.0f
	CNavNode *pNode = NULL;
	if ( g_pNavNodeHash )
	{
		static CNavNode lookup;
		lookup.m_pos = pos;
		UtlHashHandle_t hNode = g_pNavNodeHash->Find( &lookup );

		if ( hNode != g_pNavNodeHash->InvalidHandle() )
		{
			for( pNode = g_pNavNodeHash->Element( hNode ); pNode; pNode = pNode->m_nextAtXY )
			{
				if (fabs( pNode->m_pos.z - pos.z ) < tolerance)
				{
					break;
				}
			}
		}
	}

#ifdef DEBUG_NODE_HASH
	CNavNode *pTestNode = NULL;
	for( CNavNode *node = m_list; node; node = node->m_next )
	{
		float dx = fabs( node->m_pos.x - pos.x );
		float dy = fabs( node->m_pos.y - pos.y );
		float dz = fabs( node->m_pos.z - pos.z );

		if (dx < tolerance && dy < tolerance && dz < tolerance)
		{
			pTestNode = node;
			break;
		}
	}
	AssertFatal( pTestNode == pNode );
#endif

	return pNode;
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Return true if this node is bidirectionally linked to 
 * another node in the given direction
 */
bool CNavNode::IsBiLinked( NavDirType dir ) const
{
	return m_to[ dir ] && m_to[ dir ]->m_to[ Opposite[dir] ] == this;
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Return true if this node is the NW corner of a quad of nodes
 * that are all bidirectionally linked.
 */
bool CNavNode::IsClosedCell( void ) const
{
	return IsBiLinked( SOUTH ) &&
		IsBiLinked( EAST ) &&
		m_to[ EAST ]->IsBiLinked( SOUTH ) &&
		m_to[ SOUTH ]->IsBiLinked( EAST ) &&
		m_to[ EAST ]->m_to[ SOUTH ] == m_to[ SOUTH ]->m_to[ EAST ];
}

