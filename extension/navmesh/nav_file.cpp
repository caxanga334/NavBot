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

#include <cinttypes>
#include <memory>

#include "extension.h"
#include <manager.h>
#include <mods/basemod.h>

#include "nav_mesh.h"
#include "nav_area.h"
#include "nav_waypoint.h"
#include "nav_volume.h"
#include "nav_prereq.h"

#include "tier1/lzmaDecoder.h"

#include <utlbuffer.h>
#include <filesystem.h>
#include <eiface.h>

/*
* Reminders:
* size_t / std::size_t will change size between 32/64 bits. If saving a size_t, save as uint64_t!
*/


class NavMeshFileHeader
{
public:
	static constexpr auto NAV_FILE_HEADER = "NavBotNavMesh";

	NavMeshFileHeader(uint32_t sb) // for writing
	{
		memset(header, 0, sizeof(header));
		strcpy(header, NAV_FILE_HEADER);
		version = CNavMesh::NavMeshVersion;
		subversion = sb;
		magic = CNavMesh::NavMagicNumber;
	}

	NavMeshFileHeader() // for reading
	{
		memset(&header, 0, sizeof(header));
		version = 0;
		subversion = 0;
		magic = 0;
	}

	inline bool IsMagicValid() const { return magic == CNavMesh::NavMagicNumber; }
	inline bool IsVersionValid() const { return version <= CNavMesh::NavMeshVersion; }
	inline bool IsHeaderValid() const { return strcmp(header, NAV_FILE_HEADER) == 0; }
	inline bool IsSubVersionValid(uint32_t subv) const { return subversion <= subv; }

	char header[16];
	uint32_t version;
	uint32_t subversion;
	uint32_t magic;
};

static_assert(sizeof(NavMeshFileHeader) == 28U, "Changing this will invalidate all existing nav mesh files!");

/**
 * @brief Header that stores information about the current mod and map.
 */
class NavMeshInfoHeader
{
public:
	NavMeshInfoHeader()
	{
		memset(mapname, 0, sizeof(mapname));
		memset(modfolder, 0, sizeof(modfolder));
		memset(modname, 0, sizeof(modname));
		mapversion = 0;
	}

	inline void Init()
	{
		const char* map = STRING(gpGlobals->mapname);
		ke::SafeStrcpy(mapname, sizeof(mapname), map);
		const char* gamefolder = smutils->GetGameFolderName();
		ke::SafeStrcpy(modfolder, sizeof(modfolder), gamefolder);
		const char* mod = extmanager->GetMod()->GetModName();
		ke::SafeStrcpy(modname, sizeof(modname), mod);
		mapversion = gpGlobals->mapversion;
	}

	char mapname[128];
	char modfolder[64];
	char modname[128];
	int mapversion;
};

static_assert(sizeof(NavMeshInfoHeader) == 324U, "Changing this will invalidate all existing nav mesh files!");

//--------------------------------------------------------------------------------------------------------------
/// The current version of the nav file format



/// IMPORTANT: If this version changes, the swap function in makegamedata 
/// must be updated to match. If not, this will break the Xbox 360.
// TODO: Was changed from 15, update when latest 360 code is integrated (MSB 5/5/09)
constexpr int NavCurrentVersion = 17;

// Current version of the Sourcemod Nav Mesh
constexpr int SMNavVersion = 1;

extern IFileSystem *filesystem;
extern IVEngineServer* engine;
extern CGlobalVars *gpGlobals;
extern NavAreaVector TheNavAreas;

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

/// store the directory
void PlaceDirectory::Save(std::fstream& filestream)
{
	// store number of entries in directory
	uint64_t size = static_cast<uint64_t>(m_directory.size());
	filestream.write(reinterpret_cast<char*>(&size), sizeof(uint64_t));

	for (auto& place : m_directory)
	{
		auto name = TheNavMesh->GetPlaceName(place.second);
		uint64_t length = static_cast<uint64_t>(name->length() + 1U);
		filestream.write(reinterpret_cast<char*>(&length), sizeof(uint64_t));
		filestream.write(name->c_str(), length);
		IndexType entry = place.first;
		filestream.write(reinterpret_cast<char*>(&entry), sizeof(IndexType));
	}

	filestream.write(reinterpret_cast<char*>(&m_hasUnnamedAreas), sizeof(bool));
}

/// load the directory
void PlaceDirectory::Load(std::fstream& filestream, uint32_t version)
{
	// read number of entries
	uint64_t size = 0U;
	filestream.read(reinterpret_cast<char*>(&size), sizeof(uint64_t));

	m_directory.clear();

	for (uint64_t i = 0; i < size; i++)
	{
		char placeName[256];
		uint64_t length = 0U;
		IndexType entry = 0;
		filestream.read(reinterpret_cast<char*>(&length), sizeof(uint64_t));
		filestream.read(placeName, length);
		filestream.read(reinterpret_cast<char*>(&entry), sizeof(IndexType));

		std::string name(placeName);
		auto place = TheNavMesh->GetPlaceFromName(name);

		if (place == UNDEFINED_PLACE)
		{
			Warning("Warning: NavMesh place \"%s\" is undefined? \n", placeName);
		}

		LoadPlace(entry, place);
	}

	filestream.read(reinterpret_cast<char*>(&m_hasUnnamedAreas), sizeof(m_hasUnnamedAreas));
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

	size_t len = strlen( bspFilename );
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
void CNavArea::Save(std::fstream& filestream, uint32_t version)
{
	// save ID
	filestream.write(reinterpret_cast<char*>(&m_id), sizeof(unsigned int));

	// save attribute flags
	filestream.write(reinterpret_cast<char*>(&m_attributeFlags), sizeof(int));

	// save extent of area
	filestream.write(reinterpret_cast<char*>(&m_nwCorner), sizeof(Vector));
	filestream.write(reinterpret_cast<char*>(&m_seCorner), sizeof(Vector));
	// save heights of implicit corners
	filestream.write(reinterpret_cast<char*>(&m_neZ), sizeof(float));
	filestream.write(reinterpret_cast<char*>(&m_swZ), sizeof(float));

	// save connections to adjacent areas
	// in the enum order NORTH, EAST, SOUTH, WEST
	for (int d = 0; d < NUM_DIRECTIONS; d++)
	{
		int count = m_connect[d].Count();
		filestream.write(reinterpret_cast<char*>(&count), sizeof(int));

		FOR_EACH_VEC(m_connect[d], it)
		{
			NavConnect connect = m_connect[d][it];
			unsigned int id = connect.area->m_id;
			filestream.write(reinterpret_cast<char*>(&id), sizeof(unsigned int));
		}
	}

	//
	// Store hiding spots for this area
	//
	constexpr auto MAX_HIDING_SPOTS_TO_SAVE = 255;

	int count;
	if (m_hidingSpots.Count() > MAX_HIDING_SPOTS_TO_SAVE)
	{
		count = 255;
		Warning( "Warning: NavArea #%d: Truncated hiding spot list to %i\n", m_id, MAX_HIDING_SPOTS_TO_SAVE);
	}
	else
	{
		count = m_hidingSpots.Count();
	}
	filestream.write(reinterpret_cast<char*>(&count), sizeof(int));

	// store HidingSpot objects
	int saveCount = 0;
	FOR_EACH_VEC( m_hidingSpots, hit )
	{
		HidingSpot *spot = m_hidingSpots[ hit ];
		
		spot->Save(filestream, version);

		// overflow check
		if (++saveCount == count)
			break;
	}

	// store place dictionary entry
	PlaceDirectory::IndexType entry = placeDirectory.GetIndex( GetPlace() );
	filestream.write(reinterpret_cast<char*>(&entry), sizeof(PlaceDirectory::IndexType));

	// write out ladder info
	int i;
	for ( i=0; i<CNavLadder::NUM_LADDER_DIRECTIONS; ++i )
	{
		// save number of encounter paths for this area
		int count = m_ladder[i].Count();
		filestream.write(reinterpret_cast<char*>(&count), sizeof(int));

		NavLadderConnect ladder;
		FOR_EACH_VEC( m_ladder[i], it )
		{
			ladder = m_ladder[i][it];

			unsigned int id = ladder.ladder->GetID();
			filestream.write(reinterpret_cast<char*>(&id), sizeof(unsigned int));
		}
	}

	// save earliest occupy times
	for( i=0; i<MAX_NAV_TEAMS; ++i )
	{
		// no spot in the map should take longer than this to reach
		filestream.write(reinterpret_cast<char*>(&m_earliestOccupyTime[i]), sizeof(float));
	}

	// save light intensity
	for ( i=0; i<NUM_CORNERS; ++i )
	{
		filestream.write(reinterpret_cast<char*>(&m_lightIntensity[i]), sizeof(float));
	}

	// Save special links

	uint64_t linksize = static_cast<uint64_t>(m_offmeshconnections.size()); // size_t changes sizes between 32/64 bits, use a fixed unsigned 64 bit integer for compatibility
	filestream.write(reinterpret_cast<char*>(&linksize), sizeof(uint64_t));

	for (auto& link : m_offmeshconnections)
	{
		// write connecting area ID
		unsigned int id = link.m_link.area->GetID();
		filestream.write(reinterpret_cast<char*>(&id), sizeof(unsigned int));

		// write link type
		OffMeshConnectionType type = link.m_type;
		filestream.write(reinterpret_cast<char*>(&type), sizeof(OffMeshConnectionType));

		// write position
		Vector start = link.m_start;
		filestream.write(reinterpret_cast<char*>(&start), sizeof(Vector));

		Vector end = link.m_end;
		filestream.write(reinterpret_cast<char*>(&end), sizeof(Vector));
	}
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Load a navigation area from the file
 */
NavErrorType CNavArea::Load(std::fstream& filestream, uint32_t version, uint32_t subversion)
{
	if (!filestream.good())
	{
		return NAV_CORRUPT_DATA;
	}

	// load ID
	filestream.read(reinterpret_cast<char*>(&m_id), sizeof(unsigned int));

	// update nextID to avoid collisions
	if (m_id >= m_nextID)
		m_nextID = m_id+1;

	// save attribute flags
	filestream.read(reinterpret_cast<char*>(&m_attributeFlags), sizeof(int));

	if (!filestream.good())
	{
		return NAV_CORRUPT_DATA;
	}

	// load extent of area
	filestream.read(reinterpret_cast<char*>(&m_nwCorner), sizeof(Vector));
	filestream.read(reinterpret_cast<char*>(&m_seCorner), sizeof(Vector));
	// load heights of implicit corners
	filestream.read(reinterpret_cast<char*>(&m_neZ), sizeof(float));
	filestream.read(reinterpret_cast<char*>(&m_swZ), sizeof(float));

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

		DevWarning( "Degenerate Navigation Area #%d at setpos %g %g %g\n", 
			m_id, m_center.x, m_center.y, m_center.z );
	}

	CheckWaterLevel();

	// load connections (IDs) to adjacent areas
	// in the enum order NORTH, EAST, SOUTH, WEST
	for( int d=0; d<NUM_DIRECTIONS; d++ )
	{
		// load number of connections for this direction
		int count = 0;
		filestream.read(reinterpret_cast<char*>(&count), sizeof(int));

		m_connect[d].EnsureCapacity( count );
		for(int i=0; i<count; ++i)
		{
			NavConnect connect;
			unsigned int cid = 0;
			filestream.read(reinterpret_cast<char*>(&cid), sizeof(unsigned int));
			connect.id = cid;

			// don't allow self-referential connections
			if ( connect.id != m_id )
			{
				m_connect[d].AddToTail( connect );
			}
		}
	}

	//
	// Load hiding spots
	//

	// load number of hiding spots
	int hidingSpotCount = 0;
	filestream.read(reinterpret_cast<char*>(&hidingSpotCount), sizeof(int));

	// load HidingSpot objects for this area
	for( int h=0; h<hidingSpotCount; ++h )
	{
		// create new hiding spot and put on master list
		HidingSpot *spot = TheNavMesh->CreateHidingSpot();
		spot->Load(filestream, version);
		m_hidingSpots.AddToTail(spot);
	}

	//
	// Load Place data
	//
	PlaceDirectory::IndexType entry = 0;
	filestream.read(reinterpret_cast<char*>(&entry), sizeof(PlaceDirectory::IndexType));

	// convert entry to actual Place
	SetPlace(placeDirectory.IndexToPlace(entry));

	// load ladder data
	for ( int dir=0; dir<CNavLadder::NUM_LADDER_DIRECTIONS; ++dir )
	{
		int count = 0;
		filestream.read(reinterpret_cast<char*>(&count), sizeof(int));

		for(int i = 0; i < count; ++i )
		{
			NavLadderConnect connect;
			unsigned int id = 0;
			filestream.read(reinterpret_cast<char*>(&id), sizeof(unsigned int));
			connect.id = id;

			bool alreadyConnected = false;
			FOR_EACH_VEC( m_ladder[dir], j )
			{
				if ( m_ladder[dir][j].id == connect.id )
				{
					alreadyConnected = true;
					break;
				}
			}

			if ( !alreadyConnected )
			{
				m_ladder[dir].AddToTail( connect );
			}
		}
	}

	// load earliest occupy times
	for( int i=0; i<MAX_NAV_TEAMS; ++i )
	{
		// no spot in the map should take longer than this to reach
		filestream.read(reinterpret_cast<char*>(&m_earliestOccupyTime[i]), sizeof(float));
	}

	// load light intensity
	for ( int i=0; i<NUM_CORNERS; ++i )
	{
		filestream.read(reinterpret_cast<char*>(&m_lightIntensity[i]), sizeof(float));
	}

	// Load special links

	uint64_t linksize = 0U;
	filestream.read(reinterpret_cast<char*>(&linksize), sizeof(uint64_t));

	for (uint64_t i = 0U; i < linksize; i++)
	{
		unsigned int id = 0;
		filestream.read(reinterpret_cast<char*>(&id), sizeof(unsigned int));

		OffMeshConnectionType type = OffMeshConnectionType::OFFMESH_INVALID;
		filestream.read(reinterpret_cast<char*>(&type), sizeof(OffMeshConnectionType));

		Vector start;
		filestream.read(reinterpret_cast<char*>(&start), sizeof(Vector));

		Vector end;
		filestream.read(reinterpret_cast<char*>(&end), sizeof(Vector));

		m_offmeshconnections.emplace_back(type, id, start, end);
	}

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
				smutils->LogError(myself, "CNavArea::PostLoad: Corrupt navigation ladder data. Cannot connect Navigation Areas.");
				error = NAV_CORRUPT_DATA;
				return error;
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
				smutils->LogError(myself, "CNavArea::PostLoad: Corrupt navigation data. Cannot connect Navigation Areas.");
				error = NAV_CORRUPT_DATA;
				return error;
			}

			connect->length = ( connect->area->GetCenter() - GetCenter() ).Length();
		}
	}

	// func avoid/prefer attributes are controlled by func_nav_cost entities
	ClearAllNavCostEntities();

	// special links, convert IDs to CNavArea pointers

	for (auto& i : m_offmeshconnections)
	{
		NavOffMeshConnection* link = &i;
		auto id = link->m_link.id;
		CNavArea* area = TheNavMesh->GetNavAreaByID(id);

		if (!area)
		{
			smutils->LogError(myself, "CNavArea::PostLoad: Corrupt navigation data. Nav Area #%i Special Link <%s> is missing connecting area!", GetID(), NavOffMeshConnection::OffMeshConnectionTypeToString(link->m_type));
			error = NAV_CORRUPT_DATA;
			return error;
		}

		link->m_link.area = area;

		switch (link->m_type)
		{
		case OffMeshConnectionType::OFFMESH_TELEPORTER:
			link->m_link.length = 0.1f; // teleports are instant
			break;
		case OffMeshConnectionType::OFFMESH_BLAST_JUMP:
		{
			float length = (area->GetCenter() - GetCenter()).Length();
			length += 350.0f; // extra length to compensate for the blast jump
			link->m_link.length = length;
			break;
		}
		default:
			link->m_link.length = (area->GetCenter() - GetCenter()).Length();
			break;
		}
	}

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
	auto modfolder = smutils->GetGameFolderName();
	auto mapname = STRING(gpGlobals->mapname); // TO-DO: Clean up workshop maps

	// filename is local to game dir for Steam, so we need to prepend game dir for regular file save
	char gamePath[256];
	// engine->GetGameDir( gamePath, 256 );
	smutils->BuildPath(SourceMod::Path_SM, gamePath, sizeof(gamePath), "data/navbot/%s/%s.smnav", modfolder, mapname);

	// persistant return value
	static char filename[256];
	Q_strncpy(filename, gamePath, sizeof(gamePath));
	// Q_snprintf( filename, sizeof( filename ), "%s\\" FORMAT_NAVFILE, gamePath, STRING( gpGlobals->mapname ) );

	return filename;
}

//--------------------------------------------------------------------------------------------------------------
/*
============
COM_FixSlashes

Changes all '/' characters into '\' characters, in place.
============
*/
inline void COM_FixSlashes( char *pname )
{
#ifdef _WIN32
	while ( *pname ) 
	{
		if ( *pname == '/' )
			*pname = '\\';
		pname++;
	}
#else
	while ( *pname ) 
	{
		if ( *pname == '\\' )
			*pname = '/';
		pname++;
	}
#endif
}

static void WarnIfMeshNeedsAnalysis( int version )
{
	// Quick check to warn about needing to analyze: nav_strip, nav_delete, etc set
	// every CNavArea's m_approachCount to 0, and delete their m_spotEncounterList.
	// So, if no area has either, odds are good we need an analyze.

	if (!TheNavMesh->IsAnalyzed())
	{
		Warning("The nav mesh needs a full nav_analyze\n");
		return;
	}
}

/**
 * Store Navigation Mesh to a file
 */
bool CNavMesh::Save(void)
{
	BuildAuthorInfo();

	WarnIfMeshNeedsAnalysis(CNavMesh::NavMeshVersion);

	auto path = GetFullPathToNavMeshFile();

	std::fstream filestream;
	filestream.open(path, std::fstream::out | std::fstream::binary | std::fstream::trunc);

	NavMeshFileHeader header(GetSubVersionNumber());
	filestream.write(reinterpret_cast<char*>(&header), sizeof(NavMeshFileHeader));
	NavMeshInfoHeader info;
	info.Init();
	filestream.write(reinterpret_cast<char*>(&info), sizeof(NavMeshInfoHeader));

	filestream.write(reinterpret_cast<char*>(&m_isAnalyzed), sizeof(bool));

	// store author information
	auto& authorinfo = GetAuthorInfo();
	bool authorset = authorinfo.HasCreatorBeenSet();
	filestream.write(reinterpret_cast<char*>(&authorset), sizeof(bool));

	if (authorset)
	{
		auto& creator = authorinfo.GetCreator();

		std::uint64_t length = static_cast<std::uint64_t>(creator.first.length() + 1U);
		filestream.write(reinterpret_cast<char*>(&length), sizeof(std::uint64_t));
		filestream.write(creator.first.c_str(), length);
		std::uint64_t steamid = creator.second;
		filestream.write(reinterpret_cast<char*>(&steamid), sizeof(std::uint64_t));

		std::uint64_t count = static_cast<std::uint64_t>(authorinfo.GetEditorCount());
		filestream.write(reinterpret_cast<char*>(&count), sizeof(std::uint64_t));

		for (auto& naveditor : authorinfo.GetEditors())
		{
			length = static_cast<std::uint64_t>(naveditor.first.length() + 1U);
			filestream.write(reinterpret_cast<char*>(&length), sizeof(std::uint64_t));
			filestream.write(naveditor.first.c_str(), length);
			steamid = naveditor.second;
			filestream.write(reinterpret_cast<char*>(&steamid), sizeof(std::uint64_t));
		}
	}

	{
		// first write the number of waypoints
		std::uint64_t count = static_cast<std::uint64_t>(m_waypoints.size());
		filestream.write(reinterpret_cast<char*>(&count), sizeof(std::uint64_t));

		for (auto& pair : m_waypoints)
		{
			auto& wpt = pair.second;
			wpt->Save(filestream, CNavMesh::NavMeshVersion);
		}
	}

	{
		std::uint64_t count = static_cast<std::uint64_t>(m_volumes.size());
		filestream.write(reinterpret_cast<char*>(&count), sizeof(std::uint64_t));

		for (auto& pair : m_volumes)
		{
			auto& volume = pair.second;
			volume->Save(filestream, CNavMesh::NavMeshVersion);
		}
	}

	{
		std::uint64_t count = static_cast<std::uint64_t>(m_elevators.size());
		filestream.write(reinterpret_cast<char*>(&count), sizeof(std::uint64_t));

		for (auto& pair : m_elevators)
		{
			auto& elevator = pair.second;
			elevator->Save(filestream, CNavMesh::NavMeshVersion);
		}
	}

	{
		std::uint64_t count = static_cast<std::uint64_t>(m_prerequisites.size());
		filestream.write(reinterpret_cast<char*>(&count), sizeof(std::uint64_t));

		for (auto& pair : m_prerequisites)
		{
			auto& prerequisite = pair.second;
			prerequisite->Save(filestream, CNavMesh::NavMeshVersion);
		}
	}

	//
	// Build a directory of the Places in this map
	//
	placeDirectory.Reset();

	FOR_EACH_VEC(TheNavAreas, nit)
	{
		CNavArea *area = TheNavAreas[nit];

		Place place = area->GetPlace();
		placeDirectory.AddPlace(place);
	}

	placeDirectory.Save(filestream);
	SaveCustomDataPreArea(filestream);

	//
	// Store navigation areas
	//
	{
		// store number of areas
		int count = TheNavAreas.Count();
		filestream.write(reinterpret_cast<char*>(&count), sizeof(int));

		// store each area
		FOR_EACH_VEC(TheNavAreas, it)
		{
			CNavArea *area = TheNavAreas[it];
			area->Save(filestream, CNavMesh::NavMeshVersion);
		}
	}

	//
	// Store ladders
	//
	{
		// store number of ladders
		int count = m_ladders.Count();
		filestream.write(reinterpret_cast<char*>(&count), sizeof(int));

		// store each ladder
		for ( int i=0; i<m_ladders.Count(); ++i )
		{
			CNavLadder *ladder = m_ladders[i];
			ladder->Save(filestream, CNavMesh::NavMeshVersion);
		}
	}
	
	//
	// Store derived class mesh info
	//
	SaveCustomData(filestream);
	filestream.close();
	auto filesize = std::filesystem::file_size(path);
	auto pathname = path.string();
	Msg("[NavBot] Navigation Mesh file \"%s\" saved. Size on disk '%" PRIiMAX "' bytes. \n", pathname.c_str(), filesize);

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

	FileFindHandle_t findHandle;
	const char *bspFilename = filesystem->FindFirstEx( "maps/*.bsp", "MOD", &findHandle );
	while ( bspFilename )
	{
		switch ( CheckNavFile( bspFilename ) )
		{
		case NAV_CANT_ACCESS_FILE:
			Warning( "Missing nav file for %s\n", bspFilename );
			break;
		case NAV_INVALID_FILE:
			Warning( "Invalid nav file for %s\n", bspFilename );
			break;
		case NAV_BAD_FILE_VERSION:
			Warning( "Old nav file for %s\n", bspFilename );
			break;
		case NAV_FILE_OUT_OF_DATE:
			Warning( "The nav file for %s is built from an old version of the map\n", bspFilename );
			break;
		case NAV_OK:
			Msg( "The nav file for %s is up-to-date\n", bspFilename );
			break;
		}

		bspFilename = filesystem->FindNext( findHandle );
	}
	filesystem->FindClose( findHandle );
}
static ConCommand sm_nav_check_file_consistency( "sm_nav_check_file_consistency", CommandNavCheckFileConsistency, "Scans the maps directory and reports any missing/out-of-date navigation files.", FCVAR_GAMEDLL | FCVAR_CHEAT );

//--------------------------------------------------------------------------------------------------------------
/**
 * Load AI navigation data from a file
 */
NavErrorType CNavMesh::Load( void )
{
	// free previous navigation mesh data
	Reset();
	placeDirectory.Reset();
	CNavArea::m_nextID = 1;
	auto path = GetFullPathToNavMeshFile();

	if (!std::filesystem::exists(path))
	{
		return NAV_CANT_ACCESS_FILE;
	}

	std::fstream filestream;
	filestream.open(path, std::fstream::in | std::fstream::binary);

	if (!filestream.is_open() || filestream.eof())
	{
		filestream.close();
		return NAV_CANT_ACCESS_FILE;
	}

	if (!filestream.good())
	{
		filestream.close();
		return NAV_CANT_ACCESS_FILE;
	}

	NavMeshFileHeader header;
	filestream.read(reinterpret_cast<char*>(&header), sizeof(NavMeshFileHeader));

	if (!header.IsHeaderValid())
	{
		filestream.close();
		std::string str = path.string();
		smutils->LogError(myself, "Navigation Mesh file \"%s\" has bad header!", str.c_str());
		return NAV_INVALID_FILE;
	}

	if (!header.IsMagicValid())
	{
		filestream.close();
		std::string str = path.string();
		smutils->LogError(myself, "Navigation Mesh file \"%s\" has bad magic number!", str.c_str());
		return NAV_INVALID_FILE;
	}

	if (!header.IsVersionValid())
	{
		filestream.close();
		std::string str = path.string();
		smutils->LogError(myself, "Navigation Mesh file \"%s\" has bad version number! Got '%i', should be '%i' or lower!", str.c_str(), header.version, CNavMesh::NavMeshVersion);
		return NAV_INVALID_FILE;
	}

	if (!header.IsSubVersionValid(GetSubVersionNumber()))
	{
		filestream.close();
		std::string str = path.string();
		smutils->LogError(myself, "Navigation Mesh file \"%s\" has bad sub version number! Got '%i', should be '%i' or lower!", str.c_str(), header.subversion, GetSubVersionNumber());
		return NAV_INVALID_FILE;
	}

	NavMeshInfoHeader info;
	filestream.read(reinterpret_cast<char*>(&info), sizeof(NavMeshInfoHeader));

	if (info.mapversion != gpGlobals->mapversion)
	{
		Warning("Navigation Mesh map version mismatch! \n");
	}

	const char* gpMap = STRING(gpGlobals->mapname);

	if (Q_strcmp(info.mapname, gpMap) != 0)
	{
		Warning("Navigation Mesh was generated for another map! %s != %s \n", info.mapname, gpMap);
	}

	const char* mod = smutils->GetGameFolderName();

	if (Q_strcmp(info.modfolder, mod) != 0)
	{
		Warning("Navigation Mesh was generated from another game! %s != %s \n", info.modfolder, mod);
	}

	filestream.read(reinterpret_cast<char*>(&m_isAnalyzed), sizeof(bool));

	bool authorisset = false;
	filestream.read(reinterpret_cast<char*>(&authorisset), sizeof(bool));

	if (authorisset)
	{
		std::uint64_t length = 0U;
		filestream.read(reinterpret_cast<char*>(&length), sizeof(std::uint64_t));
		std::unique_ptr<char[]> creatornamebuffer = std::make_unique<char[]>(length);
		filestream.read(creatornamebuffer.get(), length);
		std::uint64_t steamid = 0;
		filestream.read(reinterpret_cast<char*>(&steamid), sizeof(std::uint64_t));

		std::string cname(creatornamebuffer.get());
		m_authorinfo.SetCreator(cname, steamid);

		std::uint64_t count = 0U;
		filestream.read(reinterpret_cast<char*>(&count), sizeof(std::uint64_t));

		for (std::uint64_t i = 0U; i < count; i++)
		{
			length = 0U;
			filestream.read(reinterpret_cast<char*>(&length), sizeof(std::uint64_t));
			std::unique_ptr<char[]> editornamebuffer = std::make_unique<char[]>(length);
			filestream.read(editornamebuffer.get(), length);

			std::uint64_t steamid = 0;
			filestream.read(reinterpret_cast<char*>(&steamid), sizeof(std::uint64_t));

			std::string ename(editornamebuffer.get());
			m_authorinfo.AddEditor(ename, steamid);
		}
	}

	{
		std::uint64_t numWaypoints = 0;
		CWaypoint::g_NextWaypointID = 0;
		filestream.read(reinterpret_cast<char*>(&numWaypoints), sizeof(std::uint64_t));
		Vector tmp{ 0.0f, 0.0f, 0.0f };

		m_waypoints.reserve(static_cast<size_t>(numWaypoints));

		for (std::uint64_t i = 0; i < numWaypoints; i++)
		{
			auto wpt = AddWaypoint(tmp);

			if (wpt.has_value())
			{
				NavErrorType error = wpt->get()->Load(filestream, header.version, header.subversion);

				if (error != NAV_OK)
				{
					return error;
				}
			}
		}
	}

	{
		std::uint64_t numVolumes = 0;
		CNavVolume::s_nextID = 0;
		filestream.read(reinterpret_cast<char*>(&numVolumes), sizeof(std::uint64_t));
		Vector tmp{ 0.0f, 0.0f, 0.0f };

		for (std::uint64_t i = 0; i < numVolumes; i++)
		{
			auto volume = AddNavVolume(tmp);

			if (volume.has_value())
			{
				NavErrorType error = volume->get()->Load(filestream, header.version, header.subversion);

				if (error != NAV_OK)
				{
					return error;
				}
			}
		}
	}

	{
		std::uint64_t numElevators = 0;
		CNavElevator::s_nextID = 0;
		filestream.read(reinterpret_cast<char*>(&numElevators), sizeof(std::uint64_t));

		for (std::uint64_t i = 0; i < numElevators; i++)
		{
			auto elevator = AddNavElevator(nullptr);

			if (elevator.has_value())
			{
				NavErrorType error = elevator->get()->Load(filestream, header.version, header.subversion);

				if (error != NAV_OK)
				{
					return error;
				}
			}
		}
	}

	{
		std::uint64_t numPrerequisites = 0;
		CNavPrerequisite::s_nextID = 0;

		filestream.read(reinterpret_cast<char*>(&numPrerequisites), sizeof(std::uint64_t));

		for (std::uint64_t i = 0; i < numPrerequisites; i++)
		{
			auto prereq = AddNavPrerequisite(nullptr);

			if (prereq.has_value())
			{
				NavErrorType error = prereq->get()->Load(filestream, header.version, header.subversion);

				if (error != NAV_OK)
				{
					return error;
				}
			}
		}
	}

	placeDirectory.Load(filestream, header.version);
	LoadCustomDataPreArea(filestream, header.subversion);

	// get number of areas
	int count = 0;
	int i;
	filestream.read(reinterpret_cast<char*>(&count), sizeof(int));

	if (count == 0)
	{
		return NAV_INVALID_FILE;
	}

	Extent extent;
	extent.lo.x = 9999999999.9f;
	extent.lo.y = 9999999999.9f;
	extent.hi.x = -9999999999.9f;
	extent.hi.y = -9999999999.9f;

	// load the areas and compute total extent
	TheNavMesh->PreLoadAreas( count );
	Extent areaExtent;
	for( i=0; i<count; ++i )
	{
		CNavArea *area = TheNavMesh->CreateArea();
		
		auto error = area->Load(filestream, header.version, header.subversion);

		if (error != NAV_OK)
		{
			delete area;
			Reset();
			return error;
		}

		TheNavAreas.AddToTail( area );

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

	FOR_EACH_VEC( TheNavAreas, it )
	{
		AddNavArea( TheNavAreas[ it ] );
	}


	count = 0;
	filestream.read(reinterpret_cast<char*>(&count), sizeof(count));
	m_ladders.EnsureCapacity(count);

	for (i = 0; i < count; i++)
	{
		CNavLadder* ladder = new CNavLadder;
		ladder->Load(this, filestream, header.version);
		m_ladders.AddToTail(ladder);
	}

	// mark stairways (TODO: this can be removed once all maps are re-saved with this attribute in them)
	MarkStairAreas();

	//
	// Load derived class mesh info
	//
	LoadCustomData(filestream, header.subversion);

	//
	// Bind pointers, etc
	//
	NavErrorType loadResult = PostLoad(header.version);

	WarnIfMeshNeedsAnalysis(header.version);

	if (loadResult == NAV_OK)
	{
		smutils->LogMessage(myself, "Loaded Navigation Mesh file \"%s\".", path.string().c_str());
	}

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
NavErrorType CNavMesh::PostLoad( uint32_t version )
{
	// allow areas to connect to each other, etc
	FOR_EACH_VEC( TheNavAreas, pit )
	{
		CNavArea *area = TheNavAreas[ pit ];
		area->PostLoad();
	}

	extern HidingSpotVector TheHidingSpots;
	// allow hiding spots to compute information
	FOR_EACH_VEC( TheHidingSpots, hit )
	{
		HidingSpot *spot = TheHidingSpots[ hit ];
		spot->PostLoad();
	}

	WaypointID topID = 0;

	RebuildWaypointMap();

	// allow waypoints to connect to each other
	for (auto& pair : m_waypoints)
	{
		auto& wpt = pair.second;
		wpt->PostLoad();

		if (wpt->GetID() >= topID)
		{
			topID = wpt->GetID();
		}
	}

	if (m_waypoints.empty())
	{
		CWaypoint::g_NextWaypointID = 0;
	}
	else
	{
		CWaypoint::g_NextWaypointID = topID + 1;
	}

	unsigned int nextVolumeID = 0;

	RebuildVolumeMap();

	for (auto& pair : m_volumes)
	{
		auto& volume = pair.second;
		volume->PostLoad();

		if (volume->GetID() >= nextVolumeID)
		{
			nextVolumeID = volume->GetID();
		}
	}

	if (m_volumes.empty())
	{
		CNavVolume::s_nextID = 0;
	}
	else
	{
		CNavVolume::s_nextID = nextVolumeID + 1;
	}

	RebuildElevatorMap();

	unsigned int nextElevatorID = 0;

	for (auto& pair : m_elevators)
	{
		auto& elevator = pair.second;
		elevator->PostLoad();

		if (elevator->GetID() >= nextElevatorID)
		{
			nextElevatorID = elevator->GetID();
		}
	}

	if (m_elevators.empty())
	{
		CNavElevator::s_nextID = 0;
	}
	else
	{
		CNavElevator::s_nextID = nextElevatorID + 1;
	}

	RebuildPrerequisiteMap();

	unsigned int nextPrerequisiteID = 0;

	for (auto& pair : m_prerequisites)
	{
		pair.second->PostLoad();

		if (pair.second->GetID() >= nextPrerequisiteID)
		{
			nextPrerequisiteID = pair.second->GetID();
		}
	}

	if (m_prerequisites.empty())
	{
		CNavPrerequisite::s_nextID = 0;
	}
	else
	{
		CNavPrerequisite::s_nextID = nextPrerequisiteID + 1;
	}

	for (int i = 0; i < m_ladders.Count(); i++)
	{
		m_ladders[i]->PostLoad(this, version);
	}

	ComputeBattlefrontAreas();
	
	//
	// Allow each nav area to know what other areas have one-way connections to it. Need to gather
	// then sort due to allocation restrictions on the 360
	//


	OneWayLink_t oneWayLink;
	CUtlVectorFixedGrowable<OneWayLink_t, 512> oneWayLinks;

	FOR_EACH_VEC( TheNavAreas, oit )
	{
		oneWayLink.area = TheNavAreas[ oit ];
	
		for( int d=0; d<NUM_DIRECTIONS; d++ )
		{
			const NavConnectVector *connectList = oneWayLink.area->GetAdjacentAreas( (NavDirType)d );

			FOR_EACH_VEC( (*connectList), it )
			{
				NavConnect connect = (*connectList)[ it ];
				oneWayLink.destArea = connect.area;
			
				// if the area we connect to has no connection back to us, allow that area to remember us as an incoming connection
				oneWayLink.backD = OppositeDirection( (NavDirType)d );		
				const NavConnectVector *backConnectList = oneWayLink.destArea->GetAdjacentAreas( (NavDirType)oneWayLink.backD );
				bool isOneWay = true;
				FOR_EACH_VEC( (*backConnectList), bit )
				{
					NavConnect backConnect = (*backConnectList)[ bit ];
					if (backConnect.area->GetID() == oneWayLink.area->GetID())
					{
						isOneWay = false;
						break;
					}
				}
				
				if (isOneWay)
				{
					oneWayLinks.AddToTail( oneWayLink );
				}
			}
		}
	}

	oneWayLinks.Sort( &OneWayLink_t::Compare );

	for ( int i = 0; i < oneWayLinks.Count(); i++ )
	{
		// add this one-way connection
		oneWayLinks[i].destArea->AddIncomingConnection( oneWayLinks[i].area, (NavDirType)oneWayLinks[i].backD );	
	}

	ValidateNavAreaConnections();

	// TERROR: loading into a map directly creates entities before the mesh is loaded.  Tell the preexisting
	// entities now that the mesh is loaded so they can update areas.
	for ( int i=0; i<m_avoidanceObstacles.Count(); ++i )
	{
		m_avoidanceObstacles[i]->OnNavMeshLoaded();
	}

	// the Navigation Mesh has been successfully loaded
	m_isLoaded = true;
	extmanager->GetMod()->OnNavMeshLoaded();

	return NAV_OK;
}

std::string CNavMesh::GetMapFileName() const
{
	auto mapname = gamehelpers->GetCurrentMap();
	return std::string{ mapname };
}

std::filesystem::path CNavMesh::GetFullPathToNavMeshFile() const
{
	char fullpath[PLATFORM_MAX_PATH];
	auto map = GetMapFileName();
	auto mod = smutils->GetGameFolderName();
	smutils->BuildPath(SourceMod::PathType::Path_SM, fullpath, sizeof(fullpath), "data/navbot/%s/%s.smnav", mod, map.c_str());
	return std::filesystem::path(fullpath);
}

void CNavMesh::BuildAuthorInfo()
{
	auto host = playerhelpers->GetGamePlayer(1); // gets the listen server host

	if (!host->IsAuthorized())
	{
		rootconsole->ConsolePrint("Warning: Nav Mesh author information not saved! Not authenticated with Steam!");
		return;
	}

	if (!m_authorinfo.HasCreatorBeenSet()) // add original author
	{
		auto name = host->GetName();

		if (!name)
		{
			rootconsole->ConsolePrint("Warning: Nav Mesh author information not saved! Failed to get player name!");
			return;
		}

		auto steamid = host->GetSteamId64(true);

		if (steamid == 0)
		{
			rootconsole->ConsolePrint("Warning: Nav Mesh author information not saved! Not authenticated with Steam!");
			return;
		}

		std::string szname(name);
		m_authorinfo.SetCreator(szname, steamid);
		rootconsole->ConsolePrint("Saved author information: %s,%lli", szname.c_str(), steamid);
		return;
	}
	else // add editor
	{
		auto name = host->GetName();

		if (!name)
		{
			rootconsole->ConsolePrint("Warning: Nav Mesh editor information not saved! Failed to get player name!");
			return;
		}

		auto steamid = host->GetSteamId64(true);

		if (steamid == 0)
		{
			rootconsole->ConsolePrint("Warning: Nav Mesh editor information not saved! Not authenticated with Steam!");
			return;
		}

		if (m_authorinfo.IsCreator(steamid))
		{
			return; // already saved, skip
		}

		if (m_authorinfo.IsEditor(steamid))
		{
			return; // already saved, skip
		}

		std::string szname(name);
		m_authorinfo.AddEditor(szname, steamid);
		rootconsole->ConsolePrint("Saved editor information: %s,%lli", szname.c_str(), steamid);
		return;
	}
}
