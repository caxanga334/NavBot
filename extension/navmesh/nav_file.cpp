//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
// nav_file.cpp
// Reading and writing nav files
// Author: Michael S. Booth (mike@turtlerockstudios.com), January-September 2003

#include <string>
#include <filesystem>
#include <fstream>

#include "extension.h"

#include "nav_mesh.h"
#include "nav_area.h"
#include "datacache/imdlcache.h"

#ifdef TERROR
#include "func_elevator.h"
#endif

#include "tier1/lzmaDecoder.h"

#ifdef CSTRIKE_DLL
#include "cs_shareddefs.h"
#include "nav_pathfind.h"
#include "cs_nav_area.h"
#endif
#include <utlbuffer.h>
#include <filesystem.h>
#include <eiface.h>


//--------------------------------------------------------------------------------------------------------------
/// The current version of the nav file format

/// IMPORTANT: If this version changes, the swap function in makegamedata 
/// must be updated to match. If not, this will break the Xbox 360.
// TODO: Was changed from 15, update when latest 360 code is integrated (MSB 5/5/09)

// Reset to 1 for SMNav
constexpr int NavCurrentVersion = 1;

extern IFileSystem *filesystem;
extern IVEngineServer* engine;
extern CGlobalVars *gpGlobals;
extern NavAreaVector TheNavAreas;

// Header for nav mesh files, changing this will break loading of nav files
class CSMNavFileHeader
{
public:
	unsigned int magic_number;
	unsigned int version;
	unsigned int subversion;
	unsigned int number_of_areas;
	unsigned int number_of_ladders;
};

static_assert(sizeof(CSMNavFileHeader) == 20U, "CSMNavFileHeader size changed! Do we really want this?");

//--------------------------------------------------------------------------------------------------------------
//
// The 'place directory' is used to save and load places from
// nav files in a size-efficient manner that also allows for the 
// order of the place ID's to change without invalidating the
// nav files.
//
// The place directory is stored in the nav file as a list of 
// place name strings.  Each nav area then contains an index
// into that directory, or zero if no place has been assigned to 
// that area.
//
PlaceDirectory::PlaceDirectory( void )
{
	Reset();
}

void PlaceDirectory::Reset( void )
{
	m_directory.clear();
	m_hasUnnamedAreas = false;
}

/// return true if this place is already in the directory
bool PlaceDirectory::IsKnown( Place place ) const
{
	auto start = m_directory.begin();
	auto end = m_directory.end();
	auto pos = std::find(start, end, place);

	return pos != end;
}

/// return the directory index corresponding to this Place (0 = no entry)
PlaceDirectory::IndexType PlaceDirectory::GetIndex( Place place ) const
{
	if (place == UNDEFINED_PLACE)
		return 0;

	auto start = m_directory.begin();
	auto end = m_directory.end();
	auto pos = std::find(start, end, place);

	if (pos == end)
	{
		return 0;
	}

	auto i = *pos;

	if (i < 0)
	{
		AssertMsg( false, "PlaceDirectory::GetIndex failure" );
		return 0;
	}

	return (IndexType)(i+1);
}

/// add the place to the directory if not already known
void PlaceDirectory::AddPlace( Place place )
{
	if (place == UNDEFINED_PLACE)
	{
		m_hasUnnamedAreas = true;
		return;
	}

	Assert( place < 1000 );

	if (IsKnown( place ))
		return;

	m_directory.push_back( place );
}

/// given an index, return the Place
Place PlaceDirectory::IndexToPlace( IndexType entry ) const
{
	if (entry == 0)
		return UNDEFINED_PLACE;

	int i = entry-1;

	if (i >= m_directory.size())
	{
		AssertMsg( false, "PlaceDirectory::IndexToPlace: Invalid entry" );
		return UNDEFINED_PLACE;
	}

	return m_directory[ i ];
}

/// store the directory
void PlaceDirectory::Save(std::fstream& file)
{
	// store number of entries in directory
	size_t count = m_directory.size();
	file.write(reinterpret_cast<char*>(&count), sizeof(size_t));

	// store entries

	for (auto& place : m_directory)
	{
		char buffer[PLACE_DIRECTORY_SAVE_BUFFER_SIZE]; // fixed size buffer, should be enough
		const char* placeName = TheNavMesh->PlaceToName(place);
		std::snprintf(buffer, PLACE_DIRECTORY_SAVE_BUFFER_SIZE, "%s", placeName);

		file.write(buffer, PLACE_DIRECTORY_SAVE_BUFFER_SIZE);
	}

	file.write(reinterpret_cast<char*>(m_hasUnnamedAreas), sizeof(bool));
}

/// load the directory
void PlaceDirectory::Load(std::fstream& file, const int version)
{
	// read number of entries
	size_t count = 0;
	file.read(reinterpret_cast<char*>(count), sizeof(size_t));

	m_directory.clear();

	// read each entry
	char placeName[PLACE_DIRECTORY_SAVE_BUFFER_SIZE];

	for (size_t i = 0; i < count; i++)
	{
		file.read(placeName, PLACE_DIRECTORY_SAVE_BUFFER_SIZE);

		Place place = TheNavMesh->NameToPlace(placeName);

		if (place == UNDEFINED_PLACE)
		{
			smutils->LogMessage(myself, "Warning: NavMesh place %s is undefined?", placeName);
		}

		AddPlace(place);
	}

	file.read(reinterpret_cast<char*>(m_hasUnnamedAreas), sizeof(bool));
}



PlaceDirectory placeDirectory;

#if defined( _X360 )
	#define FORMAT_BSPFILE "maps\\%s.360.bsp"
	#define FORMAT_NAVFILE "maps\\%s.360.nav"
#else
	#define FORMAT_BSPFILE "maps\\%s.bsp"
	#define FORMAT_NAVFILE "maps\\%s.nav"
#endif

//--------------------------------------------------------------------------------------------------------------
/**
 * Replace extension with "bsp"
 */
char *GetBspFilename( const char *navFilename )
{
	static char bspFilename[256];

	Q_snprintf( bspFilename, sizeof( bspFilename ), FORMAT_BSPFILE, STRING( gpGlobals->mapname ) );

	int len = strlen( bspFilename );
	if (len < 3)
		return NULL;

	bspFilename[ len-3 ] = 'b';
	bspFilename[ len-2 ] = 's';
	bspFilename[ len-1 ] = 'p';

	return bspFilename;
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Save a navigation area to the opened binary stream
 */
void CNavArea::Save(std::fstream& file, unsigned int version)
{
	// save ID
	file.write(reinterpret_cast<char*>(&m_id), sizeof(unsigned int));

	// save attribute flags
	file.write(reinterpret_cast<char*>(m_attributeFlags), sizeof(int));

	// save extent of area
	// TO-DO: VALVE uses 3 * sizeof float instead of sizeof Vector. Why?
	file.write(reinterpret_cast<char*>(&m_nwCorner), sizeof(Vector));
	file.write(reinterpret_cast<char*>(&m_seCorner), sizeof(Vector));

	// save heights of implicit corners
	file.write(reinterpret_cast<char*>(&m_neZ), sizeof(float));
	file.write(reinterpret_cast<char*>(&m_swZ), sizeof(float));

	// save connections to adjacent areas
	// in the enum order NORTH, EAST, SOUTH, WEST
	for( int d=0; d<NUM_DIRECTIONS; d++ )
	{
		// save number of connections for this direction
		size_t count = m_connect[d].size();
		file.write(reinterpret_cast<char*>(&count), sizeof(size_t));

		for (auto& connect : m_connect[d])
		{
			auto id = connect.area->m_id;
			file.write(reinterpret_cast<char*>(&id), sizeof(unsigned int));
		}
	}

	//
	// Store hiding spots for this area
	//
	size_t count;
	if (m_hidingSpots.size() > 255)
	{
		count = 255;
		smutils->LogError(myself, "Warning: NavArea #%d: Truncated hiding spot list to 255", m_id);
	}
	else
	{
		count = m_hidingSpots.size();
	}

	file.write(reinterpret_cast<char*>(&count), sizeof(size_t));

	// store HidingSpot objects
	unsigned int saveCount = 0;

	for (auto spot : m_hidingSpots)
	{
		spot->Save(file, version);

		if (++saveCount == count)
			break;
	}

	//
	// Save encounter spots for this area
	//
	{
		// save number of encounter paths for this area
		size_t count = m_spotEncounters.size();
		file.write(reinterpret_cast<char*>(&count), sizeof(size_t));

		for (auto e : m_spotEncounters)
		{
			if (e->from.area)
			{
				auto id = e->from.area->m_id;
				file.write(reinterpret_cast<char*>(&id), sizeof(unsigned int));
			}
			else
			{
				unsigned int id = 0;
				file.write(reinterpret_cast<char*>(&id), sizeof(unsigned int));
			}

			uint8 dir = static_cast<uint8>(e->fromDir);
			file.write(reinterpret_cast<char*>(&dir), sizeof(uint8));

			if (e->to.area)
			{
				auto id = e->to.area->m_id;
				file.write(reinterpret_cast<char*>(&id), sizeof(unsigned int));
			}
			else
			{
				unsigned int id = 0;
				file.write(reinterpret_cast<char*>(&id), sizeof(unsigned int));
			}

			dir = static_cast<uint8>(e->toDir);
			file.write(reinterpret_cast<char*>(&dir), sizeof(uint8));

			size_t spotCount = e->spots.size();

			if (spotCount > 255)
			{
				spotCount = 255;
				smutils->LogError(myself, "Warning: NavArea #%d: Truncated encounter spot list to 255", m_id);
			}

			file.write(reinterpret_cast<char*>(&spotCount), sizeof(size_t));

			saveCount = 0;
			for (auto& order : e->spots)
			{
				// order->spot may be NULL if we've loaded a nav mesh that has been edited but not re-analyzed
				unsigned int id = order.spot ? order.spot->GetID() : 0;
				file.write(reinterpret_cast<char*>(&id), sizeof(unsigned int));
				float t = order.t;
				file.write(reinterpret_cast<char*>(&t), sizeof(float));

				if (++saveCount == spotCount)
					break;
			}
		}
	}

	// store place dictionary entry
	PlaceDirectory::IndexType entry = placeDirectory.GetIndex( GetPlace() );
	file.write(reinterpret_cast<char*>(&entry), sizeof(entry));

	// write out ladder info
	int i;
	for ( i=0; i<CNavLadder::NUM_LADDER_DIRECTIONS; ++i )
	{
		// save number of encounter paths for this area
		size_t count = m_ladder[i].size();
		file.write(reinterpret_cast<char*>(&count), sizeof(size_t));

		for (const auto& ladder : m_ladder[i])
		{
			auto id = ladder.ladder->GetID();
			file.write(reinterpret_cast<char*>(&id), sizeof(id));
		}
	}

	file.write(reinterpret_cast<char*>(m_earliestOccupyTime), sizeof(m_earliestOccupyTime));
	file.write(reinterpret_cast<char*>(m_lightIntensity), sizeof(m_lightIntensity));

	// save visible area set
	size_t visibleAreaCount = m_potentiallyVisibleAreas.size();
	file.write(reinterpret_cast<char*>(visibleAreaCount), sizeof(size_t));

	for (auto& vit : m_potentiallyVisibleAreas)
	{
		CNavArea* area = vit.area;
		unsigned int id = 0;

		if (area) { id = area->GetID(); }

		file.write(reinterpret_cast<char*>(&id), sizeof(unsigned int));
		file.write(reinterpret_cast<char*>(&vit.attributes), sizeof(unsigned char));
	}

	// store area we inherit visibility from
	unsigned int id = ( m_inheritVisibilityFrom.area ) ? m_inheritVisibilityFrom.area->GetID() : 0;
	file.write(reinterpret_cast<char*>(&id), sizeof(unsigned int));
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Load a navigation area from the file
 */
NavErrorType CNavArea::Load(std::fstream& file, unsigned int version, unsigned int subVersion)
{
	// load ID
	file.read(reinterpret_cast<char*>(&m_id), sizeof(unsigned int));

	// update nextID to avoid collisions
	if (m_id >= m_nextID)
		m_nextID = m_id+1;

	file.read(reinterpret_cast<char*>(&m_attributeFlags), sizeof(int));

	// load extent of area
	file.read(reinterpret_cast<char*>(&m_nwCorner), sizeof(Vector));
	file.read(reinterpret_cast<char*>(&m_seCorner), sizeof(Vector));

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
		smutils->LogError(myself, "Degenerate Navigation Area #%d at setpos %3.2f %3.2f %3.2f", m_id, m_center.x, m_center.y, m_center.z);
	}

	// load heights of implicit corners

	file.read(reinterpret_cast<char*>(&m_neZ), sizeof(float));
	file.read(reinterpret_cast<char*>(&m_swZ), sizeof(float));

	CheckWaterLevel();

	// load connections (IDs) to adjacent areas
	// in the enum order NORTH, EAST, SOUTH, WEST
	for( int d=0; d<NUM_DIRECTIONS; d++ )
	{
		// load number of connections for this direction
		size_t count = 0;
		file.read(reinterpret_cast<char*>(&count), sizeof(size_t));

		m_connect[d].reserve(count);
		for(size_t i = 0; i < count; ++i)
		{
			NavConnect connect;
			unsigned int id = 0;
			file.read(reinterpret_cast<char*>(id), sizeof(unsigned int));
			connect.id = id;

			// don't allow self-referential connections
			if ( connect.id != m_id )
			{
				m_connect[d].push_back(connect);
			}
		}
	}

	//
	// Load hiding spots
	//

	// load number of hiding spots
	size_t hidingSpotCount = 0;
	file.read(reinterpret_cast<char*>(&hidingSpotCount), sizeof(size_t));

	// load HidingSpot objects for this area
	for( int h=0; h<hidingSpotCount; ++h )
	{
		// create new hiding spot and put on master list
		HidingSpot *spot = TheNavMesh->CreateHidingSpot();

		spot->Load(file, version);
			
		m_hidingSpots.push_back(spot);
	}

	//
	// Load encounter paths for this area
	//
	size_t count = 0;
	file.read(reinterpret_cast<char*>(&count), sizeof(size_t));

	for(size_t e = 0; e < count; ++e)
	{
		SpotEncounter *encounter = new SpotEncounter;

		file.read(reinterpret_cast<char*>(&encounter->from.id), sizeof(unsigned int));
		file.read(reinterpret_cast<char*>(&encounter->fromDir), sizeof(uint8));
		file.read(reinterpret_cast<char*>(&encounter->to.id), sizeof(unsigned int));
		file.read(reinterpret_cast<char*>(&encounter->toDir), sizeof(uint8));

		// read list of spots along this path
		size_t spotCount = 0;
		file.read(reinterpret_cast<char*>(spotCount), sizeof(size_t));
	
		SpotOrder order;
		for(size_t s = 0; s < spotCount; ++s)
		{
			file.read(reinterpret_cast<char*>(&order.id), sizeof(unsigned int));
			file.read(reinterpret_cast<char*>(&order.t), sizeof(float));
			encounter->spots.push_back(order);
		}

		m_spotEncounters.push_back(encounter);
	}

	//
	// Load Place data
	//
	PlaceDirectory::IndexType entry = 0;
	file.read(reinterpret_cast<char*>(&entry), sizeof(entry));

	// convert entry to actual Place
	SetPlace(placeDirectory.IndexToPlace(entry));

	// load ladder data
	for ( int dir=0; dir<CNavLadder::NUM_LADDER_DIRECTIONS; ++dir )
	{
		count = 0;
		file.read(reinterpret_cast<char*>(&count), sizeof(size_t));

		for(size_t i = 0; i < count; ++i)
		{
			NavLadderConnect connect;
			file.read(reinterpret_cast<char*>(&connect.id), sizeof(unsigned int));

			bool alreadyConnected = false;

			for (auto& ladder : m_ladder[dir])
			{
				if (ladder.id == connect.id)
				{
					alreadyConnected = true;
					break;
				}
			}

			if ( !alreadyConnected )
			{
				m_ladder[dir].push_back(connect);
			}
		}
	}

	file.read(reinterpret_cast<char*>(m_earliestOccupyTime), sizeof(m_earliestOccupyTime));
	file.read(reinterpret_cast<char*>(m_lightIntensity), sizeof(m_lightIntensity));

	// load visibility information
	size_t visibleAreaCount = 0;
	file.read(reinterpret_cast<char*>(&visibleAreaCount), sizeof(size_t));

	m_potentiallyVisibleAreas.reserve(visibleAreaCount);

	for (size_t i = 0; i < visibleAreaCount; i++)
	{
		AreaBindInfo info;
		file.read(reinterpret_cast<char*>(&info.id), sizeof(unsigned int));
		file.read(reinterpret_cast<char*>(&info.attributes), sizeof(unsigned char));
		m_potentiallyVisibleAreas.push_back(info);
	}

	// read area from which we inherit visibility
	file.read(reinterpret_cast<char*>(&m_inheritVisibilityFrom.id), sizeof(unsigned int));

	return NAV_OK;
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Convert loaded IDs to pointers
 * Make sure all IDs are converted, even if corrupt data is encountered.
 */
NavErrorType CNavArea::PostLoad( void )
{
	NavErrorType error = NAV_OK;

	for ( int dir=0; dir<CNavLadder::NUM_LADDER_DIRECTIONS; ++dir )
	{
		FOR_EACH_VEC( m_ladder[dir], it )
		{
			NavLadderConnect& connect = m_ladder[dir][it];

			unsigned int id = connect.id;

			if ( TheNavMesh->GetLadders().Find( connect.ladder ) == TheNavMesh->GetLadders().InvalidIndex() )
			{
				connect.ladder = TheNavMesh->GetLadderByID( id );
			}

			if (id && connect.ladder == NULL)
			{
				Msg( "CNavArea::PostLoad: Corrupt navigation ladder data. Cannot connect Navigation Areas.\n" );
				error = NAV_CORRUPT_DATA;
			}
		}
	}

	// connect areas together
	for( int d=0; d<NUM_DIRECTIONS; d++ )
	{
		FOR_EACH_VEC( m_connect[d], it )
		{
			NavConnect *connect = &m_connect[ d ][ it ];

			// convert connect ID into an actual area
			unsigned int id = connect->id;
			connect->area = TheNavMesh->GetNavAreaByID( id );
			if (id && connect->area == NULL)
			{
				Msg( "CNavArea::PostLoad: Corrupt navigation data. Cannot connect Navigation Areas.\n" );
				error = NAV_CORRUPT_DATA;
			}
			connect->length = ( connect->area->GetCenter() - GetCenter() ).Length();
		}
	}

	// resolve spot encounter IDs
	SpotEncounter *e;
	FOR_EACH_VEC( m_spotEncounters, it )
	{
		e = m_spotEncounters[ it ];

		e->from.area = TheNavMesh->GetNavAreaByID( e->from.id );
		if (e->from.area == NULL)
		{
			Msg( "CNavArea::PostLoad: Corrupt navigation data. Missing \"from\" Navigation Area for Encounter Spot.\n" );
			error = NAV_CORRUPT_DATA;
		}

		e->to.area = TheNavMesh->GetNavAreaByID( e->to.id );
		if (e->to.area == NULL)
		{
			Msg( "CNavArea::PostLoad: Corrupt navigation data. Missing \"to\" Navigation Area for Encounter Spot.\n" );
			error = NAV_CORRUPT_DATA;
		}

		if (e->from.area && e->to.area)
		{
			// compute path
			float halfWidth;
			ComputePortal( e->to.area, e->toDir, &e->path.to, &halfWidth );
			ComputePortal( e->from.area, e->fromDir, &e->path.from, &halfWidth );

			const float eyeHeight = HalfHumanHeight;
			e->path.from.z = e->from.area->GetZ( e->path.from ) + eyeHeight;
			e->path.to.z = e->to.area->GetZ( e->path.to ) + eyeHeight;
		}

		// resolve HidingSpot IDs
		FOR_EACH_VEC( e->spots, sit )
		{
			SpotOrder *order = &e->spots[ sit ];

			order->spot = GetHidingSpotByID( order->id );
			if (order->spot == NULL)
			{
				Msg( "CNavArea::PostLoad: Corrupt navigation data. Missing Hiding Spot\n" );
				error = NAV_CORRUPT_DATA;
			}
		}
	}

	// convert visible ID's to pointers to actual areas
	for ( int it=0; it<m_potentiallyVisibleAreas.Count(); ++it )
	{
		AreaBindInfo &info = m_potentiallyVisibleAreas[ it ];

		info.area = TheNavMesh->GetNavAreaByID( info.id );
		if ( info.area == NULL )
		{
			Warning( "Invalid area in visible set for area #%d\n", GetID() );
		}		
	}

	m_inheritVisibilityFrom.area = TheNavMesh->GetNavAreaByID( m_inheritVisibilityFrom.id );
	Assert( m_inheritVisibilityFrom.area != this );

	// remove any invalid areas from the list
	AreaBindInfo bad;
	bad.area = NULL;
	while( m_potentiallyVisibleAreas.FindAndRemove( bad ) );

	// func avoid/prefer attributes are controlled by func_nav_cost entities
	ClearAllNavCostEntities();

	return error;
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Compute travel distance along shortest path from startPos to goalPos. 
 * Return -1 if can't reach endPos from goalPos.
 */
template< typename CostFunctor >
float NavAreaTravelDistance( const Vector &startPos, const Vector &goalPos, CostFunctor &costFunc )
{
	CNavArea *startArea = TheNavMesh->GetNearestNavArea( startPos );
	if (startArea == NULL)
	{
		return -1.0f;
	}

	// compute path between areas using given cost heuristic
	CNavArea *goalArea = NULL;
	if (NavAreaBuildPath( startArea, NULL, &goalPos, costFunc, &goalArea ) == false)
	{
		return -1.0f;
	}

	// compute distance along path
	if (goalArea->GetParent() == NULL)
	{
		// both points are in the same area - return euclidean distance
		return (goalPos - startPos).Length();
	}
	else
	{
		CNavArea *area;
		float distance;

		// goalPos is assumed to be inside goalArea (or very close to it) - skip to next area
		area = goalArea->GetParent();
		distance = (goalPos - area->GetCenter()).Length();

		for( ; area->GetParent(); area = area->GetParent() )
		{
			distance += (area->GetCenter() - area->GetParent()->GetCenter()).Length();
		}

		// add in distance to startPos
		distance += (startPos - area->GetCenter()).Length();

		return distance;
	}
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Determine the earliest time this hiding spot can be reached by either team
 */
void CNavArea::ComputeEarliestOccupyTimes( void )
{
#ifdef CSTRIKE_DLL
	/// @todo Derive cstrike-specific navigation classes

	for( int i=0; i<MAX_NAV_TEAMS; ++i )
	{
		// no spot in the map should take longer than this to reach
		m_earliestOccupyTime[i] = 120.0f;
	}

	if (nav_quicksave.GetBool())
		return;

	// maximum player speed in units/second
	const float playerSpeed = 240.0f;

	ShortestPathCost cost;
	CBaseEntity *spot;

	// determine the shortest time it will take a Terrorist to reach this area
	int team = TEAM_TERRORIST % MAX_NAV_TEAMS;
	for( spot = gEntList.FindEntityByClassname( NULL, "info_player_terrorist" );
		 spot;
		 spot = gEntList.FindEntityByClassname( spot, "info_player_terrorist" ) )
	{
		float travelDistance = NavAreaTravelDistance( spot->GetAbsOrigin(), m_center, cost );
		if (travelDistance < 0.0f)
			continue;

		float travelTime = travelDistance / playerSpeed;
		if (travelTime < m_earliestOccupyTime[ team ])
		{
			m_earliestOccupyTime[ team ] = travelTime;
		}
	}


	// determine the shortest time it will take a CT to reach this area
	team = TEAM_CT % MAX_NAV_TEAMS;
	for( spot = gEntList.FindEntityByClassname( NULL, "info_player_counterterrorist" );
		 spot;
		 spot = gEntList.FindEntityByClassname( spot, "info_player_counterterrorist" ) )
	{
		float travelDistance = NavAreaTravelDistance( spot->GetAbsOrigin(), m_center, cost );
		if (travelDistance < 0.0f)
			continue;

		float travelTime = travelDistance / playerSpeed;
		if (travelTime < m_earliestOccupyTime[ team ])
		{
			m_earliestOccupyTime[ team ] = travelTime;
		}
	}

#else
	for( int i=0; i<MAX_NAV_TEAMS; ++i )
	{
		m_earliestOccupyTime[i] = 0.0f;
	}
#endif
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Determine if this area is a "battlefront" area - where two rushing teams first meet.
 */
void CNavMesh::ComputeBattlefrontAreas( void )
{
#if 0
#ifdef CSTRIKE_DLL
	ShortestPathCost cost;
	CBaseEntity *tSpawn, *ctSpawn;

	for( tSpawn = gEntList.FindEntityByClassname( NULL, "info_player_terrorist" );
		 tSpawn;
		 tSpawn = gEntList.FindEntityByClassname( tSpawn, "info_player_terrorist" ) )
	{
		CNavArea *tArea = TheNavMesh->GetNavArea( tSpawn->GetAbsOrigin() );
		if (tArea == NULL)
			continue;

		for( ctSpawn = gEntList.FindEntityByClassname( NULL, "info_player_counterterrorist" );
			 ctSpawn;
			 ctSpawn = gEntList.FindEntityByClassname( ctSpawn, "info_player_counterterrorist" ) )
		{
			CNavArea *ctArea = TheNavMesh->GetNavArea( ctSpawn->GetAbsOrigin() );

			if (ctArea == NULL)
				continue;

			if (tArea == ctArea)
			{
				m_isBattlefront = true;
				return;
			}

			// build path between these two spawn points - assume if path fails, it at least got close
			// (ie: imagine spawn points that you jump down from - can't path to)
			CNavArea *goalArea = NULL;
			NavAreaBuildPath( tArea, ctArea, NULL, cost, &goalArea );

			if (goalArea == NULL)
				continue;


/**
 * @todo Need to enumerate ALL paths between all pairs of spawn points to find all battlefront areas
 */

			// find the area with the earliest overlapping occupy times
			CNavArea *battlefront = NULL;
			float earliestTime = 999999.9f;

			const float epsilon = 1.0f;
			CNavArea *area;
			for( area = goalArea; area; area = area->GetParent() )
			{
				if (fabs(area->GetEarliestOccupyTime( TEAM_TERRORIST ) - area->GetEarliestOccupyTime( TEAM_CT )) < epsilon)
				{
				}
				
			}
		}
	}
#endif
#endif
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Return the filename for this map's "nav map" file
 */
const char *CNavMesh::GetFilename( void )
{
	// TO-DO: implement proper support for workshop maps
	// gpGlobals->mapname::workshop/ctf_harbine.ugc3067683041

	char filename[MAX_MAP_NAME + 1];
	auto map = gpGlobals->mapname.ToCStr();
	std::snprintf(filename, sizeof(filename), "%s.smnav", map);
	return filename;
}

//--------------------------------------------------------------------------------------------------------------


static void WarnIfMeshNeedsAnalysis( int version )
{
	// Quick check to warn about needing to analyze: nav_strip, nav_delete, etc set
	// every CNavArea's m_approachCount to 0, and delete their m_spotEncounterList.
	// So, if no area has either, odds are good we need an analyze.

	if ( version >= 1 )
	{
		if ( !TheNavMesh->IsAnalyzed() )
		{
			rootconsole->ConsolePrint("[SMNAV] The nav mesh needs a full nav_analyze\n");
			return;
		}
	}
#ifdef CSTRIKE_DLL
	else
	{
		bool hasApproachAreas = false;
		bool hasSpotEncounters = false;

		FOR_EACH_VEC( TheNavAreas, it )
		{
			CCSNavArea *area = dynamic_cast< CCSNavArea * >( TheNavAreas[ it ] );
			if ( area )
			{
				if ( area->GetApproachInfoCount() )
				{
					hasApproachAreas = true;
				}

				if ( area->GetSpotEncounterCount() )
				{
					hasSpotEncounters = true;
				}
			}
		}

		if ( !hasApproachAreas || !hasSpotEncounters )
		{
			Warning( "The nav mesh needs a full nav_analyze\n" );
		}
	}
#endif
}

/**
 * Store Navigation Mesh to a file
 */
bool CNavMesh::Save( void )
{
	WarnIfMeshNeedsAnalysis( NavCurrentVersion );

	const char *filename = GetFilename();
	if (filename == nullptr)
		return false;

	//
	// Store the NAV file
	//

	// get size of source bsp file for later (before we open the nav file for writing, in
	// case of failure)
	// TO-DO

	char fullpath[PLATFORM_MAX_PATH + 1];
	auto mod = smutils->GetGameFolderName();

	smutils->BuildPath(SourceMod::Path_SM, fullpath, sizeof(fullpath), "data/smnav/%s/%s", mod, filename);

	std::fstream file;

	// Open file for writing in binary mode and overwrite existing content
	file.open(fullpath, std::fstream::out | std::fstream::binary | std::fstream::trunc);

	CSMNavFileHeader header{};
	header.magic_number = NAV_MAGIC_NUMBER;
	header.version = NavCurrentVersion;
	header.subversion = GetSubVersionNumber();
	header.number_of_areas = static_cast<unsigned int>(TheNavAreas.size());
	header.number_of_ladders = static_cast<unsigned int>(m_ladders.size());

	file.write(reinterpret_cast<char*>(&header), sizeof(CSMNavFileHeader));

	// store "magic number" to help identify this kind of file

	// store version number of file
	// 1 = hiding spots as plain vector array
	// 2 = hiding spots as HidingSpot objects
	// 3 = Encounter spots use HidingSpot ID's instead of storing vector again
	// 4 = Includes size of source bsp file to verify nav data correlation
	// ---- Beta Release at V4 -----
	// 5 = Added Place info
	// ---- Conversion to Src ------
	// 6 = Added Ladder info
	// 7 = Areas store ladder ID's so ladders can have one-way connections
	// 8 = Added earliest occupy times (2 floats) to each area
	// 9 = Promoted CNavArea's attribute flags to a short
	// 10 - Added sub-version number to allow derived classes to have custom area data
	// 11 - Added light intensity to each area
	// 12 - Storing presence of unnamed areas in the PlaceDirectory
	// 13 - Widened NavArea attribute bits from unsigned short to int
	// 14 - Added a bool for if the nav needs analysis
	// 15 - removed approach areas
	// 16 - Added visibility data to the base mesh

	// The sub-version number is maintained and owned by classes derived from CNavMesh and CNavArea
	// and allows them to track their custom data just as we do at this top level
	
	// store the size of source bsp file in the nav file
	// so we can test if the bsp changed since the nav file was made
	// unsigned int bspSize = filesystem->Size( bspFilename );
	// DevMsg( "Size of bsp file '%s' is %u bytes.\n", bspFilename, bspSize );
	// TO-DO!
	// fileBuffer.PutUnsignedInt( bspSize );

	// Store the analysis state

	file.write(reinterpret_cast<char*>(&m_isAnalyzed), sizeof(bool));

	//
	// Build a directory of the Places in this map
	//
	placeDirectory.Reset();

	for (auto area : TheNavAreas)
	{
		Place place = area->GetPlace();
		placeDirectory.AddPlace(place);
	}

	placeDirectory.Save(file);

	SaveCustomDataPreArea(file);

	//
	// Store navigation areas
	//

	// Vector size moved to header

	for (auto area : TheNavAreas)
	{
		area->Save(file, NavCurrentVersion);
	}

	for (auto ladder : m_ladders)
	{
		ladder->Save(file, NavCurrentVersion);
	}
	
	//
	// Store derived class mesh info
	//
	SaveCustomData(file);

	file.close();

	auto navSize = std::filesystem::file_size(fullpath);

	smutils->LogMessage(myself, "Saved nav mesh file \"%s\", size %u", navSize);

	return true;
}


//--------------------------------------------------------------------------------------------------------------
static NavErrorType CheckNavFile( const char *bspFilename )
{
	if ( !bspFilename )
		return NAV_CANT_ACCESS_FILE;

	char baseName[256];
	Q_StripExtension(bspFilename,baseName,sizeof(baseName));
	char bspPathname[256];
	Q_snprintf(bspPathname,sizeof(bspPathname), FORMAT_BSPFILE, baseName);
	char filename[256];
	Q_snprintf(filename,sizeof(filename), FORMAT_NAVFILE, baseName);

	bool navIsInBsp = false;
	FileHandle_t file = filesystem->Open( filename, "rb", "MOD" );	// this ignores .nav files embedded in the .bsp ...
	if ( !file )
	{
		navIsInBsp = true;
		file = filesystem->Open( filename, "rb", "GAME" );	// ... and this looks for one if it's the only one around.
	}

	if (!file)
	{
		return NAV_CANT_ACCESS_FILE;
	}

	// check magic number
	int result;
	unsigned int magic;
	result = filesystem->Read( &magic, sizeof(unsigned int), file );
	if (!result || magic != NAV_MAGIC_NUMBER)
	{
		filesystem->Close( file );
		return NAV_INVALID_FILE;
	}

	// read file version number
	unsigned int version;
	result = filesystem->Read( &version, sizeof(unsigned int), file );
	if (!result || version > NavCurrentVersion || version < 4)
	{
		filesystem->Close( file );
		return NAV_BAD_FILE_VERSION;
	}

	// get size of source bsp file and verify that the bsp hasn't changed
	unsigned int saveBspSize;
	filesystem->Read( &saveBspSize, sizeof(unsigned int), file );

	// verify size
	unsigned int bspSize = filesystem->Size( bspPathname );

	if (bspSize != saveBspSize && !navIsInBsp)
	{
		return NAV_FILE_OUT_OF_DATE;
	}

	return NAV_OK;
}


//--------------------------------------------------------------------------------------------------------------
void CommandNavCheckFileConsistency( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	rootconsole->ConsolePrint("Command not implemented!\n");

	//FileFindHandle_t findHandle;
	//const char *bspFilename = filesystem->FindFirstEx( "maps/*.bsp", "MOD", &findHandle );
	//while ( bspFilename )
	//{
	//	switch ( CheckNavFile( bspFilename ) )
	//	{
	//	case NAV_CANT_ACCESS_FILE:
	//		Warning( "Missing nav file for %s\n", bspFilename );
	//		break;
	//	case NAV_INVALID_FILE:
	//		Warning( "Invalid nav file for %s\n", bspFilename );
	//		break;
	//	case NAV_BAD_FILE_VERSION:
	//		Warning( "Old nav file for %s\n", bspFilename );
	//		break;
	//	case NAV_FILE_OUT_OF_DATE:
	//		Warning( "The nav file for %s is built from an old version of the map\n", bspFilename );
	//		break;
	//	case NAV_OK:
	//		Msg( "The nav file for %s is up-to-date\n", bspFilename );
	//		break;
	//	}

	//	bspFilename = filesystem->FindNext( findHandle );
	//}
	//filesystem->FindClose( findHandle );
}
static ConCommand nav_check_file_consistency( "nav_check_file_consistency", CommandNavCheckFileConsistency, "Scans the maps directory and reports any missing/out-of-date navigation files.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
/**
 * Reads the used place names from the nav file (can be used to selectively precache before the nav is loaded)
 */
const std::vector<Place> *CNavMesh::GetPlacesFromNavFile( bool *hasUnnamedPlaces )
{
	return nullptr;

	/*
	* Not used by anything, not converting for now

	placeDirectory.Reset();
	// nav filename is derived from map filename
	char filename[256];
	Q_snprintf( filename, sizeof( filename ), FORMAT_NAVFILE, STRING( gpGlobals->mapname ) );

	CUtlBuffer fileBuffer( 4096, 1024*1024, CUtlBuffer::READ_ONLY );
	if ( !filesystem->ReadFile( filename, "GAME", fileBuffer )	// this ignores .nav files embedded in the .bsp ...
		&& !filesystem->ReadFile( filename, "BSP", fileBuffer ) )	// ... and this looks for one if it's the only one around.
	{
		return NULL;
	}
	
	if ( IsX360()
		// 360 has compressed NAVs
		&& CLZMA::IsCompressed( (unsigned char *)fileBuffer.Base() ) )
	{
		int originalSize = CLZMA::GetActualSize( (unsigned char *)fileBuffer.Base() );
		unsigned char *pOriginalData = new unsigned char[originalSize];
		CLZMA::Uncompress( (unsigned char *)fileBuffer.Base(), pOriginalData );
		fileBuffer.AssumeMemory( pOriginalData, originalSize, originalSize, CUtlBuffer::READ_ONLY );
	}

	// check magic number
	if ( fileBuffer.GetUnsignedInt() != NAV_MAGIC_NUMBER || !fileBuffer.IsValid() )
	{
		return NULL;	// Corrupt nav file?
	}

	// read file version number
	unsigned int version = fileBuffer.GetUnsignedInt();
	if ( !fileBuffer.IsValid() || version > NavCurrentVersion
			// Unknown nav file version
			|| version < 5 )
	{
		return NULL;	// Too old to have place names
	}

	unsigned int subVersion = 0;
	if ( version >= 10 )
	{
		subVersion = fileBuffer.GetUnsignedInt();
		if ( !fileBuffer.IsValid() )
		{
			return NULL;	// No sub-version
		}
	}

	fileBuffer.GetUnsignedInt();	// skip BSP file size
	if ( version >= 14 )
	{
		fileBuffer.GetUnsignedChar();	// skip m_isAnalyzed
	}

	placeDirectory.Load( fileBuffer, version );

	LoadCustomDataPreArea( fileBuffer, subVersion );

	if ( hasUnnamedPlaces )
	{
		*hasUnnamedPlaces = placeDirectory.HasUnnamedPlaces();
	}

	return placeDirectory.GetPlaces();

	*/
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Load AI navigation data from a file
 */
NavErrorType CNavMesh::Load( void )
{
	extern IMDLCache *mdlcache;
	MDLCACHE_CRITICAL_SECTION(); // TO-DO: Is this needed?

	// free previous navigation mesh data
	Reset();
	placeDirectory.Reset();
	// CNavVectorNoEditAllocator::Reset();

	// TODO: GameRules()->OnNavMeshLoad();

	CNavArea::m_nextID = 1;

	// nav filename is derived from map filename

	auto mapname = GetFilename();
	char fullpath[PLATFORM_MAX_PATH + 1];
	auto modfolder = g_pSM->GetGameFolderName();

	g_pSM->BuildPath(SourceMod::Path_SM, fullpath, sizeof(fullpath), "data/smnav/%s/%s", modfolder, mapname);

	//if ( IsX360()
	//	// 360 has compressed NAVs
	//	&&  CLZMA::IsCompressed( (unsigned char *)fileBuffer.Base() ) )
	//{
	//	int originalSize = CLZMA::GetActualSize( (unsigned char *)fileBuffer.Base() );
	//	unsigned char *pOriginalData = new unsigned char[originalSize];
	//	CLZMA::Uncompress( (unsigned char *)fileBuffer.Base(), pOriginalData );
	//	fileBuffer.AssumeMemory( pOriginalData, originalSize, originalSize, CUtlBuffer::READ_ONLY );
	//}

	if (std::filesystem::exists(fullpath) == false)
	{
		smutils->LogMessage(myself, "Failed to load navigation mesh file \"%s\". File does not exists!", fullpath);
		return NAV_CANT_ACCESS_FILE;
	}

	std::fstream file;

	file.open(fullpath, std::fstream::in | std::fstream::binary);

	if (file.is_open() == false)
	{
		smutils->LogError(myself, "Failed to load navigation mesh file \"%s\"!", fullpath);
		return NAV_CANT_ACCESS_FILE;
	}

	CSMNavFileHeader header{};

	file.read(reinterpret_cast<char*>(&header), sizeof(CSMNavFileHeader));

	if (header.magic_number != NAV_MAGIC_NUMBER)
	{
		file.close();
		smutils->LogError(myself, "Invalid navigation mesh file \"%s\"!", fullpath);
		return NAV_INVALID_FILE;
	}

	if (header.version > NavCurrentVersion)
	{
		file.close();
		smutils->LogError(myself, "Unknown navigation mesh file version \"%i\"!", header.version);
		return NAV_INVALID_FILE;
	}

	// TO-DO
	//if ( version >= 4 )
	//{
	//	// get size of source bsp file and verify that the bsp hasn't changed
	//	unsigned int saveBspSize = fileBuffer.GetUnsignedInt();

	//	// verify size
	//	char *bspFilename = GetBspFilename( filename );
	//	if ( bspFilename == NULL )
	//	{
	//		return NAV_INVALID_FILE;
	//	}

	//	if ( filesystem->Size( bspFilename ) != saveBspSize && !navIsInBsp )
	//	{
	//		if ( engine->IsDedicatedServer() )
	//		{
	//			// Warning doesn't print to the dedicated server console, so we'll use Msg instead
	//			DevMsg( "The Navigation Mesh was built using a different version of this map.\n" );
	//		}
	//		else
	//		{
	//			DevWarning( "The Navigation Mesh was built using a different version of this map.\n" );
	//		}
	//		m_isOutOfDate = true;
	//	}
	//}

	file.read(reinterpret_cast<char*>(&m_isAnalyzed), sizeof(bool));

	placeDirectory.Load(file, header.version);


	LoadCustomDataPreArea(file, header.subversion);

	if (header.number_of_areas == 0)
	{
		file.close();
		return NAV_INVALID_FILE;
	}

	Extent extent;
	extent.lo.x = 9999999999.9f;
	extent.lo.y = 9999999999.9f;
	extent.hi.x = -9999999999.9f;
	extent.hi.y = -9999999999.9f;

	// load the areas and compute total extent
	TheNavMesh->PreLoadAreas(header.number_of_areas);
	Extent areaExtent;
	for(int i = 0; i < header.number_of_areas; ++i)
	{
		CNavArea *area = TheNavMesh->CreateArea();
		area->Load(file, header.version, header.subversion);
		TheNavAreas.push_back(area);

		area->GetExtent( &areaExtent );

		if (areaExtent.lo.x < extent.lo.x)
			extent.lo.x = areaExtent.lo.x;
		if (areaExtent.lo.y < extent.lo.y)
			extent.lo.y = areaExtent.lo.y;
		if (areaExtent.hi.x > extent.hi.x)
			extent.hi.x = areaExtent.hi.x;
		if (areaExtent.hi.y > extent.hi.y)
			extent.hi.y = areaExtent.hi.y;
	}

	// add the areas to the grid
	AllocateGrid( extent.lo.x, extent.hi.x, extent.lo.y, extent.hi.y );

	for (auto area : TheNavAreas)
	{
		AddNavArea(area);
	}

	for (int i = 0; i < header.number_of_ladders; i++)
	{
		CNavLadder* ladder = new CNavLadder;
		ladder->Load(this, file, header.version);
		m_ladders.push_back(ladder);
	}

	// mark stairways (TODO: this can be removed once all maps are re-saved with this attribute in them)
	MarkStairAreas();

	//
	// Load derived class mesh info
	//
	LoadCustomData(file, header.subversion);

	//
	// Bind pointers, etc
	//
	NavErrorType loadResult = PostLoad(header.version);

	WarnIfMeshNeedsAnalysis(header.version);

	file.close();
	return loadResult;
}


struct OneWayLink_t
{
	CNavArea *destArea;
	CNavArea *area;
	int backD;

	static int Compare(const OneWayLink_t *lhs, const OneWayLink_t *rhs )
	{
		int result = ( lhs->destArea - rhs->destArea );
		return result != 0 ?  result : ( lhs->backD - rhs->backD );
	}
};

//--------------------------------------------------------------------------------------------------------------
/**
 * Invoked after all areas have been loaded - for pointer binding, etc
 */
NavErrorType CNavMesh::PostLoad( unsigned int version )
{
	// allow areas to connect to each other, etc

	for (auto area : TheNavAreas)
	{
		area->PostLoad();
	}

	extern HidingSpotVector TheHidingSpots;
	// allow hiding spots to compute information

	for (auto spot : TheHidingSpots)
	{
		spot->PostLoad();
	}

	// CSTRIKE stuff
	// ComputeBattlefrontAreas();
	
	//
	// Allow each nav area to know what other areas have one-way connections to it. Need to gather
	// then sort due to allocation restrictions on the 360
	//


	OneWayLink_t oneWayLink;
	std::vector<OneWayLink_t> oneWayLinks;
	oneWayLinks.reserve(512);

	for (auto area : TheNavAreas)
	{
		oneWayLink.area = area;

		for (int dir = 0; dir < NUM_DIRECTIONS; dir++)
		{
			auto connectList = oneWayLink.area->GetAdjacentAreas(static_cast<NavDirType>(dir));

			for (auto& navconn : *connectList)
			{
				oneWayLink.destArea = navconn.area;

				// if the area we connect to has no connection back to us, allow that area to remember us as an incoming connection
				oneWayLink.backD = OppositeDirection(static_cast<NavDirType>(dir));

				auto backConnectList = oneWayLink.destArea->GetAdjacentAreas(static_cast<NavDirType>(oneWayLink.backD));
				bool isOneWay = true;

				for (auto& backconn : *backConnectList)
				{
					if (backconn.area->GetID() == oneWayLink.area->GetID())
					{
						isOneWay = false;
						break;
					}
				}

				if (isOneWay)
				{
					oneWayLinks.push_back(oneWayLink);
				}
			}
		}
	}

	// TO-DO:
	// 
	//auto start = oneWayLinks.begin();
	//auto end = oneWayLinks.end();
	//std::sort(start, end, [](const OneWayLink_t& lhs, const OneWayLink_t& rhs) {
	//	
	//});

	//oneWayLinks.Sort( &OneWayLink_t::Compare );

	for (auto& owl : oneWayLinks)
	{
		owl.destArea->AddIncomingConnection(owl.area, static_cast<NavDirType>(owl.backD));
	}

	ValidateNavAreaConnections();

	// TERROR: loading into a map directly creates entities before the mesh is loaded.  Tell the preexisting
	// entities now that the mesh is loaded so they can update areas.

	for (auto it : m_avoidanceObstacles)
	{
		it->OnNavMeshLoaded();
	}

	// the Navigation Mesh has been successfully loaded
	m_isLoaded = true;
	
	return NAV_OK;
}
