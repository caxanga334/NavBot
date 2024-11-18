//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
// nav_mesh.h
// The Navigation Mesh interface
// Author: Michael S. Booth (mike@turtlerockstudios.com), January 2003

//
// Author: Michael S. Booth (mike@turtlerockstudios.com), 2003
//
// NOTE: The Navigation code uses Doxygen-style comments. If you run Doxygen over this code, it will 
// auto-generate documentation.  Visit www.doxygen.org to download the system for free.
//

#ifndef _NAV_MESH_H_
#define _NAV_MESH_H_

#include <filesystem>
#include <fstream>
#include <string>
#include <cstdint>
#include <vector>
#include <utility>
#include <array>
#include <unordered_map>
#include <unordered_set>
#include <optional>
#include <algorithm>

#include "nav.h"
#include <sdkports/sdk_timers.h>
#include <sdkports/eventlistenerhelper.h>
#include <shareddefs.h>
#include <convar.h>

class HidingSpot;
class CUtlBuffer;
class NavPlaceDatabaseLoader;

namespace SourceMod
{
	class IGameConfig;
};


HidingSpot *GetHidingSpotByID( unsigned int id );
//--------------------------------------------------------------------------------------------------------
class NavAreaCollector
{
	bool m_checkForDuplicates;
public:
	NavAreaCollector( bool checkForDuplicates = false )
	{
		m_checkForDuplicates = checkForDuplicates;
	}

	bool operator() ( CNavArea *area )
	{
		if ( m_checkForDuplicates && m_area.HasElement( area ) )
			return true;

		m_area.AddToTail( area );
		return true;
	}
	CUtlVector< CNavArea * > m_area;
};


//--------------------------------------------------------------------------------------------------------
class EditDestroyNotification
{
	CNavArea *m_deadArea;

public:
	EditDestroyNotification( CNavArea *deadArea )
	{
		m_deadArea = deadArea;
	}

	bool operator()( edict_t *actor )
	{
		// TODO: Implement
		return true;
	}
};


//--------------------------------------------------------------------------------------------------------
class NavAttributeClearer
{
public:
	NavAttributeClearer( NavAttributeType attribute )
	{
		m_attribute = attribute;
	}

	bool operator() ( CNavArea *area );

	NavAttributeType m_attribute;
};


//--------------------------------------------------------------------------------------------------------
class NavAttributeSetter
{
public:
	NavAttributeSetter( NavAttributeType attribute )
	{
		m_attribute = attribute;
	}

	bool operator() ( CNavArea *area );

	NavAttributeType m_attribute;
};


//--------------------------------------------------------------------------------------------------------
class NavAttributeToggler
{
public:
	NavAttributeToggler( NavAttributeType attribute )
	{
		m_attribute = attribute;
	}

	bool operator() ( CNavArea *area );

	NavAttributeType m_attribute;
};


//--------------------------------------------------------------------------------------------------------
struct NavAttributeLookup
{
	const char *name;
	NavAttributeType attribute;	
};

extern NavAttributeLookup TheNavAttributeTable[];

//--------------------------------------------------------------------------------------------------------
class SelectOverlappingAreas
{
public:
	bool operator()( CNavArea *area );
};

//--------------------------------------------------------------------------------------------------------
abstract_class INavAvoidanceObstacle
{
public:
	virtual bool IsPotentiallyAbleToObstructNavAreas( void ) const = 0;	// could we at some future time obstruct nav?
	virtual float GetNavObstructionHeight( void ) const = 0;			// height at which to obstruct nav areas
	virtual bool CanObstructNavAreas( void ) const = 0;					// can we obstruct nav right this instant?
	virtual edict_t *GetObstructingEntity( void ) = 0;
	virtual void OnNavMeshLoaded( void ) = 0;
};

//--------------------------------------------------------------------------------------------------------
enum GetNavAreaFlags_t
{
	GETNAVAREA_CHECK_LOS			= 0x1,
	GETNAVAREA_ALLOW_BLOCKED_AREAS	= 0x2,
	GETNAVAREA_CHECK_GROUND			= 0x4,
};

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
class PlaceDirectory
{
public:
	typedef uint32_t IndexType;	// Loaded/Saved as UnsignedShort.  Change this and you'll have to version.

	PlaceDirectory(void)
	{
		m_directory.reserve(1024);
		m_hasUnnamedAreas = false;
		m_currentIndex = 1;
	}

	void Reset(void)
	{
		m_directory.clear();
		m_hasUnnamedAreas = false;
		m_currentIndex = 1;
	}

	/// return true if this place is already in the directory
	bool IsKnown( Place place ) const
	{
		for (auto& object : m_directory)
		{
			if (object.second == place)
			{
				return true;
			}
		}

		return false;
	}

	/// return the directory index corresponding to this Place (0 = no entry)
	IndexType GetIndex(Place place) const
	{
		if (place == UNDEFINED_PLACE)
		{
			return 0;
		}

		for (auto& object : m_directory)
		{
			if (object.second == place)
			{
				return object.first;
			}
		}

		return 0;
	}

	/// add the place to the directory if not already known
	void AddPlace(Place place)
	{
		if (place == UNDEFINED_PLACE)
		{
			m_hasUnnamedAreas = true;
			return;
		}

		if (IsKnown(place))
		{
			return;
		}

		m_directory.emplace(m_currentIndex, place);
		++m_currentIndex;
	}

	void LoadPlace(IndexType entryindex, Place place)
	{
		m_directory.emplace(entryindex, place);
	}

	/// given an index, return the Place
	Place IndexToPlace(IndexType entry) const
	{
		auto it = m_directory.find(entry);

		if (it != m_directory.end())
		{
			return it->second;
		}

		return UNDEFINED_PLACE;
	}

	void Save(std::fstream& filestream);					/// store the directory
	void Load(std::fstream& filestream, uint32_t version);	/// load the directory

	bool HasUnnamedPlaces( void ) const 
	{
		return m_hasUnnamedAreas;
	}

	const std::unordered_map<IndexType, Place>& GetDirectory() const { return m_directory; }

private:
	std::unordered_map<IndexType, Place> m_directory;
	IndexType m_currentIndex;
	bool m_hasUnnamedAreas;
};

extern PlaceDirectory placeDirectory;

//--------------------------------------------------------------------------------------------------------
/**
 * The CNavMesh is the global interface to the Navigation Mesh.
 * @todo Make this an abstract base class interface, and derive mod-specific implementations.
 */
class CNavMesh : public CEventListenerHelper
{
public:
	CNavMesh( void );
	virtual ~CNavMesh();

	static constexpr uint32_t NavMeshVersion = 1;
	static constexpr uint32_t NavMagicNumber = 0x20110FC0;

	typedef std::pair<std::string, uint64_t> NavEditor; // name & steamid pair

	class AuthorInfo
	{
	public:
		AuthorInfo()
		{
			creator.second = 0;
			editors.reserve(16);
		}

		inline bool HasCreatorBeenSet() const { return creator.second != 0; }
		inline void SetCreator(std::string name, uint64_t steamid)
		{
			creator = std::make_pair(name, steamid);
		}

		inline void AddEditor(std::string name, uint64_t steamid)
		{
			editors.emplace_back(name, steamid);
		}

		inline bool IsCreator(uint64_t steamid) const { return creator.second == steamid; }
		inline const NavEditor& GetCreator() const { return creator; }
		inline size_t GetEditorCount() const { return editors.size(); }
		inline const std::vector<NavEditor>& GetEditors() const { return editors; }
		inline bool IsEditor(uint64_t steamid) const
		{
			for (auto& naveditor : editors)
			{
				if (naveditor.second == steamid)
				{
					return true;
				}
			}

			return false;
		}

	private:
		NavEditor creator; // original navmesh author
		std::vector<NavEditor> editors; // list of other editors
	};

	// CEventListenerHelper
	void FireGameEvent(IGameEvent* event) override; // incoming event processing

#if SOURCE_ENGINE >= SE_LEFT4DEAD
	int	GetEventDebugID(void) override;
#endif // SOURCE_ENGINE >= SE_LEFT4DEAD

	// Used by CTraceFilterWalkableEntities to determine if this entity is walkable
	virtual bool IsEntityWalkable(CBaseEntity* pEntity, unsigned int flags);

	virtual void Precache(); // precache edit sounds here
	virtual void OnMapStart();
	virtual void OnMapEnd();
	
	virtual void PreLoadAreas( int nAreas ) {}
	virtual CNavArea *CreateArea( void ) const;							// CNavArea factory
	virtual void DestroyArea( CNavArea * ) const;
	virtual HidingSpot *CreateHidingSpot( void ) const;					// Hiding Spot factory

	virtual void Reset( void );											// destroy Navigation Mesh data and revert to initial state
	virtual void Update( void );										// invoked on each game frame

	virtual NavErrorType Load( void );									// load navigation data from a file
	virtual NavErrorType PostLoad( uint32_t version );				// (EXTEND) invoked after all areas have been loaded - for pointer binding, etc
	inline bool IsLoaded( void ) const		{ return m_isLoaded; }				// return true if a Navigation Mesh has been loaded
	inline bool IsAnalyzed( void ) const	{ return m_isAnalyzed; }			// return true if a Navigation Mesh has been analyzed

	/**
	 * Return true if nav mesh can be trusted for all climbing/jumping decisions because game environment is fairly simple.
	 * Authoritative meshes mean path followers can skip CPU intensive realtime scanning of unpredictable geometry.
	 * This should probably be true by default since we only deal with player bots and players have very limited climbing, unlike Non-Players NextBots
	 */
	virtual bool IsAuthoritative( void ) const { return false; }		

	virtual bool Save(void);									// store Navigation Mesh to a file
	inline bool IsOutOfDate( void ) const	{ return m_isOutOfDate; }			// return true if the Navigation Mesh is older than the current map version

	virtual uint32_t GetSubVersionNumber( void ) const;										// returns sub-version number of data format used by derived classes
	virtual void SaveCustomData(std::fstream& filestream) { }								// store custom mesh data for derived classes
	virtual void LoadCustomData(std::fstream& filestream, uint32_t subVersion ) { }			// load custom mesh data for derived classes
	virtual void SaveCustomDataPreArea(std::fstream& filestream) { }						// store custom mesh data for derived classes that needs to be loaded before areas are read in
	virtual void LoadCustomDataPreArea(std::fstream& filestream, uint32_t subVersion) { }	// load custom mesh data for derived classes that needs to be loaded before areas are read in

	// events
	virtual void OnServerActivate( void );								// (EXTEND) invoked when server loads a new map
	virtual void OnRoundRestart( void );								// invoked when a game round restarts
	virtual void OnRoundRestartPreEntity( void );						// invoked when a game round restarts, but before entities are deleted and recreated
	virtual void OnBreakableCreated( edict_t *breakable ) { }		// invoked when a breakable is created
	virtual void OnBreakableBroken( edict_t *broken ) { }			// invoked when a breakable is broken
	virtual void OnAreaBlocked( CNavArea *area );						// invoked when the area becomes blocked
	virtual void OnAreaUnblocked( CNavArea *area );						// invoked when the area becomes un-blocked
	virtual void OnAvoidanceObstacleEnteredArea( CNavArea *area );					// invoked when the area becomes obstructed
	virtual void OnAvoidanceObstacleLeftArea( CNavArea *area );					// invoked when the area becomes un-obstructed

	virtual void OnEditCreateNotify( CNavArea *newArea );				// invoked when given area has just been added to the mesh in edit mode
	virtual void OnEditDestroyNotify( CNavArea *deadArea );				// invoked when given area has just been deleted from the mesh in edit mode
	virtual void OnEditDestroyNotify( CNavLadder *deadLadder );			// invoked when given ladder has just been deleted from the mesh in edit mode
	virtual void OnNodeAdded( CNavNode *node ) {};						

	// Obstructions
	void RegisterAvoidanceObstacle( INavAvoidanceObstacle *obstruction );
	void UnregisterAvoidanceObstacle( INavAvoidanceObstacle *obstruction );
	const CUtlVector< INavAvoidanceObstacle * > &GetObstructions( void ) const { return m_avoidanceObstacles; }

	unsigned int GetNavAreaCount( void ) const	{ return m_areaCount; }	// return total number of nav areas

	// See GetNavAreaFlags_t for flags
	CNavArea *GetNavArea( const Vector &pos, float beneathLimt = 120.0f ) const;	// given a position, return the nav area that IsOverlapping and is *immediately* beneath it
	CNavArea *GetNavArea( edict_t *pEntity, int nGetNavAreaFlags, float flBeneathLimit = 120.0f ) const;
	CNavArea *GetNavAreaByID( unsigned int id ) const;
	CNavArea *GetNearestNavArea( const Vector &pos, float maxDist = 10000.0f, bool checkLOS = false, bool checkGround = true, int team = NAV_TEAM_ANY ) const;
	CNavArea *GetNearestNavArea( edict_t *pEntity, int nGetNavAreaFlags = GETNAVAREA_CHECK_GROUND, float maxDist = 10000.0f ) const;
	CNavArea* GetNearestNavArea(CBaseEntity* entity, float maxDist = 10000.0f, bool checkLOS = false, bool checkGround = true, int team = NAV_TEAM_ANY) const;
	template <typename T>
	static T* GetRandomNavArea();

	Place GetPlace( const Vector &pos ) const;							// return Place at given coordinate
	// const char *PlaceToName( Place place ) const;						// given a place, return its name
	const std::string* GetPlaceName(const Place place) const;
	Place GetPlaceFromName(const std::string& name) const;
	Place NameToPlace( const char *name ) const;						// given a place name, return a place ID or zero if no place is defined
	Place PartialNameToPlace( const char *name ) const;					// given the first part of a place name, return a place ID or zero if no place is defined, or the partial match is ambiguous
	void PrintAllPlaces( void ) const;									// output a list of names to the console
	int PlaceNameAutocomplete( char const *partial, char commands[ COMMAND_COMPLETION_MAXITEMS ][ COMMAND_COMPLETION_ITEM_LENGTH ] );	// Given a partial place name, fill in possible place names for ConCommand autocomplete

	/**
	 * @brief Given a place index, returns a human readable name for the place.
	 * @param place Place index.
	 * @return Place readable name or NULL on invalid place index.
	 */
	std::optional<std::string> GetPlaceDisplayName(Place place) const;

	static bool GetGroundHeight( const Vector &pos, float *height, Vector *normal = NULL );		// get the Z coordinate of the topmost ground level below the given point
	bool GetSimpleGroundHeight( const Vector &pos, float *height, Vector *normal = NULL ) const;// get the Z coordinate of the ground level directly below the given point


	/// increase "danger" weights in the given nav area and nearby ones
	void IncreaseDangerNearby( int teamID, float amount, CNavArea *area, const Vector &pos, float maxRadius, float dangerLimit = -1.0f );
	void DrawDanger( void ) const;										// draw the current danger levels
	void DrawPlayerCounts( void ) const;								// draw the current player counts for each area
	void DrawFuncNavAvoid( void ) const;								// draw bot avoidance areas from func_nav_avoid entities
	void DrawFuncNavPrefer( void ) const;								// draw bot preference areas from func_nav_prefer entities
#ifdef NEXT_BOT
	void DrawFuncNavPrerequisite( void ) const;							// draw bot prerequisite areas from func_nav_prerequisite entities
#endif
	//-------------------------------------------------------------------------------------
	// Auto-generation
	//
	#define INCREMENTAL_GENERATION true
	void BeginGeneration( bool incremental = false );					// initiate the generation process
	void BeginAnalysis( bool quitWhenFinished = false );						// re-analyze an existing Mesh.  Determine Hiding Spots, Encounter Spots, etc.

	bool IsGenerating( void ) const		{ return m_generationMode != GENERATE_NONE; }	// return true while a Navigation Mesh is being generated
	/**
	 * @brief Adds an entity classname to the list of entities to be used for generating walkable spots
	 * @param name Entity classname
	 * @param useCenter if true, use the entity WorldSpaceCenter instead of AbsOrigin for the walkable spot. 
	 */
	void AddWalkableEntity(const char* name, bool useCenter = false) { m_walkableEntities.emplace(name, useCenter); }
	void AddWalkableSeed( const Vector &pos, const Vector &normal );	// add given walkable position to list of seed positions for map sampling
	virtual void AddWalkableSeeds( void );								// adds walkable positions for any/all positions a mod specifies
	void ClearWalkableSeeds( void )		{ m_walkableSeeds.RemoveAll(); }	// erase all walkable seed positions
	void MarkStairAreas( void );

	virtual unsigned int GetGenerationTraceMask( void ) const;			// return the mask used by traces when generating the mesh


	//-------------------------------------------------------------------------------------
	// Edit mode
	//
	unsigned int GetNavPlace( void ) const			{ return m_navPlace; }
	void SetNavPlace( unsigned int place )			{ m_navPlace = place; }

	// Edit callbacks from ConCommands
	void CommandNavDelete( void );										// delete current area
	void CommandNavDeleteMarked( void );								// delete current marked area

	virtual void CommandNavFloodSelect( const CCommand &args );			// select current area and all connected areas, recursively
	void CommandNavToggleSelectedSet( void );							// toggles all areas into/out of the selected set
	void CommandNavStoreSelectedSet( void );							// stores the current selected set for later
	void CommandNavRecallSelectedSet( void );							// restores an older selected set
	void CommandNavAddToSelectedSet( void );							// add current area to selected set
	void CommandNavAddToSelectedSetByID(  const CCommand &args );		// add specified area id to selected set
	void CommandNavRemoveFromSelectedSet( void );						// remove current area from selected set
	void CommandNavToggleInSelectedSet( void );							// add/remove current area from selected set
	void CommandNavClearSelectedSet( void );							// clear the selected set to empty
	void CommandNavBeginSelecting( void );								// start continuously selecting areas into the selected set
	void CommandNavEndSelecting( void );								// stop continuously selecting areas into the selected set
	void CommandNavBeginDragSelecting( void );							// start dragging a selection area
	void CommandNavEndDragSelecting( void );							// stop dragging a selection area
	void CommandNavBeginDragDeselecting( void );						// start dragging a deselection area
	void CommandNavEndDragDeselecting( void );							// stop dragging a deselection area
	void CommandNavRaiseDragVolumeMax( void );							// raise the top of the drag volume
	void CommandNavLowerDragVolumeMax( void );							// lower the top of the drag volume
	void CommandNavRaiseDragVolumeMin( void );							// raise the bottom of the drag volume
	void CommandNavLowerDragVolumeMin( void );							// lower the bottom of the drag volume
	void CommandNavToggleSelecting( bool playSound = true );			// start/stop continuously selecting areas into the selected set
	void CommandNavBeginDeselecting( void );							// start continuously de-selecting areas from the selected set
	void CommandNavEndDeselecting( void );								// stop continuously de-selecting areas from the selected set
	void CommandNavToggleDeselecting( bool playSound = true );			// start/stop continuously de-selecting areas from the selected set
	void CommandNavSelectInvalidAreas( void );							// adds invalid areas to the selected set
	void CommandNavSelectBlockedAreas( void );							// adds blocked areas to the selected set
	void CommandNavSelectObstructedAreas( void );						// adds obstructed areas to the selected set
	void CommandNavSelectDamagingAreas( void );							// adds damaging areas to the selected set
	void CommandNavSelectHalfSpace( const CCommand &args );				// selects all areas that intersect the half-space
	void CommandNavSelectStairs( void );								// adds stairs areas to the selected set
	void CommandNavSelectOrphans( void );								// adds areas not connected to mesh to the selected set

	void CommandNavSplit( void );										// split current area
	void CommandNavMerge( void );										// merge adjacent areas
	void CommandNavMark( const CCommand &args );						// mark an area for further operations
	void CommandNavUnmark( void );										// removes the mark

	void CommandNavBeginArea( void );									// begin creating a new nav area
	void CommandNavEndArea( void );										// end creation of the new nav area

	void CommandNavBeginShiftXY( void );								// begin shifting selected set in the XY plane
	void CommandNavEndShiftXY( void );									// end shifting selected set in the XY plane

	void CommandNavConnect( void );										// connect marked area to selected area
	void CommandNavDisconnect( void );									// disconnect marked area from selected area
	void CommandNavDisconnectOutgoingOneWays( void );					// disconnect all outgoing one-way connects from each area in the selected set
	void CommandNavSplice( void );										// create new area in between marked and selected areas
	// void CommandNavCrouch( void );										// toggle crouch attribute on current area
	void CommandNavTogglePlaceMode( void );								// switch between normal and place editing
	// void CommandNavSetPlaceMode( void );								// switch between normal and place editing
	void CommandNavPlaceFloodFill( void );								// floodfill areas out from current area
	void CommandNavPlaceSet( void );									// sets the Place for the selected set
	void CommandNavPlacePick( void );									// "pick up" the place at the current area
	void CommandNavTogglePlacePainting( void );							// switch between "painting" places onto areas
	void CommandNavMarkUnnamed( void );									// mark an unnamed area for further operations
	void CommandNavCornerSelect( void );								// select a corner on the current area
	void CommandNavCornerRaise( const CCommand &args );					// raise a corner on the current area
	void CommandNavCornerLower( const CCommand &args );					// lower a corner on the current area
	void CommandNavCornerPlaceOnGround( const CCommand &args );			// position a corner on the current area at ground height
	void CommandNavWarpToMark( void );									// warp a spectating local player to the selected mark
	void CommandNavLadderFlip( void );									// Flips the direction a ladder faces
	void CommandNavToggleAttribute( NavAttributeType attribute );		// toggle an attribute on current area
	void CommandNavMakeSniperSpots( void );								// cuts up the marked area into individual areas suitable for sniper spots
	void CommandNavBuildLadder( void );									// builds a nav ladder on the climbable surface under the cursor
	void CommandNavRemoveJumpAreas( void );								// removes jump areas, replacing them with connections
	void CommandNavSubdivide( const CCommand &args );					// subdivide each nav area in X and Y to create 4 new areas - limit min size
	void CommandNavSaveSelected( const CCommand &args );				// Save selected set to disk
	void CommandNavMergeMesh( const CCommand &args );					// Merge a saved selected set into the current mesh
	void CommandNavMarkWalkable( void );
	void CommandNavSeedWalkableSpots(void);
	void CommandNavConnectSpecialLink(uint32_t linktype);
	void CommandNavDisconnectSpecialLink(uint32_t linktype);
	void CommandNavSetLinkOrigin();
	void CommandNavWarpToLinkOrigin() const;
	void CommandNavTestForBlocked() const;


	void AddToDragSelectionSet( CNavArea *pArea );
	void RemoveFromDragSelectionSet( CNavArea *pArea );
	void ClearDragSelectionSet( void );

	CNavArea *GetMarkedArea( void ) const;										// return area marked by user in edit mode
	CNavLadder *GetMarkedLadder( void ) const	{ return m_markedLadder; }		// return ladder marked by user in edit mode

	CNavArea *GetSelectedArea( void ) const		{ return m_selectedArea; }		// return area user is pointing at in edit mode
	CNavLadder *GetSelectedLadder( void ) const	{ return m_selectedLadder; }	// return ladder user is pointing at in edit mode
	void SetMarkedLadder( CNavLadder *ladder );							// mark ladder for further edit operations
	void SetMarkedArea( CNavArea *area );								// mark area for further edit operations

	bool IsContinuouslySelecting( void ) const
	{
		return m_isContinuouslySelecting;
	}

	bool IsContinuouslyDeselecting( void ) const
	{
		return m_isContinuouslyDeselecting;
	}

	void CreateLadder( const Vector &mins, const Vector &maxs, float maxHeightAboveTopArea );
	void CreateLadder( const Vector &top, const Vector &bottom, float width, const Vector2D &ladderDir, float maxHeightAboveTopArea );

	float SnapToGrid( float x, bool forceGrid = false ) const;					// snap given coordinate to generation grid boundary
	Vector SnapToGrid( const Vector& in, bool snapX = true, bool snapY = true, bool forceGrid = false ) const;	// snap given vector's X & Y coordinates to generation grid boundary

	const Vector &GetEditCursorPosition( void ) const	{ return m_editCursorPos; }	// return position of edit cursor
	void StripNavigationAreas( void );
	static const char *GetFilename( void );								// return the filename for this map's "nav" file

	/// @todo Remove old select code and make all commands use this selected set
	void AddToSelectedSet( CNavArea *area );							// add area to the currently selected set
	void RemoveFromSelectedSet( CNavArea *area );						// remove area from the currently selected set
	void ClearSelectedSet( void );										// clear the currently selected set to empty
	bool IsSelectedSetEmpty( void ) const;								// return true if the selected set is empty
	bool IsInSelectedSet( const CNavArea *area ) const;					// return true if the given area is in the selected set
	int GetSelecteSetSize( void ) const;
	const NavAreaVector &GetSelectedSet( void ) const;					// return the selected set

	/**
	 * Apply the functor to all navigation areas in the Selected Set,
	 * or the current selected area.
	 * If functor returns false, stop processing and return false.
	 */
	template < typename Functor >
	bool ForAllSelectedAreas( Functor &func )
	{
		if (IsSelectedSetEmpty())
		{
			CNavArea *area = GetSelectedArea();
			
			if (area && !func( area )) {
				return false;
			}
		}
		else if (!forAll(func, m_selectedSet)) {
			return false;
		}
		return true;
	}

	//-------------------------------------------------------------------------------------
	/**
	 * Apply the functor to all navigation areas.
	 * If functor returns false, stop processing and return false.
	 */
	template < typename Functor >
	static bool ForAllAreas( Functor &func )
	{
		extern NavAreaVector TheNavAreas;
		return forAll(func, TheNavAreas);
	}

	//-------------------------------------------------------------------------------------
	/**
	 * Apply the functor to all navigation areas that overlap the given extent.
	 * If functor returns false, stop processing and return false.
	 */
	template < typename Functor >
	bool ForAllAreasOverlappingExtent( Functor &func, const Extent &extent );

	//-------------------------------------------------------------------------------------
	/**
	 * Populate the given vector with all navigation areas that overlap the given extent.
	 */
	template< typename NavAreaType >
	void CollectAreasOverlappingExtent( const Extent &extent, CUtlVector< NavAreaType * > *outVector );

	template < typename Functor >
	bool ForAllAreasInRadius( Functor &func, const Vector &pos, float radius );

	//---------------------------------------------------------------------------------------------------------------
	/*
	 * Step through nav mesh along line between startArea and endArea.
	 * Return true if enumeration reached endArea, false if doesn't reach it (no mesh between, bad connection, etc)
	 */
	template < typename Functor >
	bool ForAllAreasAlongLine( Functor &func, CNavArea *startArea, CNavArea *endArea );

	//-------------------------------------------------------------------------------------
	/**
	 * Apply the functor to all navigation ladders.
	 * If functor returns false, stop processing and return false.
	 */
	template < typename Functor >
	bool ForAllLadders( Functor &func )
	{
		return forAll(func, m_ladders);
	}

	//-------------------------------------------------------------------------------------
	/**
	 * Apply the functor to all navigation ladders.
	 * If functor returns false, stop processing and return false.
	 */
	template < typename Functor >
	bool ForAllLadders( Functor &func ) const
	{
		return forAll(func, m_ladders);
	}

	template <typename Functor, typename T>
	static bool forAll(Functor& func, CUtlVector<T>& vec) {
		for ( int i=0; i<vec.Count(); ++i )
		{
			if (!func( vec[i] ))
				return false;
		}

		return true;
	}

	//-------------------------------------------------------------------------------------
	/**
	 * tests a new area for connections to adjacent pre-existing areas
	 */
	template < typename Functor > void StitchAreaIntoMesh( CNavArea *area, NavDirType dir, Functor &func );

	//-------------------------------------------------------------------------------------
	/**
	 * Use the functor to test if an area is needing stitching into the existing nav mesh.
	 * The functor is different from how we normally use functors - it does no processing,
	 * and it's return value is true if the area is in the new set to be stiched, and false
	 * if it's a pre-existing area.
	 */
	template < typename Functor >
		bool StitchMesh( Functor &func )
	{
		extern NavAreaVector TheNavAreas;
		FOR_EACH_VEC( TheNavAreas, it )
		{
			CNavArea *area = TheNavAreas[ it ];

			if ( func( area ) )
			{
				StitchAreaIntoMesh( area, NORTH, func );
				StitchAreaIntoMesh( area, SOUTH, func );
				StitchAreaIntoMesh( area, EAST, func );
				StitchAreaIntoMesh( area, WEST, func );
			}
		}

		return true;
	}

	NavLadderVector& GetLadders( void ) { return m_ladders; }	// Returns the list of ladders
	CNavLadder *GetLadderByID( unsigned int id ) const;

	CUtlVector< CNavArea * >& GetTransientAreas( void ) { return m_transientAreas; }

	enum EditModeType
	{
		NORMAL,				// normal mesh editing
		PLACE_PAINTING,		// in place painting mode
		CREATING_AREA,		// creating a new nav area
		CREATING_LADDER,	// creating a nav ladder
		DRAG_SELECTING,		// drag selecting a set of areas
		SHIFTING_XY,		// shifting selected set in XY plane
		SHIFTING_Z,			// shifting selected set in Z plane
	};
	EditModeType GetEditMode( void ) const;						// return the current edit mode
	void SetEditMode( EditModeType mode );						// change the edit mode
	bool IsEditMode( EditModeType mode ) const;					// return true if current mode matches given mode

	bool FindNavAreaOrLadderAlongRay( const Vector &start, const Vector &end, CNavArea **area, CNavLadder **ladder, CNavArea *ignore = NULL );

	void SimplifySelectedAreas( void );	// Simplifies the selected set by reducing to 1x1 areas and re-merging them up with loosened tolerances

	// Formats the map filename for save/load
	virtual std::string GetMapFileName() const;
	std::filesystem::path GetFullPathToNavMeshFile() const;
	const AuthorInfo& GetAuthorInfo() const { return m_authorinfo; }

	void LoadEditSounds(SourceMod::IGameConfig* gamedata);

	enum class EditSoundType : uint32_t
	{
		SOUND_GENERIC_BLIP = 0,
		SOUND_GENERIC_SUCCESS,
		SOUND_GENERIC_ERROR,
		SOUND_SWITCH_ON,
		SOUND_SWITCH_OFF,
		SOUND_GENERIC_OFF,
		SOUND_CONNECT_FAIL,
		SOUND_WAYPOINT_ADD,

		MAX_EDIT_SOUNDS
	};

	inline void PlayEditSound(EditSoundType type) const
	{
		PlayEditSoundInternal(m_editsounds[static_cast<size_t>(type)]);
	}

protected:
	virtual void PostCustomAnalysis( void ) { }					// invoked when custom analysis step is complete
	bool FindActiveNavArea( void );								// Finds the area or ladder the local player is currently pointing at.  Returns true if a surface was hit by the traceline.
	virtual void RemoveNavArea( CNavArea *area );				// remove an area from the grid
	bool FindGroundForNode( Vector *pos, Vector *normal ) const;
	void GenerateNodes( const Extent &bounds );
	void RemoveNodes( void );

	const NavAreaVector& GetSelectedAreaSet() const { return m_selectedSet; }
	void SetMarkedCorner(NavCornerType corner) { m_markedCorner = corner; }

private:
	friend class CNavArea;
	friend class CNavNode;
	friend class CNavUIBasePanel;
	friend class CWaypoint;

	mutable CUtlVector<NavAreaVector> m_grid;
	float m_gridCellSize;										// the width/height of a grid cell for spatially partitioning nav areas for fast access
	int m_gridSizeX;
	int m_gridSizeY;
	float m_minX;
	float m_minY;
	unsigned int m_areaCount;									// total number of nav areas

	bool m_isLoaded;											// true if a Navigation Mesh has been loaded
	bool m_isOutOfDate;											// true if the Navigation Mesh is older than the actual BSP
	bool m_isAnalyzed;											// true if the Navigation Mesh needs analysis

	static constexpr auto HASH_TABLE_SIZE = 256;
	CNavArea *m_hashTable[ HASH_TABLE_SIZE ];					// hash table to optimize lookup by ID
	int ComputeHashKey( unsigned int id ) const;				// returns a hash key for the given nav area ID

	int WorldToGridX( float wx ) const;							// given X component, return grid index
	int WorldToGridY( float wy ) const;							// given Y component, return grid index
	void AllocateGrid( float minX, float maxX, float minY, float maxY );	// clear and reset the grid to the given extents
	void GridToWorld( int gridX, int gridY, Vector *pos ) const;

	void AddNavArea( CNavArea *area );							// add an area to the grid

	void DestroyNavigationMesh( bool incremental = false );		// free all resources of the mesh and reset it to empty state
	void DestroyHidingSpots( void );

	void ComputeBattlefrontAreas( void );						// determine areas where rushing teams will first meet

	//----------------------------------------------------------------------------------
	// Place directory
	//

	/**
	 * @brief Map of place names
	 * First is the place key name (IE: CTSpawn)
	 * Second is the place display/full name (IE: Counter-Terrorist Spawn)
	 */
	std::unordered_map<Place, std::pair<std::string, std::string>> m_placeMap;

	// char **m_placeName;											// master directory of place names (ie: "places")
	// unsigned int m_placeCount;									// number of "places" defined in placeName[]
	void LoadPlaceDatabase( void );								// load the place names from a file
	void ParseGlobalPlaceDatabase(NavPlaceDatabaseLoader& loader);
	void ParseModPlaceDatabase(NavPlaceDatabaseLoader& loader);
	void ParseMapPlaceDatabase(NavPlaceDatabaseLoader& loader);
	size_t GetPlaceCount() const { return m_placeMap.size(); }
	const std::unordered_map<Place, std::pair<std::string, std::string>>& GetPlaces() const { return m_placeMap; }

	//----------------------------------------------------------------------------------
	// Edit mode
	//
	EditModeType m_editMode;									// the current edit mode
	bool m_isEditing;											// true if in edit mode

	unsigned int m_navPlace;									// current navigation place for editing
	void OnEditModeStart( void );								// called when edit mode has just been enabled
	void DrawEditMode( void );									// draw navigation areas
	void DrawWaypoints();										// draw waypoints
	void OnEditModeEnd( void );									// called when edit mode has just been disabled
	void UpdateDragSelectionSet( void );							// update which areas are overlapping the drag selected bounds
	Vector m_editCursorPos;										// current position of the cursor
	CNavArea *m_markedArea;										// currently marked area for edit operations
	CNavArea *m_selectedArea;									// area that is selected this frame
	CNavArea *m_lastSelectedArea;								// area that was selected last frame
	NavCornerType m_markedCorner;								// currently marked corner for edit operations
	Vector m_anchor;											// first corner of an area being created
	bool m_isPlacePainting;										// if true, we set an area's place by pointing at it
	bool m_splitAlongX;											// direction the selected nav area would be split
	float m_splitEdge;											// location of the possible split

	bool m_climbableSurface;									// if true, the cursor is pointing at a climable surface
	Vector m_surfaceNormal;										// Normal of the surface the cursor is pointing at
	Vector m_ladderAnchor;										// first corner of a ladder being created
	Vector m_ladderNormal;										// Normal of the surface of the ladder being created
	CNavLadder *m_selectedLadder;								// ladder that is selected this frame
	CNavLadder *m_lastSelectedLadder;							// ladder that was selected last frame
	CNavLadder *m_markedLadder;									// currently marked ladder for edit operations

	bool FindLadderCorners( Vector *c1, Vector *c2, Vector *c3 );	// computes the other corners of a ladder given m_ladderAnchor, m_editCursorPos, and m_ladderNormal

	void GetEditVectors( Vector *pos, Vector *forward );		// Gets the eye position and view direction of the editing player

	CountdownTimer m_showAreaInfoTimer;							// Timer that controls how long area info is displayed
	
	NavAreaVector m_selectedSet;								// all currently selected areas
	NavAreaVector m_dragSelectionSet;							// all areas in the current drag selection
	bool m_isContinuouslySelecting;								// if true, we are continuously adding to the selected set
	bool m_isContinuouslyDeselecting;							// if true, we are continuously removing from the selected set

	bool m_bIsDragDeselecting;
	int m_nDragSelectionVolumeZMax;
	int m_nDragSelectionVolumeZMin;

	void DoToggleAttribute( CNavArea *area, NavAttributeType attribute );		// toggle an attribute on given area


	//----------------------------------------------------------------------------------
	// Auto-generation
	//
	bool UpdateGeneration( float maxTime = 0.25f );				// process the auto-generation for 'maxTime' seconds. return false if generation is complete.

	virtual void BeginCustomAnalysis( bool bIncremental ) {}
	virtual void EndCustomAnalysis() {}

	CNavNode *m_currentNode;									// the current node we are sampling from
	NavDirType m_generationDir;
	CNavNode *AddNode( const Vector &destPos, const Vector &destNormal, NavDirType dir, CNavNode *source, bool isOnDisplacement, float obstacleHeight, float flObstacleStartDist, float flObstacleEndDist );		// add a nav node and connect it, update current node

	NavLadderVector m_ladders;									// list of ladder navigation representations
	void BuildLadders( void );
	void DestroyLadders( void );

	bool SampleStep( void );									// sample the walkable areas of the map
	void CreateNavAreasFromNodes( void );						// cover all of the sampled nodes with nav areas

	bool TestArea( CNavNode *node, int width, int height );		// check if an area of size (width, height) can fit, starting from node as upper left corner
	int BuildArea( CNavNode *node, int width, int height );		// create a CNavArea of size (width, height) starting fom node at upper left corner
	bool CheckObstacles( CNavNode *node, int width, int height, int x, int y );

	void MarkPlayerClipAreas( void );
	void MarkJumpAreas( void );
	void StichAndRemoveJumpAreas( void );
	void RemoveJumpAreas( void );	
	void SquareUpAreas( void );
	void MergeGeneratedAreas( void );
	void ConnectGeneratedAreas( void );
	void FixUpGeneratedAreas( void );
	void FixCornerOnCornerAreas( void );
	void FixConnections( void );
	void SplitAreasUnderOverhangs( void );
	void ValidateNavAreaConnections( void );
	void StitchGeneratedAreas( void );							// Stitches incrementally-generated areas into the existing mesh
	void StitchAreaSet( CUtlVector< CNavArea * > *areas );		// Stitches an arbitrary set of areas into the existing mesh
	void HandleObstacleTopAreas( void );						// Handles fixing/generating areas on top of slim obstacles such as fences and railings
	void RaiseAreasWithInternalObstacles();
	void CreateObstacleTopAreas();
	bool CreateObstacleTopAreaIfNecessary( CNavArea *area, CNavArea *areaOther, NavDirType dir, bool bMultiNode );
	void RemoveOverlappingObstacleTopAreas();	


	enum GenerationStateType
	{
		SAMPLE_WALKABLE_SPACE,
		CREATE_AREAS_FROM_SAMPLES,
		FIND_HIDING_SPOTS,
		FIND_ENCOUNTER_SPOTS,
		FIND_SNIPER_SPOTS,
		FIND_EARLIEST_OCCUPY_TIMES,
		FIND_LIGHT_INTENSITY,
		COMPUTE_MESH_VISIBILITY,
		CUSTOM,													// mod-specific generation step
		SAVE_NAV_MESH,

		NUM_GENERATION_STATES
	}
	m_generationState;											// the state of the generation process
	enum GenerationModeType
	{
		GENERATE_NONE,
		GENERATE_FULL,
		GENERATE_INCREMENTAL,
		GENERATE_SIMPLIFY,
		GENERATE_ANALYSIS_ONLY,
	}
	m_generationMode;											// true while a Navigation Mesh is being generated
	int m_generationIndex;										// used for iterating nav areas during generation process
	int m_sampleTick;											// counter for displaying pseudo-progress while sampling walkable space
	bool m_bQuitWhenFinished;
	float m_generationStartTime;
	Extent m_simplifyGenerationExtent;

	std::unordered_map<std::string, bool> m_walkableEntities;			// List of entities class names to generate walkable seeds

	struct WalkableSeedSpot
	{
		Vector pos;
		Vector normal;
	};
	CUtlVector< WalkableSeedSpot > m_walkableSeeds;				// list of walkable seed spots for sampling

	CNavNode *GetNextWalkableSeedNode( void );					// return the next walkable seed as a node
	int m_seedIdx;
	int m_hostThreadModeRestoreValue;							// stores the value of host_threadmode before we changed it

	void BuildTransientAreaList( void );
	CUtlVector< CNavArea * > m_transientAreas;

	void UpdateAvoidanceObstacleAreas( void );
	CUtlVector< CNavArea * > m_avoidanceObstacleAreas;
	CUtlVector< INavAvoidanceObstacle * > m_avoidanceObstacles;

	void UpdateBlockedAreas( void );
	CUtlVector< CNavArea * > m_blockedAreas;

	CUtlVector< int > m_storedSelectedSet;						// "Stored" selected set, so we can do some editing and then restore the old selected set.  Done by ID, so we don't have to worry about split/delete/etc.

	void BeginVisibilityComputations( void );
	void EndVisibilityComputations( void );

	void TestAllAreasForBlockedStatus( void );					// Used to update blocked areas after a round restart. Need to delay so the map logic has all fired.
	CountdownTimer m_updateBlockedAreasTimer;		
	CountdownTimer m_invokeAreaUpdateTimer;
	CountdownTimer m_invokeWaypointUpdateTimer;
	static constexpr auto NAV_AREA_UPDATE_INTERVAL = 1.0f;
	void BuildAuthorInfo();
	AuthorInfo m_authorinfo;
	std::array<std::string, static_cast<size_t>(EditSoundType::MAX_EDIT_SOUNDS)> m_editsounds;
	Vector m_linkorigin;

	void PlayEditSoundInternal(const std::string& sound) const;

private:
	std::unordered_map<WaypointID, std::shared_ptr<CWaypoint>> m_waypoints;
	std::shared_ptr<CWaypoint> m_selectedWaypoint; // Selected waypoint for editing

protected:
	// Creates a new waypoint instance
	virtual std::shared_ptr<CWaypoint> CreateWaypoint() const;

	// Rebuilds the waypoint ID map
	void RebuildWaypointMap();

public:
	/**
	 * @brief Adds a new waypoint.
	 * @param origin Initial position of the new waypoint
	 * @return Pointer to waypoint or NULL on failure.
	 */
	std::optional<const std::shared_ptr<CWaypoint>> AddWaypoint(const Vector& origin);

	void RemoveWaypoint(CWaypoint* wpt);

	template <typename T>
	inline std::optional<const std::shared_ptr<T>> GetWaypointOfID(WaypointID id) const
	{
		auto it = m_waypoints.find(id);

		if (it == m_waypoints.end())
		{
			return std::nullopt;
		}

		return it->second;
	}

	/**
	 * @brief Runs a function on every waypoint
	 * @tparam T Waypoint class
	 * @tparam F Function bool (T* waypoint)
	 * @param functor Function to run. Return false to end loop.
	 */
	template <typename T, typename F>
	inline void ForEveryWaypoint(F functor)
	{
		std::for_each(m_waypoints.begin(), m_waypoints.end(), [&functor](const std::pair<WaypointID, std::shared_ptr<CWaypoint>>& object) {
			T* wpt = static_cast<T*>(object.second.get());
			
			if (functor(wpt) == false)
			{
				return;
			}
		});
	}

	/**
	 * @brief Collects waypoints into a vector.
	 * @tparam T Waypoint class.
	 * @tparam F Collect functor. bool (T* waypoint)
	 * @param vec Vector to store the collected waypoints.
	 * @param functor Function to collect waypoints. Return true to collect, false to skip.
	 */
	template <typename T, typename F>
	inline void CollectWaypoints(std::vector<T*>& vec, F functor)
	{
		std::for_each(m_waypoints.begin(), m_waypoints.end(), [&vec, &functor](const std::pair<WaypointID, std::shared_ptr<CWaypoint>>& object) {
			T* wpt = static_cast<T*>(object.second.get());

			if (functor(wpt) == true)
			{
				vec.push_back(wpt);
			}
		});
	}

	const std::unordered_map<WaypointID, std::shared_ptr<CWaypoint>>& GetAllWaypoints() const { return m_waypoints; }

	void CompressWaypointsIDs();

	void SetSelectedWaypoint(std::shared_ptr<CWaypoint> wpt) { m_selectedWaypoint = wpt; }
	void ClearSelectedWaypoint() { m_selectedWaypoint = nullptr; }
	const std::shared_ptr<CWaypoint>& GetSelectedWaypoint() const { return m_selectedWaypoint; }
	void SelectNearestWaypoint(const Vector& start);
	void SelectWaypointofID(WaypointID id);

};

// the global singleton interface
extern CNavMesh *TheNavMesh;

#ifdef STAGING_ONLY
// for debugging the A* algorithm, if nonzero, show debug display and decrement for each pathfind
extern int g_DebugPathfindCounter;
#endif


//--------------------------------------------------------------------------------------------------------------
inline bool CNavMesh::IsEditMode( EditModeType mode ) const
{
	return m_editMode == mode;
}

//--------------------------------------------------------------------------------------------------------------
inline CNavMesh::EditModeType CNavMesh::GetEditMode( void ) const
{
	return m_editMode;
}

//--------------------------------------------------------------------------------------------------------------
inline uint32_t CNavMesh::GetSubVersionNumber( void ) const
{
	return 0;
}

//--------------------------------------------------------------------------------------------------------------
inline int CNavMesh::ComputeHashKey( unsigned int id ) const
{
	return id & 0xFF;
}

//--------------------------------------------------------------------------------------------------------------
inline int CNavMesh::WorldToGridX( float wx ) const
{ 
	int x = (int)( (wx - m_minX) / m_gridCellSize );

	if (x < 0)
		x = 0;
	else if (x >= m_gridSizeX)
		x = m_gridSizeX-1;
	
	return x;
}

//--------------------------------------------------------------------------------------------------------------
inline int CNavMesh::WorldToGridY( float wy ) const
{ 
	int y = (int)( (wy - m_minY) / m_gridCellSize );

	if (y < 0)
		y = 0;
	else if (y >= m_gridSizeY)
		y = m_gridSizeY-1;
	
	return y;
}

//--------------------------------------------------------------------------------------------------------------
//
// Function prototypes
//

extern void ApproachAreaAnalysisPrep( void );
extern void CleanupApproachAreaAnalysisPrep( void );
extern bool IsHeightDifferenceValid( float test, float other1, float other2, float other3 );

#endif // _NAV_MESH_H_
