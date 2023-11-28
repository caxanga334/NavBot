#ifndef SMNAV_BOT_BASE_PATH_H_
#define SMNAV_BOT_BASE_PATH_H_
#pragma once

#include <vector>
#include <stack>

#include <sdkports/sdk_timers.h>
#include <bot/basebot.h>
#include <mathlib/mathlib.h>
#include <navmesh/nav.h>
#include <navmesh/nav_mesh.h>
#include <navmesh/nav_pathfind.h>

class CNavArea;
class CNavLadder;
class CFuncElevator;

namespace AIPath
{
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
	virtual float operator()(CNavArea* toArea, CNavArea* fromArea, const CNavLadder* ladder, const CFuncElevator* elevator, float length) = 0;
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

	void Invalidate();

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
			// to-do: notify path failure
			return false;
		}

		constexpr float MaxDistanceToArea = 200.0f;
		CNavArea* goalArea = TheNavMesh->GetNearestNavArea(goal, MaxDistanceToArea, true, true);

		if (goalArea == startArea)
		{
			// to-do: build path from bot pos to goal pos
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
			return false; // no path
		}

		if (areaCount == 1)
		{
			// to-do: build path from bot pos to goal pos
			return true;
		}

		// the path is built from end to start, include the end position first
		if (pathBuildResult == true || includeGoalOnFailure == true)
		{
			CBasePathSegment* segment = CreateNewSegment();

			segment->area = closestArea;
			segment->goal = endPos;

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
			return false;
		}

		PostProcessPath();

		return pathBuildResult;
	}

	virtual void DrawFullPath(const float duration = 0.01f);

protected:
	virtual bool ProcessCurrentPath(CBaseBot* bot, const Vector& start);
	virtual bool ProcessGroundPath(CBaseBot* bot, const Vector& start, CBasePathSegment* from, CBasePathSegment* to, std::stack<PathInsertSegmentInfo>& pathinsert);
	virtual bool ProcessLaddersInPath(CBaseBot* bot, CBasePathSegment* from, CBasePathSegment* to, std::stack<PathInsertSegmentInfo>& pathinsert);
	virtual bool ProcessPathJumps(CBaseBot* bot, CBasePathSegment* from, CBasePathSegment* to, std::stack<PathInsertSegmentInfo>& pathinsert);
	virtual void ComputeAreaCrossing(CBaseBot* bot, CNavArea* from, const Vector& frompos, CNavArea* to, NavDirType dir, Vector* crosspoint);
	virtual void PostProcessPath();

private:
	std::vector<CBasePathSegment*> m_segments;
	IntervalTimer m_ageTimer;
};

inline void CPath::Invalidate()
{
	for (auto segment : m_segments)
	{
		delete segment;
	}

	m_segments.clear();
	m_ageTimer.Invalidate();
}

#endif // !SMNAV_BOT_BASE_PATH_H_
