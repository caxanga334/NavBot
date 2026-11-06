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
#include <variant>

#include "nav.h"
#include <sdkports/sdk_ehandle.h>

class CUtlBuffer;
class CNavMesh;
class IServerEntity;

// Represents a connection from the ladder to exit areas
struct LadderToAreaConnection
{
	LadderToAreaConnection() :
		point(0.0f, 0.0f, 0.0f)
	{
		connect = 0U;
		bottom = true;
	}

	LadderToAreaConnection(CNavArea* area) :
		point(0.0f, 0.0f, 0.0f)
	{
		connect = area;
		bottom = true;
	}

	bool operator==(const LadderToAreaConnection& other)
	{
		if (std::holds_alternative<CNavArea*>(this->connect) == false)
		{
			return false;
		}

		if (std::holds_alternative<CNavArea*>(other.connect) == false)
		{
			return false;
		}

		return std::get<CNavArea*>(this->connect) == std::get<CNavArea*>(other.connect);
	}

	// Post load operations
	void PostLoad();

	CNavArea* GetConnectedArea() const { return std::get<CNavArea*>(connect); }
	bool IsConnectedToLadderBottom() const { return bottom; }
	bool IsConnectedToLadderTop() const { return !bottom; }
	const Vector& GetConnectionPoint() const { return point; }
	void ComputeConnectionPoint(const Vector& top, const Vector& bottom);

	std::variant<unsigned int, CNavArea*> connect;
	Vector point; // connection point on the ladder
	bool bottom;
};

//--------------------------------------------------------------------------------------------------------------
/**
 * The NavLadder represents ladders in the Navigation Mesh, and their connections to adjacent NavAreas
 * @todo Deal with ladders that allow jumping off to areas in the middle
 */
class CNavLadder
{
public:
	CNavLadder( void ) :
		m_bottom(0.0f, 0.0f, 0.0f), m_top(0.0f, 0.0f, 0.0f), m_normal(0.0f, 0.0f, 0.0f), m_useableOrigin(0.0f, 0.0f, 0.0f)
	{
		m_connections.reserve(2); // ladders will generally have two connections
		// set an ID for interactive editing - loads will overwrite this
		m_id = m_nextID++;
		m_length = 0.0f;
		m_width = 0.0f;
		m_dir = NUM_DIRECTIONS;
		m_ladderType = MAX_LADDER_TYPES;
		m_bottomCount = 0;
		m_topCount = 0;
		m_attributes = 0;
		std::fill(m_teamBlocked.begin(), m_teamBlocked.end(), false);
		m_linkedTopLadder = static_cast<CNavLadder*>(nullptr);
		m_linkedBottomLadder = static_cast<CNavLadder*>(nullptr);
	}

	virtual ~CNavLadder();

	virtual void OnRoundRestart( void );			///< invoked when a game round restarts

	virtual void Save(std::fstream& filestream, uint32_t version);
	virtual void Load(CNavMesh* TheNavMesh, std::fstream& filestream, uint32_t version, uint32_t subversion);
	virtual void PostLoad(CNavMesh* TheNavMesh, uint32_t version);
	/**
	 * @brief Queries if this ladder is blocked for navigation.
	 * @param teamID Team index.
	 * @param queryLinkedLadders If true, also queries linked ladders
	 * @return True if blocked, false otherwise.
	 */
	virtual bool IsBlocked(int teamID, const bool queryLinkedLadders = true) const;

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

	bool IsConnected( const CNavArea *area, LadderDirectionType dir ) const;	///< returns true if given area is connected in given direction
	/**
	 * @brief Queries this ladder connected to the given area.
	 * @param area Nav area
	 * @param searchLinkedLadders If true, also search linked ladders.
	 * @return True if connected, false otherwise.
	 */
	bool IsConnected(const CNavArea* area, const bool searchLinkedLadders = true) const
	{
		for (auto& lc : m_connections)
		{
			CNavArea* other = lc.GetConnectedArea();

			if (area == other)
			{
				return true;
			}
		}

		if (searchLinkedLadders)
		{
			auto findarea = [&area](const CNavLadder* linked, const bool isTop) {
				if (linked->IsConnected(area, false))
				{
					return false;
				}

				return true;
			};

			// the search returns false when an area is found, which makes ForEachLinkedLadder return false.
			// if it returns true, no area was found
			return !(ForEachLinkedLadder(findarea));
		}


		return false;
	}
	// Returns true if this ladder is connected to the given ladder.
	bool IsConnected(const CNavLadder* ladder) const
	{
		if (GetLinkedTopLadder() == ladder || GetLinkedBottomLadder() == ladder)
		{
			return true;
		}

		return false;
	}

	void ConnectGeneratedLadder( float maxHeightAboveTopArea );		///< Connect a generated ladder to nav areas at the end of nav generation

	void ConnectTo( CNavArea *area );				///< connect this ladder to given area
	void Disconnect( CNavArea *area );				///< disconnect this ladder from given area
	// Links this ladder to another ladder
	void ConnectTo(CNavLadder* ladder)
	{
		if (ladder == this) { return; }

		if (ladder->m_top.z >= this->m_top.z)
		{
			SetLinkedTopLadder(ladder);
			ladder->SetLinkedBottomLadder(this);
			return;
		}

		SetLinkedBottomLadder(ladder);
		ladder->SetLinkedTopLadder(this);
	}
	// Disconnects this ladder of another ladder
	void Disconnect(CNavLadder* ladder)
	{
		if (ladder == this) { return; }

		if (GetLinkedTopLadder() == ladder)
		{
			SetLinkedTopLadder(nullptr);
		}

		if (GetLinkedBottomLadder() == ladder)
		{
			SetLinkedBottomLadder(nullptr);
		}
	}

	void OnSplit( CNavArea *original, CNavArea *alpha, CNavArea *beta );	///< when original is split into alpha and beta, update our connections
	void OnDestroyNotify( CNavArea *dead );			///< invoked when given area is going away
	void OnDestroyNotify(CNavLadder* ladder); // invoked when a ladder is being removed

	void DrawLadder(bool isSelected, bool isMarked, bool isEdit, bool isLinkedLadder = false) const;					///< Draws ladder and connections
	void DrawConnectedAreas( bool isEdit );				///< Draws connected areas

	// void UpdateDangling( void );					///< Checks if the ladder is dangling (bots cannot go up)

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

	const std::vector<LadderToAreaConnection>& GetConnections() const { return m_connections; }
	const LadderToAreaConnection* GetConnectionToArea(const CNavArea* area, const bool searchLinked = true) const;
	std::size_t GetConnectionsCount() const { return m_connections.size(); }
	std::size_t GetTopConnectionCount() const { return m_topCount; }
	std::size_t GetBottomConnectionCount() const { return m_bottomCount; }
	void GetConnectedAreas(std::vector<CNavArea*>& topAreas, std::vector<CNavArea*>& bottomAreas) const;

	// Clamps a Z coordinate to be between the ladder top and bottom Z
	inline float ClampZ(const float z) const
	{
		if (z > m_top.z)
		{
			return m_top.z;
		}

		if (z < m_bottom.z)
		{
			return m_bottom.z;
		}

		return z;
	}

	static void MergeLadders(CNavLadder* bottom, CNavLadder* top);

	const CNavLadder* GetLinkedTopLadder() const { return std::get<CNavLadder*>(m_linkedTopLadder); }
	const CNavLadder* GetLinkedBottomLadder() const { return std::get<CNavLadder*>(m_linkedBottomLadder); }
	/**
	 * @brief Updates the ladder blocked status for a given team.
	 * Note: This is static and is saved to the nav mesh file.
	 * @param teamID Team ID to update.
	 * @param status True if blocked, false if unblocked.
	 */
	void SetBlockedForTeam(int teamID, bool status)
	{
		if (teamID < 0 || teamID >= static_cast<int>(m_teamBlocked.size()))
		{
			std::fill(m_teamBlocked.begin(), m_teamBlocked.end(), status);
			return;
		}

		m_teamBlocked[teamID] = status;
	}

	void PrintTeamBlockedStatus() const;

	/**
	 * @brief Invokes a function on every linked ladder.
	 * @tparam F Function to invoke. bool (const CNavLadder* linked, const bool isTop). Return false to exit the function early.
	 * @param func Function to invoke.
	 * @return Returns false if the invoked function returned false, true otherwise.
	 */
	template <typename F>
	bool ForEachLinkedLadder(F& func) const
	{
		const CNavLadder* ladder = GetLinkedTopLadder();

		while (ladder != nullptr)
		{
			if (func(ladder, true) == false)
			{
				return false;
			}

			ladder = ladder->GetLinkedTopLadder();
		}

		ladder = GetLinkedBottomLadder();

		while (ladder != nullptr)
		{
			if (func(ladder, false) == false)
			{
				return false;
			}

			ladder = ladder->GetLinkedBottomLadder();
		}

		return true;
	}

protected:
	bool IsBlockedForTeam(int teamID) const
	{
		if (teamID < 0 || teamID >= static_cast<int>(m_teamBlocked.size()))
		{
			for (auto& blocked : m_teamBlocked)
			{
				if (blocked)
				{
					return true;
				}
			}

			return false;
		}

		return m_teamBlocked[teamID];
	}

	void SetLinkedTopLadder(CNavLadder* ladder) { if (ladder != this) { m_linkedTopLadder = ladder; }  }
	void SetLinkedBottomLadder(CNavLadder* ladder) { if (ladder != this) { m_linkedBottomLadder = ladder; } }

	bool IsConnectedViaTopLadder(const CNavArea* area) const
	{
		const CNavLadder* next = GetLinkedTopLadder();

		while (next != nullptr)
		{
			if (next->IsConnected(area, false))
			{
				return true;
			}

			next = GetLinkedTopLadder();
		}

		return false;
	}

	bool IsConnectedViaBottomLadder(const CNavArea* area) const
	{
		const CNavLadder* next = GetLinkedBottomLadder();

		while (next != nullptr)
		{
			if (next->IsConnected(area, false))
			{
				return true;
			}

			next = GetLinkedBottomLadder();
		}

		return false;
	}

private:
	friend class CNavMesh;
	void FindLadderEntity( void );

	static constexpr auto USABLE_LADDER_ENTITY_SEARCH_RANGE = 512.0f; // search range for the ladder entity
	static constexpr auto USABLE_LADDER_DISMOUNT_POINT_SEARCH_RANGE = 128.0f; // search range for the dismount points around the ladder

	std::vector<LadderToAreaConnection> m_connections;
	CHandle<CBaseEntity> m_ladderEntity;
	NavDirType m_dir;								///< which way the ladder faces (ie: surface normal of climbable side)
	Vector m_normal;								///< surface normal of the ladder surface (or Vector-ized m_dir, if the traceline fails)
	Vector m_useableOrigin;							/// If this is a useable ladder, this is the origin of the func_useableladder entity
	int m_attributes; // ladder attributes
	std::array<bool, NAV_TEAMS_ARRAY_SIZE> m_teamBlocked; // per team ladder blocked status
	std::variant<unsigned int, CNavLadder*> m_linkedTopLadder; // linked ladder above me
	std::variant<unsigned int, CNavLadder*> m_linkedBottomLadder; // linked ladder bewow me

	static unsigned int m_nextID;					///< used to allocate unique IDs
	unsigned int m_id;								///< unique area ID
	LadderType m_ladderType;
	std::size_t m_bottomCount;						// Number of bottom connections, used in path finding
	std::size_t m_topCount;							// Number of top connections, used in path finding

	bool HasTopLadderID() const { return std::holds_alternative<unsigned int>(m_linkedTopLadder); }
	bool HasBottomLadderID() const { return std::holds_alternative<unsigned int>(m_linkedBottomLadder); }
	unsigned int GetTopLadderID() const { return std::get<unsigned int>(m_linkedTopLadder); }
	unsigned int GetBottomLadderID() const { return std::get<unsigned int>(m_linkedBottomLadder); }
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
