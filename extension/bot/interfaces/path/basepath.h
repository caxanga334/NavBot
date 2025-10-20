#ifndef SMNAV_BOT_BASE_PATH_H_
#define SMNAV_BOT_BASE_PATH_H_
#pragma once

#include <vector>
#include <stack>
#include <iterator>
#include <algorithm>
#include <memory>

#include "path_shareddefs.h"
#include <sdkports/sdk_timers.h>
#include <bot/basebot.h>
#include <navmesh/nav.h>
#include <navmesh/nav_mesh.h>
#include <navmesh/nav_pathfind.h>

class CNavArea;
class CNavLadder;
class CNavElevator;
class CFuncElevator;
class NavOffMeshConnection;

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

	virtual void SetRouteType(RouteType type) = 0;
	virtual RouteType GetRouteType() const = 0;
};

// A path segment is a single 'node' that the bot uses to move. The path is a list of segments and the bot follows these segments
class CBasePathSegment
{
public:
	CBasePathSegment() :
		forward(0.0f, 0.0f, 0.0f), goal(0.0f, 0.0f, 0.0f), portalcenter(0.0f, 0.0f, 0.0f)
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

		PathCursor() :
			forward(0.0f, 0.0f, 0.0f), position(0.0f, 0.0f, 0.0f)
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

	// Invalidates the current path
	virtual void Invalidate();
	// Mark this path for repathing
	virtual void ForceRepath();
	// Invalidates the current path and resets the repath timer
	inline void InvalidateAndRepath()
	{
		Invalidate();
		ForceRepath();
	}
	// Returns the path destination since the last ComputePath call.
	inline const Vector& GetPathDestination() const { return m_destination; }

protected:
	virtual CBasePathSegment* AllocNewSegment() const { return new CBasePathSegment; }

	std::shared_ptr<CBasePathSegment> CreateNewSegment() { return std::shared_ptr<CBasePathSegment>(AllocNewSegment()); }

public:

	// Maximum distance when searching for the nearest nav area during path computation.
	static constexpr float PATH_GOAL_MAX_DISTANCE_TO_AREA = 200.0f;

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

		if (!bot->GetMovementInterface()->IsPathingAllowed())
		{
			// Return true since we a skipping path calculations and not actually failing due to no path found.
			m_repathTimer.Invalidate();
			OnPathChanged(bot, AIPath::ResultType::NO_PATH);
			return true;
		}

		const Vector start = bot->GetAbsOrigin();
		CNavArea* startArea = bot->GetLastKnownNavArea();
		const bool isDebugging = bot->IsDebugging(BOTDEBUG_PATH);
		m_destination = goal;

		if (startArea == nullptr)
		{
			OnPathChanged(bot, AIPath::ResultType::NO_PATH);
			m_repathTimer.Invalidate();

			if (isDebugging)
			{
				bot->DebugPrintToConsole(255, 0, 0, "%s COMPUTE PATH ERROR: NULL START AREA! <%g %g %g>\n", bot->GetDebugIdentifier(), start.x, start.y, start.z);
			}

			return false;
		}

		CNavArea* goalArea = TheNavMesh->GetNearestNavArea(goal, PATH_GOAL_MAX_DISTANCE_TO_AREA, true, true);

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

		if (closestArea)
		{
			SetTravelDistance(closestArea->GetTotalCost());
		}

		if (isDebugging && !pathBuildResult)
		{
			bot->DebugPrintToConsole(255, 0, 0, "%s COMPUTE PATH: FAILED TO BUILD A FULL PATH TO GOAL FROM <%g %g %g> TO <%g %g %g> (%g) \n", 
				bot->GetDebugIdentifier(), start.x, start.y, start.z, goal.x, goal.y, goal.z, endPos.z);
		}

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
		if (pathBuildResult || includeGoalOnFailure)
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

		if (!ProcessCurrentPath(bot, start))
		{
			Invalidate(); // destroy the path so IsValid returns false
			OnPathChanged(bot, AIPath::ResultType::NO_PATH);
			return false;
		}

		PostProcessPath();

		if (pathBuildResult)
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

	// The first segment on the path
	const CBasePathSegment* GetFirstSegment() const;
	// The last segment on the path
	const CBasePathSegment* GetLastSegment() const;
	const CBasePathSegment* GetNextSegment(const CBasePathSegment* current) const;
	const CBasePathSegment* GetPriorSegment(const CBasePathSegment* current) const;
	// The segment the bot is trying to reach
	virtual const CBasePathSegment* GetGoalSegment() const { return nullptr; /* implemented by derived classes */ }

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
	void ForEveryPathSegment(F& functor)
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

	// Returns the total travel distance since the last ComputePath call.
	const float GetTravelDistance() const { return m_travelDistance; }
	const CountdownTimer& GetRepathTimer() const { return m_repathTimer; }
	static constexpr float DEFAULT_REPATH_DURATION = 2.0f;
	void StartRepathTimer(const float duration = DEFAULT_REPATH_DURATION) { m_repathTimer.Start(duration); }
	const bool NeedsRepath() const { return m_repathTimer.IsElapsed(); }

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

	void SetTravelDistance(const float dist) { m_travelDistance = dist; }

	CountdownTimer* InternalGetRepathTimer() { return &m_repathTimer; }

private:
	std::vector<std::shared_ptr<CBasePathSegment>> m_segments;
	IntervalTimer m_ageTimer;
	PathCursor m_cursor;
	float m_cursorPos;
	Vector m_destination; // 'Goal' position of the last ComputePath call
	float m_travelDistance; // Travel distance between start and goal from the last ComputePath Call
	CountdownTimer m_repathTimer;

	void DrawSingleSegment(const Vector& v1, const Vector& v2, AIPath::SegmentType type, const float duration);
	void Drawladder(const CNavLadder* ladder, AIPath::SegmentType type, const float duration);
};

inline void CPath::Invalidate()
{
	m_segments.clear();
	m_ageTimer.Invalidate();
	m_cursorPos = 0.0f;
	m_cursor.Invalidate();
	m_destination = vec3_origin;
}

inline void CPath::ForceRepath()
{
	m_repathTimer.Invalidate();
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
