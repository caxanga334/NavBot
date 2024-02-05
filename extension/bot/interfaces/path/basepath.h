#ifndef SMNAV_BOT_BASE_PATH_H_
#define SMNAV_BOT_BASE_PATH_H_
#pragma once

#include <vector>
#include <stack>
#include <algorithm>

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

		MAX_SEGMENT_TYPES
	};

	enum ResultType
	{
		COMPLETE_PATH = 0, // Full path from point A to B
		PARTIAL_PATH, // Partial path, doesn't reach the end goal
		NO_PATH // No path at all
	};
}

// Abstract class for custom path finding costs
class IPathCost
{
public:
	/**
	 * @brief Computes the cost to go from 'fromArea' to 'toArea'
	 * @param toArea Current nav area
	 * @param fromArea Nav area the bot will be moving from, can be NULL for the first area
	 * @param ladder Ladder the bot will be using
	 * @param elevator Not used, to be replaced when proper elevator supported is added to the extension version of the nav mesh
	 * @param length Path length
	 * @return path cost
	*/
	virtual float operator()(CNavArea* toArea, CNavArea* fromArea, const CNavLadder* ladder, const CFuncElevator* elevator, float length) const = 0;
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
};

class PathInsertSegmentInfo
{
public:
	PathInsertSegmentInfo(CBasePathSegment* Seg, CBasePathSegment* newSeg, const bool afterseg = true) :
		after(afterseg)
	{
		this->Seg = Seg;
		this->newSeg = newSeg;
	}

	const bool after;
	CBasePathSegment* Seg; // the segment will be added after this one
	CBasePathSegment* newSeg; // new segment to be added
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

	virtual CBasePathSegment* CreateNewSegment() { return new CBasePathSegment; }

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
			CBasePathSegment* segment = CreateNewSegment();

			segment->area = closestArea;
			segment->goal = endPos;
			segment->type = AIPath::SegmentType::SEGMENT_GROUND;

			m_segments.push_back(segment);
		}

		// construct the path segments
		// Reminder: areas are added to the back of the segment vector, the first area is the goal area.
		for (CNavArea* area = closestArea; area != nullptr; area = area->GetParent())
		{
			CBasePathSegment* segment = CreateNewSegment();

			segment->area = area;
			segment->how = area->GetParentHow();

			m_segments.push_back(segment);
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

	bool BuildTrivialPath(const Vector& start, const Vector& goal);

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

	virtual const CBasePathSegment* GetFirstSegment() const;
	virtual const CBasePathSegment* GetLastSegment() const;
	virtual const CBasePathSegment* GetNextSegment(const CBasePathSegment* current) const;
	virtual const CBasePathSegment* GetPriorSegment(const CBasePathSegment* current) const;
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

protected:
	virtual bool ProcessCurrentPath(CBaseBot* bot, const Vector& start);
	virtual bool ProcessGroundPath(CBaseBot* bot, const Vector& start, CBasePathSegment* from, CBasePathSegment* to, std::stack<PathInsertSegmentInfo>& pathinsert);
	virtual bool ProcessLaddersInPath(CBaseBot* bot, CBasePathSegment* from, CBasePathSegment* to, std::stack<PathInsertSegmentInfo>& pathinsert);
	virtual bool ProcessPathJumps(CBaseBot* bot, CBasePathSegment* from, CBasePathSegment* to, std::stack<PathInsertSegmentInfo>& pathinsert);
	virtual void ComputeAreaCrossing(CBaseBot* bot, CNavArea* from, const Vector& frompos, CNavArea* to, NavDirType dir, Vector* crosspoint);
	virtual void PostProcessPath();
	
	inline std::vector<CBasePathSegment*>& GetAllSegments() { return m_segments; }

private:
	std::vector<CBasePathSegment*> m_segments;
	IntervalTimer m_ageTimer;
	PathCursor m_cursor;
	float m_cursorPos;

	void DrawSingleSegment(const Vector& v1, const Vector& v2, AIPath::SegmentType type, const float duration);
	void Drawladder(const CNavLadder* ladder, AIPath::SegmentType type, const float duration);
};

inline void CPath::Invalidate()
{
	for (auto segment : m_segments)
	{
		delete segment;
	}

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

#endif // !SMNAV_BOT_BASE_PATH_H_
