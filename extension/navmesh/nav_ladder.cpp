//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

// AI Navigation areas
// Author: Michael S. Booth (mike@turtlerockstudios.com), January 2003

#include <extension.h>
#include <extplayer.h>
#include <util/entprops.h>
#include <entities/baseentity.h>
#include <sdkports/debugoverlay_shared.h>
#include <sdkports/sdk_traces.h>
#include <sdkports/sdk_utils.h>
#include "nav_area.h"
#include "nav_colors.h"
#include "nav.h"
#include "nav_mesh.h"
#include <shareddefs.h>

extern ConVar sm_nav_area_bgcolor;

unsigned int CNavLadder::m_nextID = 1;

//--------------------------------------------------------------------------------------------------------------
/**
 * Shift the nav area
 */
void CNavLadder::Shift( const Vector &shift )
{
	m_top += shift;
	m_bottom += shift;
}



 void CNavLadder::CompressIDs( CNavMesh* TheNaVmesh )
{
	m_nextID = 1;

	if ( TheNavMesh )
	{
		for ( int i=0; i<TheNavMesh->GetLadders().Count(); ++i )
		{
			TheNavMesh->GetLadders()[i]->m_id = m_nextID++;
		}
	}
}

//--------------------------------------------------------------------------------------------------------------
void CNavLadder::OnSplit( CNavArea *original, CNavArea *alpha, CNavArea *beta )
{
	for (auto& connect : m_connections)
	{
		if (connect.GetConnectedArea() == original)
		{
			if (alpha->GetDistanceSquaredToPoint(m_top) < beta->GetDistanceSquaredToPoint(m_top))
			{
				connect.connect = alpha;
			}
			else
			{
				connect.connect = beta;
			}
		}
	}
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Connect this ladder to given area
 */
void CNavLadder::ConnectTo( CNavArea *area )
{
	for (auto& connect : m_connections)
	{
		if (connect.GetConnectedArea() == area)
		{
			return;
		}
	}

	auto& newconnect = m_connections.emplace_back(area);
	const float center = (m_top.z + m_bottom.z) * 0.5f;

	if (area->GetCenter().z > center)
	{
		// connected area is closer to the ladder top, mark it as a 'top' connection
		newconnect.bottom = false;
		newconnect.point.x = m_top.x;
		newconnect.point.y = m_top.y;
		Vector close;
		area->GetClosestPointOnArea(m_top, &close);
		newconnect.point.z = close.z + (navgenparams->step_height / 2.0f);

		if (newconnect.point.z > m_top.z)
		{
			newconnect.point.z = m_top.z - 1.0f;
		}
	}
	else
	{
		// mark as bottom
		newconnect.bottom = true;
		newconnect.point.x = m_bottom.x;
		newconnect.point.y = m_bottom.y;
		Vector close;
		area->GetClosestPointOnArea(m_bottom, &close);
		newconnect.point.z = close.z + (navgenparams->step_height / 2.0f);

		if (newconnect.point.z < m_bottom.z)
		{
			newconnect.point.z = m_bottom.z + 1.0f;
		}
	}
}


//--------------------------------------------------------------------------------------------------------------

CNavLadder::~CNavLadder()
{
	// tell the other areas we are going away
	extern NavAreaVector TheNavAreas;
	FOR_EACH_VEC( TheNavAreas, it )
	{
		TheNavAreas[ it ]->OnDestroyNotify( this );
	}
}

//--------------------------------------------------------------------------------------------------------------
/**
 * invoked when given area is going away
 */
void CNavLadder::OnDestroyNotify( CNavArea *dead )
{
	Disconnect( dead );
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Disconnect this ladder from given area
 */
void CNavLadder::Disconnect( CNavArea *area )
{
	m_connections.erase(std::remove_if(m_connections.begin(), m_connections.end(), [&area](const LadderToAreaConnection& object) {
		return object.GetConnectedArea() == area;
	}), m_connections.end());
}


//--------------------------------------------------------------------------------------------------------------
/**
 * returns true if given area is connected in given direction
 */
bool CNavLadder::IsConnected( const CNavArea *area, LadderDirectionType dir ) const
{
	for (auto& connect : m_connections)
	{
		switch (dir)
		{
		case CNavLadder::LADDER_UP:

			if (connect.IsConnectedToLadderTop() && connect.GetConnectedArea() == area)
			{
				return true;
			}

			break;
		case CNavLadder::LADDER_DOWN:
		default:

			if (connect.IsConnectedToLadderBottom() && connect.GetConnectedArea() == area)
			{
				return true;
			}

			break;
		}
	}

	return false;
}

bool CNavLadder::IsConnected(const CNavArea* area) const
{
	for (auto& connect : m_connections)
	{
		if (connect.GetConnectedArea() == area)
		{
			return true;
		}
	}

	return false;
}

//--------------------------------------------------------------------------------------------------------------
void CNavLadder::SetDir( NavDirType dir )
{
	m_dir = dir;

	m_normal.Init();
	AddDirectionVector( &m_normal, m_dir, 1.0f );	// worst-case, we have the NavDirType as a normal

	Vector from = (m_top + m_bottom) * 0.5f + m_normal * 5.0f;
	Vector to = from - m_normal * 32.0f;

	trace_t result;
#ifdef TERROR
	// TERROR: use the MASK_ZOMBIESOLID_BRUSHONLY contents, since that's what zombies use
	UTIL_TraceLine( from, to, MASK_ZOMBIESOLID_BRUSHONLY, NULL, COLLISION_GROUP_NONE, &result );
#else
	trace::line(from, to, MASK_PLAYERSOLID_BRUSHONLY, nullptr, COLLISION_GROUP_NONE, result);
#endif
	extern IPhysicsSurfaceProps *physprops;
	if (result.fraction != 1.0f
		&& (physprops->GetSurfaceData( result.surface.surfaceProps )->game.climbable != 0
			|| (result.contents & CONTENTS_LADDER) != 0) )
	{
		m_normal = result.plane.normal;
	}
}

static void DrawAbsBoxOverlay(edict_t *edict)
{
	int red = 0;
	int green = 200;

	if (edict)
	{
		// Surrounding boxes are axially aligned, so ignore angles
		Vector vecSurroundMins, vecSurroundMaxs;
		edict->GetCollideable()->WorldSpaceSurroundingBounds( &vecSurroundMins, &vecSurroundMaxs );
		Vector center = 0.5f * (vecSurroundMins + vecSurroundMaxs);
		Vector extents = vecSurroundMaxs - center;
		debugoverlay->AddBoxOverlay(center, -extents, extents, QAngle(0,0,0), red, green, 0, 0 ,0);
	}
}

//--------------------------------------------------------------------------------------------------------------
void CNavLadder::DrawLadder( bool isSelected,  bool isMarked, bool isEdit ) const
{

	edict_t* ent = UTIL_GetListenServerEnt();
	extern IServerGameClients* gameclients;
	if (ent == NULL)
		return;

	Vector eye;
	gameclients->ClientEarPosition(ent, &eye);
	Vector2D eyeDir( eye.x - m_bottom.x, eye.y - m_bottom.y );
	eyeDir.NormalizeInPlace();
	bool isFront = DotProduct2D( eyeDir, GetNormal().AsVector2D() ) > 0;

	if ( isEdit )
	{
		isSelected = isMarked = false;
		isFront = true;
	}

	// Highlight ladder entity ------------------------------------------------
	edict_t* ladderEntity = m_ladderEntity.ToEdict();
	if (ladderEntity)
	{
		DrawAbsBoxOverlay(ladderEntity);
	}

	// Draw 'ladder' lines ----------------------------------------------------
	NavEditColor ladderColor = NavNormalColor;
	if ( isFront )
	{
		if ( isMarked )
		{
			ladderColor = NavMarkedColor;
		}
		else if ( isSelected )
		{
			ladderColor = NavSelectedColor;
		}
		else
		{
			ladderColor = NavSamePlaceColor;
		}
	}
	else if ( isMarked )
	{
		ladderColor = NavMarkedColor;
	}
	else if ( isSelected )
	{
		ladderColor = NavSelectedColor;
	}

	Vector right(0, 0, 0), up( 0, 0, 0 );
	VectorVectors( GetNormal(), right, up );
	if ( up.z <= 0.0f )
	{
		AssertMsg( false, "A nav ladder has an invalid normal" );
		up.Init( 0, 0, 1 );
	}

	right *= m_width * 0.5f;

	Vector bottomLeft = m_bottom - right;
	Vector bottomRight = m_bottom + right;
	Vector topLeft = m_top - right;
	Vector topRight = m_top + right;

	int bgcolor[4];
	if ( 4 == sscanf(sm_nav_area_bgcolor.GetString(), "%d %d %d %d", &(bgcolor[0]), &(bgcolor[1]), &(bgcolor[2]), &(bgcolor[3]) ) )
	{
		for ( int i=0; i<4; ++i )
			bgcolor[i] = clamp( bgcolor[i], 0, 255 );

		if ( bgcolor[3] > 0 )
		{
			Vector offset( 0, 0, 0 );
			AddDirectionVector( &offset, OppositeDirection( m_dir ), 1 );
			debugoverlay->AddTriangleOverlay( topLeft+offset, topRight+offset, bottomRight+offset, bgcolor[0], bgcolor[1], bgcolor[2], bgcolor[3], true, 0.15f );
			debugoverlay->AddTriangleOverlay( bottomRight+offset, bottomLeft+offset, topLeft+offset, bgcolor[0], bgcolor[1], bgcolor[2], bgcolor[3], true, 0.15f );
		}
	}

	NavDrawLine( topLeft, bottomLeft, ladderColor );
	NavDrawLine( topRight, bottomRight, ladderColor );

	while ( bottomRight.z < topRight.z )
	{
		NavDrawLine( bottomRight, bottomLeft, ladderColor );
		bottomRight += up * (navgenparams->generation_step_size/2);
		bottomLeft += up * (navgenparams->generation_step_size/2);
	}

	if (m_ladderType == USEABLE_LADDER)
	{
		NDebugOverlay::Cross3D(m_useableOrigin, 12.0f, 0, 255, 255, true, NDEBUG_PERSIST_FOR_ONE_TICK);
	}

	NDebugOverlay::Cross3D(m_top + (m_normal * 24.0f), 12.0f, 255, 0, 0, true, NDEBUG_PERSIST_FOR_ONE_TICK);
	NDebugOverlay::Cross3D(m_bottom + (m_normal * 24.0f), 12.0f, 0, 255, 0, true, NDEBUG_PERSIST_FOR_ONE_TICK);
	NDebugOverlay::Text(m_top, true, NDEBUG_PERSIST_FOR_ONE_TICK, "Ladder #%i", m_id);

	// Draw connector lines ---------------------------------------------------
	if ( !isEdit )
	{
		Vector bottom = m_bottom;
		Vector top = m_top;

		NavDrawLine(top, bottom, NavConnectedTwoWaysColor);

		for (auto& connect : m_connections)
		{
			CNavArea* area = connect.GetConnectedArea();

			if (connect.IsConnectedToLadderTop())
			{
				NavDrawLine(connect.point, area->GetCenter(), area->IsConnected(this, LADDER_DOWN) ? NavConnectedTwoWaysColor : NavConnectedOneWayColor);
			}
			else
			{
				NavDrawLine(connect.point, area->GetCenter(), area->IsConnected(this, LADDER_UP) ? NavConnectedTwoWaysColor : NavConnectedOneWayColor);
			}
		}
	}
}


//--------------------------------------------------------------------------------------------------------------
void CNavLadder::DrawConnectedAreas( bool isEdit )
{
	for (auto& connect : m_connections)
	{
		connect.GetConnectedArea()->Draw();
		connect.GetConnectedArea()->DrawHidingSpots();
	}
}

//--------------------------------------------------------------------------------------------------------------
bool CNavLadder::IsUsableByTeam(int teamNumber) const
{
	CBaseEntity* pEntity = GetLadderEntity();

	if (pEntity == nullptr)
	{
		return true;
	}

	entities::HBaseEntity be(pEntity);
	const int ladderTeam = be.GetTeam();
	return (ladderTeam == teamNumber || ladderTeam == TEAM_UNASSIGNED);
}

//--------------------------------------------------------------------------------------------------------------
/**
 * invoked when a game round restarts
 */
void CNavLadder::OnRoundRestart( void )
{
	FindLadderEntity();
}


void CNavLadder::BuildUseableLadder(CBaseEntity* ladder)
{
	const Vector& origin = UtilHelpers::getEntityOrigin(ladder);

	m_ladderType = USEABLE_LADDER;
	m_useableOrigin = origin;
	m_ladderEntity = ladder;

	entities::HBaseEntity ent{ ladder };

	Vector topPosition, bottomPosition;
	Vector* top = entprops->GetPointerToEntData<Vector>(ladder, Prop_Send, "m_vecPlayerMountPositionTop");
	Vector* bottom = entprops->GetPointerToEntData<Vector>(ladder, Prop_Send, "m_vecPlayerMountPositionBottom");

	ent.ComputeAbsPosition((*top + origin), &topPosition);
	ent.ComputeAbsPosition((*bottom + origin), &bottomPosition);

	m_top = topPosition;
	m_bottom = bottomPosition;
	m_top.z += navgenparams->step_height;
	m_bottom.z += (navgenparams->step_height / 2.0f);
	m_length = fabsf(topPosition.z - bottomPosition.z);
	m_width = navgenparams->half_human_width;
	UpdateUseableLadderDir(NORTH);

	ConnectGeneratedLadder(10.0f);
}

void CNavLadder::UpdateUseableLadderDir(NavDirType dir)
{
	if (m_ladderType != USEABLE_LADDER)
		return;

	m_dir = dir;
	m_normal.Init(0.0f, 0.0f, 0.0f);
	AddDirectionVector(&m_normal, OppositeDirection(m_dir), 1.0f);

	NDebugOverlay::HorzArrow(m_useableOrigin, m_useableOrigin + (m_normal * 256.0f), 6.0f, 255, 0, 0, 255, true, 20.0f);
}

const LadderToAreaConnection* CNavLadder::GetConnectionToArea(const CNavArea* area) const
{
	for (auto& connect : m_connections)
	{
		if (connect.GetConnectedArea() == area)
		{
			return &connect;
		}
	}

	return nullptr;
}

void CNavLadder::GetConnectedAreas(std::vector<CNavArea*>& topAreas, std::vector<CNavArea*>& bottomAreas) const
{
	topAreas.clear();
	bottomAreas.clear();

	for (auto& connect : m_connections)
	{
		if (connect.IsConnectedToLadderTop())
		{
			topAreas.push_back(connect.GetConnectedArea());
		}
		else
		{
			bottomAreas.push_back(connect.GetConnectedArea());
		}
	}
}

//--------------------------------------------------------------------------------------------------------------
void CNavLadder::FindLadderEntity( void )
{
	if (this->m_ladderType == USEABLE_LADDER)
	{
		CBaseEntity* ladder = nullptr;
		float distance = 99999999.0f;
		auto functor = [&ladder, &distance, this](int index, edict_t* edict, CBaseEntity* entity) {
			if (edict == nullptr || entity == nullptr)
			{
				return true; // exit early but keep searching
			}

			Vector origin = edict->GetCollideable()->GetCollisionOrigin();

			float current = (this->GetUseableLadderEntityOrigin() - origin).Length();

			if (current < distance)
			{
				ladder = entity;
				distance = current;
			}

			return true;
		};

		UtilHelpers::ForEachEntityOfClassname("func_useableladder", functor);

		if (ladder != nullptr)
		{
			m_ladderEntity = ladder;

#ifdef EXT_DEBUG
			smutils->LogMessage(myself, "Nav Ladder <Useable> #%i found ladder entity %i", m_id, gamehelpers->EntityToBCompatRef(ladder));
#endif // EXT_DEBUG

		}
		else
		{
			Warning("Useable Nav Ladder #%i could not find a matching func_useableladder entity!\n", m_id);
		}
	}
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Save a navigation ladder to the opened binary stream
 */
void CNavLadder::Save(std::fstream& filestream, uint32_t version)
{
	// save ID
	filestream.write(reinterpret_cast<char*>(&m_id), sizeof(unsigned int));

	// save extent of ladder
	filestream.write(reinterpret_cast<char*>(&m_width), sizeof(float));

	// save top endpoint of ladder
	filestream.write(reinterpret_cast<char*>(&m_top), sizeof(Vector));

	// save bottom endpoint of ladder
	filestream.write(reinterpret_cast<char*>(&m_bottom), sizeof(Vector));

	// save useable ladder entity origin
	filestream.write(reinterpret_cast<char*>(&m_useableOrigin), sizeof(Vector));

	// save ladder length
	filestream.write(reinterpret_cast<char*>(&m_length), sizeof(float));

	// save direction
	filestream.write(reinterpret_cast<char*>(&m_dir), sizeof(NavDirType));

	// save ladder type
	filestream.write(reinterpret_cast<char*>(&m_ladderType), sizeof(LadderType));

	// save IDs of connecting areas
	std::uint8_t count = 0;
	std::size_t size = m_connections.size();

	if (size >= 254U)
	{
		count = 254U; // only save up to 254 connections
	}
	else
	{
		count = static_cast<std::uint8_t>(size);
	}

	// save vector size
	filestream.write(reinterpret_cast<char*>(&count), sizeof(std::uint8_t));

	for (auto& connect : m_connections)
	{
		// area ID
		unsigned int id = connect.GetConnectedArea()->GetID();
		filestream.write(reinterpret_cast<char*>(&id), sizeof(unsigned int));

		// data
		filestream.write(reinterpret_cast<char*>(&connect.bottom), sizeof(bool));
		filestream.write(reinterpret_cast<char*>(&connect.point), sizeof(Vector));
	}
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Load a navigation ladder from the opened binary stream
 */
void CNavLadder::Load(CNavMesh* TheNavMesh, std::fstream& filestream, uint32_t version)
{
	// load ID
	filestream.read(reinterpret_cast<char*>(&m_id), sizeof(unsigned int));

	// load extent of ladder
	filestream.read(reinterpret_cast<char*>(&m_width), sizeof(float));

	// load top endpoint of ladder
	filestream.read(reinterpret_cast<char*>(&m_top), sizeof(Vector));

	// load bottom endpoint of ladder
	filestream.read(reinterpret_cast<char*>(&m_bottom), sizeof(Vector));

	// save useable ladder entity origin
	filestream.read(reinterpret_cast<char*>(&m_useableOrigin), sizeof(Vector));

	// load ladder length
	filestream.read(reinterpret_cast<char*>(&m_length), sizeof(float));

	// load direction
	filestream.read(reinterpret_cast<char*>(&m_dir), sizeof(NavDirType));

	// load ladder type
	filestream.read(reinterpret_cast<char*>(&m_ladderType), sizeof(LadderType));

	// load IDs of connecting areas
	
	std::uint8_t count = 0;
	
	// load vector size
	filestream.read(reinterpret_cast<char*>(&count), sizeof(std::uint8_t));

	for (std::uint8_t i = 0; i < count; i++)
	{
		auto& connect = m_connections.emplace_back();

		// area ID
		unsigned int id = 0;
		filestream.read(reinterpret_cast<char*>(&id), sizeof(unsigned int));
		connect.connect = id;

		// data
		filestream.read(reinterpret_cast<char*>(&connect.bottom), sizeof(bool));
		filestream.read(reinterpret_cast<char*>(&connect.point), sizeof(Vector));
	}
}

void CNavLadder::PostLoad(CNavMesh* TheNavMesh, uint32_t version)
{
	m_topCount = 0;
	m_bottomCount = 0;

	for (auto& connect : m_connections)
	{
		connect.PostLoad(); // convert ID to pointers

		if (connect.IsConnectedToLadderTop())
		{
			m_topCount++;
		}
		else
		{
			m_bottomCount++;
		}
	}

	FindLadderEntity();

	if (m_ladderType == USEABLE_LADDER)
	{
		m_normal.Init(0.0f, 0.0f, 0.0f);
		AddDirectionVector(&m_normal, OppositeDirection(m_dir), 1.0f);
	}
	else
	{
		// regenerate the surface normal
		this->SetDir(m_dir);
	}
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Functor returns true if ladder is free, or false if someone is on the ladder
 */
class IsLadderFreeFunctor
{
public:
	IsLadderFreeFunctor( const CNavLadder *ladder, const edict_t *ignore )
	{
		m_ladder = ladder;
		m_ignore = ignore;
	}

	bool operator() ( edict_t *entity )
	{
		if (entity == m_ignore)
			return true;

		CBaseExtPlayer player(entity);

		if (!player.IsOnLadder())
			return true;

		// player is on a ladder - is it this one?
		const Vector& feet = player.GetAbsOrigin();

		if (feet.z > m_ladder->m_top.z + navgenparams->human_height)
			return true;

		if (feet.z + navgenparams->human_height < m_ladder->m_bottom.z - navgenparams->human_height)
			return true;

		Vector2D away(m_ladder->m_bottom.x - feet.x, m_ladder->m_bottom.y - feet.y);
		const float onLadderRange = 50.0f;
		return away.IsLengthGreaterThan(onLadderRange);
	}

	const CNavLadder *m_ladder;
	const edict_t *m_ignore;
};

//--------------------------------------------------------------------------------------------------------------
/**
 * DEPRECATED: Use CollectPlayers() instead.
 * Iterate over all active players in the game, invoking functor on each.
 * If functor returns false, stop iteration and return false.
 */
template < typename Functor >
bool ForEachPlayer( Functor &func )
{
	for( int i=1; i<=gpGlobals->maxClients; ++i )
	{
		edict_t* ent = gamehelpers->EdictOfIndex(i);
		IPlayerInfo* player = playerinfomanager->GetPlayerInfo(ent);
		if (player == NULL || (!player->IsPlayer() && !player->IsConnected()) )
			continue;
		if (!func( ent ))
			return false;
	}
	return true;
}



//--------------------------------------------------------------------------------------------------------------
/**
 * Return true if someone is on this ladder
 */
bool CNavLadder::IsInUse( const edict_t *ignore ) const
{
	IsLadderFreeFunctor	isLadderFree( this, ignore );
	return !ForEachPlayer( isLadderFree );
}

//--------------------------------------------------------------------------------------------------------------
Vector CNavLadder::GetPosAtHeight( float height ) const
{
	if ( height < m_bottom.z )
	{
		return m_bottom;
	}

	if ( height > m_top.z || m_top.z == m_bottom.z )
	{
		return m_top;
	}

	float percent = ( height - m_bottom.z ) / ( m_top.z - m_bottom.z );

	return m_top * percent + m_bottom * ( 1.0f - percent );
}

//--------------------------------------------------------------------------------------------------------------

void LadderToAreaConnection::PostLoad()
{
	unsigned int id = std::get<unsigned int>(connect);
	CNavArea* area = TheNavMesh->GetNavAreaByID(id);

	if (area == nullptr)
	{
		smutils->LogError(myself, "LadderToAreaConnection::PostLoad Failed to convert Area ID #%i to a CNavArea object!", id);
	}

	connect = area;
}
