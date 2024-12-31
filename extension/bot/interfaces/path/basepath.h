#ifndef SMNAV_BOT_BASE_PATH_H_
#define SMNAV_BOT_BASE_PATH_H_
#pragma once

#include <vector>
#include <stack>
#include <iterator>
#include <algorithm>
#include <memory>

#include <sdkports/sdk_timers.h>
#include <bot/basebot.h>
#include <navmesh/nav.h>
#include <navmesh/nav_mesh.h>
#include <navmesh/nav_pathfind.h>

class CNavArea;
class CNavLadder;
class CFuncElevator;

namespace AIPath
{
	// Path segment type, tell bots how they should traverse the segment
	enum SegmentType
	{
		SEGMENT_GROUND = 0, // Walking over solid ground
		SEGMENT_DROP_FROM_LEDGE, // Dropping down from a ledge/cliff 
		SEGMENT_CLIMB_UP, // Climbing over an obstacle
		SEGMENT_JUMP_OVER_GAP, // Jumping over a gap/hole on the ground
		SEGMENT_LADDER_UP, // Going up a ladder
		SEGMENT_LADDER_DOWN, // Going down a ladder
		SEGMENT_CLIMB_DOUBLE_JUMP, // Climbing over an obstacle that requires a double jump
		SEGMENT_BLAST_JUMP, // Blast/Rocket jump to the next segment
		SEGMENT_ELEVATOR,// Use an elevator

		MAX_SEGMENT_TYPES
	};

	enum ResultType
	{
		COMPLETE_PATH = 0, // Full path from point A to B
		PARTIAL_PATH, // Partial path, doesn't reach the end goal
		NO_PATH // No path at all
	};

	inline const char* SegmentTypeToString(SegmentType type)
	{
		switch (type)
		{
		case AIPath::SEGMENT_GROUND:
			return "GROUND";
		case AIPath::SEGMENT_DROP_FROM_LEDGE:
			return "DROP_FROM_LEDGE";
		case AIPath::SEGMENT_CLIMB_UP:
			return "CLIMB_UP";
		case AIPath::SEGMENT_JUMP_OVER_GAP:
			return "JUMP_OVER_GAP";
		case AIPath::SEGMENT_LADDER_UP:
			return "LADDER_UP";
		case AIPath::SEGMENT_LADDER_DOWN:
			return "LADDER_DOWN";
		case AIPath::SEGMENT_CLIMB_DOUBLE_JUMP:
			return "CLIMB_DOUBLE_JUMP";
		case AIPath::SEGMENT_BLAST_JUMP:
			return "BLAST_JUMP";
		case SEGMENT_ELEVATOR:
			return "ELEVATOR";
		case AIPath::MAX_SEGMENT_TYPES:
			return "MAX_SEGMENT_TYPES";
		default:
			return "ERROR";
		}
	}
}

// Abstract class for custom path finding costs
class IPathCost
{
public:
	/* This is the original nav mesh cost function used by NavAreaBuildPath */

	/**
	 * @brief Computes the cost to go from 'fromArea' to 'toArea'
	 * @param toArea Current nav area
	 * @param fromArea Nav area the bot will be moving from, can be NULL for the first area
	 * @param ladder Ladder the bot will be using
	 * @param link If 'how' refers to GO_OFF_MESH_CONNECTION, this is the link or NULL if not using special links
	 * @param elevator Not used, to be replaced when proper elevator supported is added to the extension version of the nav mesh
	 * @param length Path length
	 * @return path cost
	*/
	virtual float operator()(CNavArea* toArea, CNavArea* fromArea, const CNavLadder* ladder, const NavOffMeshConnection* link, const CNavElevator* elevator, float length) const = 0;
};

// A path segment is a single 'node' that the bot uses to move. The path is a list of segments and the bot follows these segments
class CBasePathSegment
{
public:
	CBasePathSegment()
	{
		area = nullptr;
		ladder = nullptr;
		how = NUM_TRAVERSE_TYPES;
		type = AIPath::SegmentType::SEGMENT_GROUND;
		length = 0.0f;
		distance = 0.0f;
		curvature = 0.0f;
		portalhalfwidth = 0.0f;
	}

	virtual ~CBasePathSegment() {}

	CNavArea* area; // The area that is part of this segment
	Vector goal; // Movement goal position for this segment
	NavTraverseType how; // How to approach this segment
	const CNavLadder* ladder; // If 'how' mentions a ladder, this is the ladder
	AIPath::SegmentType type; // How to traverse this segment
	Vector forward; // Segment unit vector
	float length; // Segment length
	float distance; // Distance between this segment and the start position
	float curvature; // How much this path segment 'curves' (ranges from 0 (0º) to 1 (180º)
	Vector portalcenter; // Segment portal center position
	float portalhalfwidth; // Portal's half width

	virtual void CopySegment(CBasePathSegment* other)
	{
		this->area = other->area;
		this->goal = other->goal;
		this->how = other->how;
		this->ladder = other->ladder;
		this->type = other->type;
		this->forward = other->forward;
		this->distance = other->distance;
		this->curvature = other->curvature;
		this->portalcenter = other->portalcenter;
		this->portalhalfwidth = other->portalhalfwidth;
	}

	inline void CopySegment(const std::shared_ptr<CBasePathSegment>& other)
	{
		CopySegment(other.get());
	}
};

class PathInsertSegmentInfo
{
public:
	PathInsertSegmentInfo(std::shared_ptr<CBasePathSegment> Seg, std::shared_ptr<CBasePathSegment> newSeg, const bool afterseg = true) :
		after(afterseg)
	{
		this->Seg = Seg;
		this->newSeg = newSeg;
	}

	const bool after;
	std::shared_ptr<CBasePathSegment> Seg; // the segment will be added after this one
	std::shared_ptr<CBasePathSegment> newSeg; // new segment to be added
};

// good reference for valve nav mesh pathing
// - https://github.com/ValveSoftware/halflife/blob/master/game_shared/bot/nav_path.cpp
// - https://github.com/ValveSoftware/halflife/blob/master/game_shared/bot/nav_path.h

// Base path
class CPath
{
public:
	CPath();
	virtual ~CPath();

	class PathCursor
	{
	public:
		Vector position;
		Vector forward;
		float curvature;
		const CBasePathSegment* segment; // segment before the cursor position
		bool outdated; // true if the cursor was changed without updating

		PathCursor()
		{
			curvature = 0.0f;
			segment = nullptr;
			outdated = true;
		}

		inline void Invalidate()
		{
			this->position = vec3_invalid;
			this->forward = vec3_invalid;
			this->curvature = 0.0f;
			this->segment = nullptr;
			this->outdated = true;
		}
	};

	virtual void Invalidate();

protected:
	virtual CBasePathSegment* AllocNewSegment() const { return new CBasePathSegment; }

	std::shared_ptr<CBasePathSegment> CreateNewSegment() { return std::shared_ptr<CBasePathSegment>(AllocNewSegment()); }

public:

	/**
	 * @brief Finds a path via A* search
	 * @tparam CostFunction Path cost function
	 * @param bot The bot that will traverse this path
	 * @param goal Path goal position
	 * @param costFunc cost function
	 * @param maxPathLength Maximum path length
	 * @param includeGoalOnFailure if true, a segment to the goal position will be added even if it failed to find a path
	 * @return true if a path is found
	*/
	template <typename CostFunction>
	bool ComputePathToPosition(CBaseBot* bot, const Vector& goal, CostFunction& costFunc, const float maxPathLength = 0.0f, const bool includeGoalOnFailure = false)
	{
		Invalidate();

		auto start = bot->GetAbsOrigin();
		auto startArea = bot->GetLastKnownNavArea();

		if (startArea == nullptr)
		{
			OnPathChanged(bot, AIPath::ResultType::NO_PATH);
			return false;
		}

		constexpr float MaxDistanceToArea = 200.0f;
		CNavArea* goalArea = TheNavMesh->GetNearestNavArea(goal, MaxDistanceToArea, true, true);

		if (goalArea == startArea)
		{
			BuildTrivialPath(start, goal);
			OnPathChanged(bot, AIPath::ResultType::COMPLETE_PATH);
			return true;
		}

		Vector endPos = goal;
		if (goalArea)
		{
			endPos.z = goalArea->GetZ(endPos);
		}
		else
		{
			TheNavMesh->GetGroundHeight(endPos, &endPos.z);
		}

		// Compute the shorest path
		CNavArea* closestArea = nullptr;
		bool pathBuildResult = NavAreaBuildPath(startArea, goalArea, &goal, costFunc, &closestArea, maxPathLength, bot->GetCurrentTeamIndex());

		// count the number of areas in the path
		int areaCount = 0;

		for (CNavArea* area = closestArea; area != nullptr; area = area->GetParent())
		{
			areaCount++;

			if (area == startArea)
			{
				break;
			}
		}

		if (areaCount == 0)
		{
			OnPathChanged(bot, AIPath::ResultType::NO_PATH);
			return false; // no path
		}

		if (areaCount == 1)
		{
			BuildTrivialPath(start, goal);
			OnPathChanged(bot, AIPath::ResultType::COMPLETE_PATH);
			return true;
		}

		// the path is built from end to start, include the end position first
		if (pathBuildResult == true || includeGoalOnFailure == true)
		{
			std::shared_ptr<CBasePathSegment> segment = CreateNewSegment();

			segment->area = closestArea;
			segment->goal = endPos;
			segment->type = AIPath::SegmentType::SEGMENT_GROUND;

			m_segments.push_back(std::move(segment));
		}

		// construct the path segments
		// Reminder: areas are added to the back of the segment vector, the first area is the goal area.
		for (CNavArea* area = closestArea; area != nullptr; area = area->GetParent())
		{
			std::shared_ptr<CBasePathSegment> segment = CreateNewSegment();

			segment->area = area;
			segment->how = area->GetParentHow();

			m_segments.push_back(std::move(segment));
		}

		// Place the path start at the vector start
		std::reverse(m_segments.begin(), m_segments.end());

		if (ProcessCurrentPath(bot, start) == false)
		{
			Invalidate(); // destroy the path so IsValid returns false
			OnPathChanged(bot, AIPath::ResultType::NO_PATH);
			return false;
		}

		PostProcessPath();

		if (pathBuildResult == true)
		{
			OnPathChanged(bot, AIPath::ResultType::COMPLETE_PATH);
		}
		else
		{
			OnPathChanged(bot, AIPath::ResultType::PARTIAL_PATH);
		}

		return pathBuildResult;
	}

	/**
	 * @brief Checks if the bot can reach a specific goal.
	 * @tparam CostFunction Nav path cost functor
	 * @param bot Bot to test.
	 * @param goal Goal position.
	 * @param costFunc Path cost functor.
	 * @param maxPathLength Maximum path length.
	 * @return true if reachable, false otherwise.
	 */
	template <typename CostFunction>
	bool IsReachable(CBaseBot* bot, const Vector& goal, CostFunction& costFunc, const float maxPathLength = 0.0f)
	{
		auto start = bot->GetAbsOrigin();
		auto startArea = bot->GetLastKnownNavArea();

		if (startArea == nullptr)
		{
			return false;
		}

		constexpr float MaxDistanceToArea = 200.0f;
		CNavArea* goalArea = TheNavMesh->GetNearestNavArea(goal, MaxDistanceToArea, true, true);

		if (goalArea == startArea)
		{
			return true;
		}

		Vector endPos = goal;
		if (goalArea)
		{
			endPos.z = goalArea->GetZ(endPos);
		}
		else
		{
			TheNavMesh->GetGroundHeight(endPos, &endPos.z);
		}

		return NavAreaBuildPath(startArea, goalArea, &goal, costFunc, nullptr, maxPathLength, bot->GetCurrentTeamIndex());
	}

	virtual void Draw(const CBasePathSegment* start, const float duration = 0.1f);
	virtual void DrawFullPath(const float duration = 0.1f);
	virtual float GetPathLength() const;

	// How many seconds since this path was built
	virtual float GetAge() const { return m_ageTimer.GetElapsedTime(); }
	// Checks if the path is valid.
	// Note that a path can be valid but not reach the goal.
	// To known if the goal can be reached, use the result of the ComputePath* functions.
	virtual bool IsValid() const { return m_segments.size() > 0; }

	virtual void OnPathChanged(CBaseBot* bot, AIPath::ResultType result) {}

	virtual const Vector& GetStartPosition() const;
	virtual const Vector& GetEndPosition() const;

	const CBasePathSegment* GetFirstSegment() const;
	const CBasePathSegment* GetLastSegment() const;
	const CBasePathSegment* GetNextSegment(const CBasePathSegment* current) const;
	const CBasePathSegment* GetPriorSegment(const CBasePathSegment* current) const;
	virtual const CBasePathSegment* GetGoalSegment() const;

	enum SeekType
	{
		SEEK_ENTIRE_PATH = 0,
		SEEK_AHEAD
	};

	virtual void MoveCursorToClosestPosition(const Vector& pos, SeekType type = SEEK_ENTIRE_PATH, float alongLimit = 0.0f);

	virtual const PathCursor& GetCursorData();
	virtual bool IsCursorOutdated() const { return m_cursor.outdated; }

	enum MoveCursorType
	{
		PATH_ABSOLUTE_DISTANCE = 0,
		PATH_RELATIVE_DISTANCE
	};

	virtual void MoveCursorToStart(void);
	virtual void MoveCursorToEnd(void);
	virtual void MoveCursor(float value, MoveCursorType type = PATH_ABSOLUTE_DISTANCE);
	virtual float GetCursorPosition(void) const;

	/**
	 * @brief Runs a function on every path segment.
	 * @tparam T Path Segment class.
	 * @tparam F Function. bool (const T* segment)
	 * @param functor Function to run, return false to end loop early.
	 */
	template <typename T, typename F>
	void ForEveryPathSegment(F functor)
	{
		for (auto& segptr : m_segments)
		{
			const T* segment = static_cast<T*>(segptr.get());

			if (functor(segment) == false)
			{
				return;
			}
		}
	}

protected:
	virtual bool ProcessCurrentPath(CBaseBot* bot, const Vector& start);
	virtual bool ProcessGroundPath(CBaseBot* bot, const size_t index, const Vector& start, std::shared_ptr<CBasePathSegment>& from, std::shared_ptr<CBasePathSegment>& to, std::stack<PathInsertSegmentInfo>& pathinsert);
	virtual bool ProcessLaddersInPath(CBaseBot* bot, std::shared_ptr<CBasePathSegment>& from, std::shared_ptr<CBasePathSegment>& to, std::stack<PathInsertSegmentInfo>& pathinsert);
	virtual bool ProcessElevatorsInPath(CBaseBot* bot, std::shared_ptr<CBasePathSegment>& from, std::shared_ptr<CBasePathSegment>& to, std::stack<PathInsertSegmentInfo>& pathinsert);
	virtual bool ProcessPathJumps(CBaseBot* bot, std::shared_ptr<CBasePathSegment>& from, std::shared_ptr<CBasePathSegment>& to, std::stack<PathInsertSegmentInfo>& pathinsert);
	virtual bool ProcessOffMeshConnectionsInPath(CBaseBot* bot, const size_t index, std::shared_ptr<CBasePathSegment>& from, std::shared_ptr<CBasePathSegment>& to, std::stack<PathInsertSegmentInfo>& pathinsert);
	virtual void ComputeAreaCrossing(CBaseBot* bot, CNavArea* from, const Vector& frompos, CNavArea* to, NavDirType dir, Vector* crosspoint);
	virtual void PostProcessPath();
	
	inline std::vector<std::shared_ptr<CBasePathSegment>>& GetAllSegments() { return m_segments; }
	bool BuildTrivialPath(const Vector& start, const Vector& goal);

private:
	std::vector<std::shared_ptr<CBasePathSegment>> m_segments;
	IntervalTimer m_ageTimer;
	PathCursor m_cursor;
	float m_cursorPos;

	void DrawSingleSegment(const Vector& v1, const Vector& v2, AIPath::SegmentType type, const float duration);
	void Drawladder(const CNavLadder* ladder, AIPath::SegmentType type, const float duration);
};

inline void CPath::Invalidate()
{
	m_segments.clear();
	m_ageTimer.Invalidate();
	m_cursorPos = 0.0f;
	m_cursor.Invalidate();
}

inline void CPath::MoveCursorToStart(void)
{
	m_cursorPos = 0.0f;
	m_cursor.outdated = true;
}

inline void CPath::MoveCursorToEnd(void)
{
	m_cursorPos = GetPathLength();
	m_cursor.outdated = true;
}

inline void CPath::MoveCursor(float value, MoveCursorType type)
{
	if (type == PATH_ABSOLUTE_DISTANCE)
	{
		m_cursorPos = value;
	}
	else
	{
		m_cursorPos += value;
	}

	if (m_cursorPos < 0.0f)
	{
		m_cursorPos = 0.0f;
	}
	else if (m_cursorPos > GetPathLength())
	{
		m_cursorPos = GetPathLength();
	}

	m_cursor.outdated = true;
}

inline float CPath::GetCursorPosition(void) const
{
	return m_cursorPos;
}

inline const CBasePathSegment* CPath::GetFirstSegment() const
{
	if (m_segments.size() == 0)
	{
		return nullptr;
	}

	return m_segments.begin()->get();
}

inline const CBasePathSegment* CPath::GetLastSegment() const
{
	if (m_segments.size() == 0)
	{
		return nullptr;
	}

	return std::prev(m_segments.end())->get();
}

inline const CBasePathSegment* CPath::GetNextSegment(const CBasePathSegment* current) const
{
	if (m_segments.size() == 0)
	{
		return nullptr;
	}

	auto it = std::find_if(m_segments.begin(), m_segments.end(), [&current](const std::shared_ptr<CBasePathSegment> object) {
		if (object.get() == current)
		{
			return true;
		}

		return false;
	});

	if (it == m_segments.end())
	{
		return nullptr; // not found
	}

	it = std::next(it);

	if (it == m_segments.end())
	{
		return nullptr;
	}

	return it->get();
}

inline const CBasePathSegment* CPath::GetPriorSegment(const CBasePathSegment* current) const
{
	if (m_segments.size() == 0)
	{
		return nullptr;
	}

	auto it = std::find_if(m_segments.begin(), m_segments.end(), [&current](const std::shared_ptr<CBasePathSegment> object) {
		if (object.get() == current)
		{
			return true;
		}

		return false;
	});

	if (it == m_segments.end())
	{
		return nullptr; // not found
	}

	if (it == m_segments.begin())
	{
		return nullptr; // current segment is the first segment, no previous segment
	}

	it = std::prev(it);

	return it->get();
}

#endif // !SMNAV_BOT_BASE_PATH_H_
