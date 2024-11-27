//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

// Navigation ladders
// Author: Michael S. Booth (mike@turtlerockstudios.com), January 2003

#ifndef _NAV_LADDER_H_
#define _NAV_LADDER_H_

#include <filesystem>
#include <fstream>
#include <string>
#include <cstdint>
#include <vector>

#include "nav.h"
#include <sdkports/sdk_ehandle.h>

class CUtlBuffer;
class CNavMesh;
class IServerEntity;

//--------------------------------------------------------------------------------------------------------------
/**
 * The NavLadder represents ladders in the Navigation Mesh, and their connections to adjacent NavAreas
 * @todo Deal with ladders that allow jumping off to areas in the middle
 */
class CNavLadder
{
public:
	CNavLadder( void )
	{
		m_topForwardArea = NULL;
		m_topRightArea = NULL;
		m_topLeftArea = NULL;
		m_topBehindArea = NULL;
		m_bottomArea = NULL;
		// set an ID for interactive editing - loads will overwrite this
		m_id = m_nextID++;
		m_length = 0.0f;
		m_width = 0.0f;
		m_dir = NUM_DIRECTIONS;
		m_ladderType = MAX_LADDER_TYPES;
	}

	~CNavLadder();

	void OnRoundRestart( void );			///< invoked when a game round restarts

	void Save(std::fstream& filestream, uint32_t version);
	void Load(CNavMesh* TheNavMesh, std::fstream& filestream, uint32_t version);
	void PostLoad(CNavMesh* TheNavMesh, uint32_t version);

	unsigned int GetID( void ) const	{ return m_id; }		///< return this ladder's unique ID
	static void CompressIDs( CNavMesh* TheNavMesh );							///<re-orders ladder ID's so they are continuous

	enum LadderDirectionType
	{
		LADDER_UP = 0,
		LADDER_DOWN,

		NUM_LADDER_DIRECTIONS
	};

	enum LadderType : std::uint32_t
	{
		SIMPLE_LADDER = 0, // simple ladder (CSS, DODS)
		USEABLE_LADDER, // useable ladder (HL2, HL2DM)

		MAX_LADDER_TYPES
	};

	Vector m_top;									///< world coords of the top of the ladder
	Vector m_bottom;								///< world coords of the top of the ladder
	float m_length;									///< the length of the ladder
	float m_width;

	Vector GetPosAtHeight( float height ) const;	///< Compute x,y coordinate of the ladder at a given height

	CNavArea *m_topForwardArea;						///< the area at the top of the ladder
	CNavArea *m_topLeftArea;
	CNavArea *m_topRightArea;
	CNavArea *m_topBehindArea;						///< area at top of ladder "behind" it - only useful for descending
	CNavArea *m_bottomArea;							///< the area at the bottom of the ladder

	bool IsConnected( const CNavArea *area, LadderDirectionType dir ) const;	///< returns true if given area is connected in given direction

	void ConnectGeneratedLadder( float maxHeightAboveTopArea );		///< Connect a generated ladder to nav areas at the end of nav generation

	void ConnectTo( CNavArea *area );				///< connect this ladder to given area
	void Disconnect( CNavArea *area );				///< disconnect this ladder from given area

	void OnSplit( CNavArea *original, CNavArea *alpha, CNavArea *beta );	///< when original is split into alpha and beta, update our connections
	void OnDestroyNotify( CNavArea *dead );			///< invoked when given area is going away

	void DrawLadder(  bool isSelected,  bool isMarked, bool isEdit ) const;					///< Draws ladder and connections
	void DrawConnectedAreas( bool isEdit );				///< Draws connected areas

	void UpdateDangling( void );					///< Checks if the ladder is dangling (bots cannot go up)

	bool IsInUse( const edict_t *ignore = NULL ) const;	///< return true if someone is on this ladder (other than 'ignore')

	void SetDir( NavDirType dir );
	NavDirType GetDir( void ) const;
	const Vector &GetNormal( void ) const;

	void Shift( const Vector &shift );							///< shift the nav ladder

	bool IsUsableByTeam( int teamNumber ) const;
	CBaseEntity* GetLadderEntity(void) const { return m_ladderEntity.Get(); }

	const Vector& GetUseableLadderEntityOrigin() const { return m_useableOrigin; }

	// Builds a useable ladder of the given entity
	void BuildUseableLadder(CBaseEntity* ladder);
	void UpdateUseableLadderDir(NavDirType dir);

	LadderType GetLadderType() const { return m_ladderType; }

private:
	void FindLadderEntity( void );

	static constexpr auto USABLE_LADDER_ENTITY_SEARCH_RANGE = 512.0f; // search range for the ladder entity
	static constexpr auto USABLE_LADDER_DISMOUNT_POINT_SEARCH_RANGE = 128.0f; // search range for the dismount points around the ladder

	CHandle<CBaseEntity> m_ladderEntity;

	NavDirType m_dir;								///< which way the ladder faces (ie: surface normal of climbable side)
	Vector m_normal;								///< surface normal of the ladder surface (or Vector-ized m_dir, if the traceline fails)
	Vector m_useableOrigin;							/// If this is a useable ladder, this is the origin of the func_useableladder entity

	enum LadderConnectionType						///< Ladder connection directions, to facilitate iterating over connections
	{
		LADDER_TOP_FORWARD = 0,
		LADDER_TOP_LEFT,
		LADDER_TOP_RIGHT,
		LADDER_TOP_BEHIND,
		LADDER_BOTTOM,

		NUM_LADDER_CONNECTIONS
	};

	CNavArea ** GetConnection( LadderConnectionType dir );

	static unsigned int m_nextID;					///< used to allocate unique IDs
	unsigned int m_id;								///< unique area ID
	LadderType m_ladderType;
};

//--------------------------------------------------------------------------------------------------------------
inline NavDirType CNavLadder::GetDir( void ) const
{
	return m_dir;
}


//--------------------------------------------------------------------------------------------------------------
inline const Vector &CNavLadder::GetNormal( void ) const
{
	return m_normal;
}


#endif // _NAV_LADDER_H_
