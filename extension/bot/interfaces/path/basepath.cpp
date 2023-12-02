#include <extension.h>
#include <bot/interfaces/playercontrol.h>
#include <bot/interfaces/movement.h>
#include <manager.h>
#include <mods/basemod.h>
#include <util/UtilTrace.h>
#include <sdkports/debugoverlay_shared.h>

#include "basepath.h"

extern CExtManager* extmanager;

CPath::CPath() :
	m_ageTimer()
{
	m_segments.reserve(512);
}

CPath::~CPath()
{
	for (auto segment : m_segments)
	{
		delete segment;
	}

	m_segments.clear();
}

bool CPath::BuildTrivialPath(const Vector& start, const Vector& goal)
{
	constexpr float NAV_MAX_DIST = 256.0f;

	CNavArea* startArea = TheNavMesh->GetNearestNavArea(start, NAV_MAX_DIST, true, true);
	CNavArea* goalArea = TheNavMesh->GetNearestNavArea(goal, NAV_MAX_DIST, true, true);

	if (startArea == nullptr || goalArea == nullptr)
		return false;

	Invalidate(); // destroy any path that exists

	CBasePathSegment* startSeg = CreateNewSegment();
	CBasePathSegment* endSeg = CreateNewSegment();

	startSeg->area = startArea;
	startSeg->goal = start;
	startSeg->type = AIPath::SegmentType::SEGMENT_GROUND;
	startSeg->forward = goal - start;
	startSeg->length = startSeg->forward.NormalizeInPlace();
	endSeg->area = goalArea;
	endSeg->goal = goal;
	endSeg->type = AIPath::SegmentType::SEGMENT_GROUND;
	endSeg->forward = startSeg->forward;
	endSeg->distance = startSeg->length;

	m_segments.push_back(startSeg);
	m_segments.push_back(endSeg);
	m_ageTimer.Start();

	return true;
}

void CPath::DrawFullPath(const float duration)
{
#ifdef SMNAV_DEBUG // only draw path when debug is enabled
	bool isstart = true;
	Vector v1, v2;

	constexpr float ARROW_WIDTH = 4.0f;

	for (auto segment : m_segments)
	{
		if (isstart == true)
		{
			v1 = segment->goal; // init positions
			isstart = false;
			continue;
		}

		v2 = segment->goal; // set second point before drawing

		switch (segment->type)
		{
		case AIPath::SegmentType::SEGMENT_DROP_FROM_LEDGE:
		{
			NDebugOverlay::VertArrow(v1, v2, ARROW_WIDTH, 0, 0, 255, 255, true, duration);
			break;
		}
		case AIPath::SegmentType::SEGMENT_CLIMB_UP:
		{
			NDebugOverlay::VertArrow(v1, v2, ARROW_WIDTH, 255, 0, 100, 255, true, duration);
			break;
		}
		case AIPath::SegmentType::SEGMENT_JUMP_OVER_GAP:
		{
			NDebugOverlay::HorzArrow(v1, v2, ARROW_WIDTH, 0, 200, 255, 255, true, duration);
			break;
		}
		case AIPath::SegmentType::SEGMENT_LADDER_UP:
		case AIPath::SegmentType::SEGMENT_LADDER_DOWN:
		{
			NDebugOverlay::HorzArrow(v1, v2, ARROW_WIDTH, 200, 255, 0, 255, true, duration);
			break;
		}
		default:
		{
			NDebugOverlay::HorzArrow(v1, v2, ARROW_WIDTH, 0, 255, 0, 255, true, duration);
			break;
		}
		}

		v1 = segment->goal; // set v1 to the current segment position
	}

#endif // SMNAV_DEBUG // only draw path when debug is enabled
}

// Gets the total length of the current path.
float CPath::GetPathLength() const
{
	float length = 0.0f;

	for (auto segment : m_segments)
	{
		length += segment->length;
	}

	return length;
}

/**
 * @brief Analyze the current path
 * @param bot Bot that will use this path
 * @param start Path start position
 * @return true if the path is valid for use
*/
bool CPath::ProcessCurrentPath(CBaseBot* bot, const Vector& start)
{
	IPlayerController* input = bot->GetControlInterface();
	IMovement* mover = bot->GetMovementInterface();

	auto startSeg = m_segments[0];

	// If the starting area is inside the start area, then use it
	if (startSeg->area->Contains(start))
	{
		startSeg->goal = start;
	}
	else
	{
		// otherwise move the path start to the nav area center
		startSeg->goal = startSeg->area->GetCenter();
	}

	// Enforce these
	startSeg->ladder = nullptr;
	startSeg->how = NUM_TRAVERSE_TYPES;
	startSeg->type = AIPath::SegmentType::SEGMENT_GROUND;

	std::stack<PathInsertSegmentInfo> insertstack;

	bool failed = false;

	// first iteration
	for (size_t i = 1; i < m_segments.size(); i++)
	{
		auto to = m_segments[i];
		auto from = m_segments[i - 1];

		switch (to->how)
		{
		case GO_NORTH:
		case GO_EAST:
		case GO_SOUTH:
		case GO_WEST:
			if (ProcessGroundPath(bot, start, from, to, insertstack) == false)
			{
				failed = true;
			}
			break;
		case GO_LADDER_UP:
		case GO_LADDER_DOWN:
			if (ProcessLaddersInPath(bot, from, to, insertstack) == false)
			{
				failed = true;
			}
			break;
		default: // other types are not supported right now
			break;
		}

		if (failed == true)
			break; // exit loop
	}

	if (failed == false)
	{
		// Second iteration, handle jump and climbing
		for (size_t i = 1; i < m_segments.size(); i++)
		{
			auto to = m_segments[i];
			auto from = m_segments[i - 1];

			if (from->how != NUM_TRAVERSE_TYPES && from->how > GO_WEST)
				continue;

			if (to->how > GO_WEST || to->type != AIPath::SegmentType::SEGMENT_GROUND)
				continue;

			if (ProcessPathJumps(bot, from, to, insertstack) == false)
			{
				failed = true;
				break;
			}
		}
	}

	// Path process failed, clean up insert stack and exit
	if (failed == true)
	{
		while (insertstack.empty() == false)
		{
			auto& insert = insertstack.top();
			delete insert.newSeg;
			insertstack.pop();
			return false;
		}
	}

	// process insert stack and add any new needed segments into the path
	while (insertstack.empty() == false)
	{
		auto& insert = insertstack.top(); // get top element
		auto seg = insert.Seg;
		auto it = std::find(m_segments.begin(), m_segments.end(), seg);

		if (it != m_segments.end()) // got iterator of where we need to insert
		{
			if (insert.after == true)
			{
				it++; // by default, it will be added before the element of the iterator
			}

			m_segments.insert(it, insert.newSeg);
		}
		else
		{
			delete insert.newSeg;
		}

		insertstack.pop(); // remove top element
	}


	return true;
}

bool CPath::ProcessGroundPath(CBaseBot* bot, const Vector& start, CBasePathSegment* from, CBasePathSegment* to, std::stack<PathInsertSegmentInfo>& pathinsert)
{
	IMovement* mover = bot->GetMovementInterface();

	from->area->ComputePortal(to->area, static_cast<NavDirType>(to->how), &to->portalcenter, &to->portalhalfwidth);

	// Compute the point the bot will cross to the next area
	ComputeAreaCrossing(bot, from->area, from->goal, to->area, static_cast<NavDirType>(to->how), &to->goal);

	// Keep a reachable height
	to->goal.z = from->area->GetZ(to->goal);

	Vector fromPos = from->goal;
	fromPos.z = from->area->GetZ(fromPos);
	Vector toPos = to->goal;
	toPos.z = to->area->GetZ(toPos);
	Vector groundNormal;
	from->area->ComputeNormal(&groundNormal);
	Vector alongPath = toPos - fromPos;
	const float expectedHeightDrop = -DotProduct(alongPath, groundNormal);

	if (expectedHeightDrop > mover->GetStepHeight())
	{
		Vector2D dir;
		DirectionToVector2D(static_cast<NavDirType>(to->how), &dir);

		constexpr float inc = 10.0f;
		const float maxPushDist = 2.0f * mover->GetHullWidth();
		const float halfWidth = mover->GetHullWidth() / 2.0f;
		const float hullHeight = mover->GetCrouchedHullHeigh();

		float pushDist;
		for (pushDist = 0.0f; pushDist <= maxPushDist; pushDist += inc)
		{
			Vector pos = to->goal + Vector(pushDist * dir.x, pushDist * dir.y, 0.0f);
			Vector lowerPos = Vector(pos.x, pos.y, toPos.z);

			trace_t result;
			const IHandleEntity* botent = bot->GetEdict()->GetIServerEntity();
			CTraceFilterNoNPCsOrPlayer filter(botent, COLLISION_GROUP_NONE);
			Vector v1(-halfWidth, -halfWidth, mover->GetStepHeight());
			Vector v2(halfWidth, halfWidth, hullHeight);

			UTIL_TraceHull(pos, lowerPos, v1, v2, mover->GetMovementTraceMask(), filter, &result);

			if (result.fraction >= 1.0f)
			{
				// found clearance to drop
				break;
			}
		}

		Vector startDrop(to->goal.x + pushDist * dir.x, to->goal.y + pushDist * dir.y, to->goal.z);
		Vector endDrop(startDrop.x, startDrop.y, to->area->GetZ(to->goal));

		float ground;
		if (TheNavMesh->GetGroundHeight(endDrop, &ground))
		{
			if (startDrop.z > ground + mover->GetStepHeight())
			{
				to->goal = startDrop;
				to->type = AIPath::SegmentType::SEGMENT_DROP_FROM_LEDGE;

				CBasePathSegment* newSegment = CreateNewSegment();
				newSegment->CopySegment(to);

				newSegment->goal.x = endDrop.x;
				newSegment->goal.y = endDrop.y;
				newSegment->goal.z = ground;
				newSegment->type = AIPath::SegmentType::SEGMENT_GROUND;

				pathinsert.emplace(to, newSegment, true);
			}
		}
	}

	return true;
}

bool CPath::ProcessLaddersInPath(CBaseBot* bot, CBasePathSegment* from, CBasePathSegment* to, std::stack<PathInsertSegmentInfo>& pathinsert)
{
	switch (to->how)
	{
	case GO_LADDER_UP:
	{
		auto ladders = from->area->GetLadders(CNavLadder::LADDER_UP);

		int i = 0;

		for (i = 0; i < ladders->Count(); i++)
		{
			CNavLadder* ladder = ladders->Element(i).ladder;

			if (ladder->m_topForwardArea == to->area || ladder->m_topLeftArea == to->area || ladder->m_topRightArea == to->area)
			{
				to->ladder = ladder;
				to->goal = ladder->m_bottom + ladder->GetNormal() * bot->GetMovementInterface()->GetHullWidth();
				to->type = AIPath::SegmentType::SEGMENT_LADDER_UP;
				break;
			}

			if (i == ladders->Count())
			{
				return false; // failed to find a valid ladder
			}
		}
		break;
	}
	case GO_LADDER_DOWN:
	{
		auto ladders = from->area->GetLadders(CNavLadder::LADDER_DOWN);

		int i = 0;

		for (i = 0; i < ladders->Count(); i++)
		{
			CNavLadder* ladder = ladders->Element(i).ladder;

			if (ladder->m_bottomArea == to->area)
			{
				to->ladder = ladder;
				to->goal = ladder->m_bottom + ladder->GetNormal() * bot->GetMovementInterface()->GetHullWidth();
				to->type = AIPath::SegmentType::SEGMENT_LADDER_UP;
				break;
			}

			if (i == ladders->Count())
			{
				return false; // failed to find a valid ladder
			}
		}
		break;
	}
	default:
		break;
	}

	return true;
}

bool CPath::ProcessPathJumps(CBaseBot* bot, CBasePathSegment* from, CBasePathSegment* to, std::stack<PathInsertSegmentInfo>& pathinsert)
{
	auto mover = bot->GetMovementInterface();
	// get closest point from areas to test for a gap
	Vector closeto, closefrom;
	to->area->GetClosestPointOnArea(from->goal, &closeto);
	from->area->GetClosestPointOnArea(closeto, &closefrom);

	// hull width used for calculations is the bot hull multiplied by this value
	constexpr float HULL_WIDTH_SAFETY_MARGIN = 1.1f;

	const float zdiff = fabsf(closeto.z - closefrom.z);
	const float gaplength = (closeto - closefrom).AsVector2D().Length();
	const float fullstepsize = mover->GetStepHeight();
	const float halfstepsize = fullstepsize * 0.5f;
	const float hullwidth = mover->GetHullWidth() * HULL_WIDTH_SAFETY_MARGIN;
	const float halfwidth = hullwidth * 0.5f;

	// not too high and gap is greater than the bot hull (with some tolerance)
	if (gaplength > hullwidth && zdiff < halfstepsize)
	{
		Vector landing;
		to->area->GetClosestPointOnArea(to->goal, &landing);
		Vector jumpfrom;
		from->area->GetClosestPointOnArea(landing, &jumpfrom);
		Vector jumpforward = landing - jumpfrom;
		jumpforward.NormalizeInPlace();

		// Adjust goal for optimal jump landing
		to->goal = landing + jumpforward * halfwidth;

		CBasePathSegment* newSegment = CreateNewSegment();

		newSegment->goal = jumpfrom - jumpforward * halfwidth;
		newSegment->type = AIPath::SegmentType::SEGMENT_JUMP_OVER_GAP;

		pathinsert.emplace(from, newSegment, true);
	}
	else if (zdiff > fullstepsize) // too high to just walk, must climb/jump
	{
		// when climbing, start from the area center
		to->goal = to->area->GetCenter();

		Vector jumppos;
		from->area->GetClosestPointOnArea(to->goal, &jumppos);

		CBasePathSegment* newSegment = CreateNewSegment();

		newSegment->goal = jumppos;
		newSegment->type = AIPath::SegmentType::SEGMENT_CLIMB_UP;

		pathinsert.emplace(from, newSegment, true);
	}

	return true;
}

void CPath::ComputeAreaCrossing(CBaseBot* bot, CNavArea* from, const Vector& frompos, CNavArea* to, NavDirType dir, Vector* crosspoint)
{
	from->ComputeClosestPointInPortal(to, dir, frompos, crosspoint);
}

void CPath::PostProcessPath()
{
	if (m_segments.size() == 0)
		return;

	if (m_segments.size() == 1)
	{
		auto seg = m_segments[0];

		seg->forward = vec3_origin;
		seg->length = 0.0f;
		seg->distance = 0.0f;
		seg->curvature = 0.0f;
		return;
	}

	float currentDistance = 0.0f;
	for (size_t i = 0; i < m_segments.size() - 1; i++)
	{
		auto from = m_segments[i];
		auto to = m_segments[i + 1];

		from->forward = to->goal - from->goal;
		from->length = from->forward.NormalizeInPlace();
		from->distance = currentDistance;
		currentDistance += from->length;
	}

	// calculate path curvature
	Vector2D fromVec, toVec;

	for (size_t i = 1; i < m_segments.size() - 1; i++)
	{
		auto to = m_segments[i];
		auto from = m_segments[i - 1];

		if (to->type != AIPath::SegmentType::SEGMENT_GROUND)
		{
			to->curvature = 0.0f;
			continue;
		}

		fromVec = from->forward.AsVector2D();
		fromVec.NormalizeInPlace();
		toVec = to->forward.AsVector2D();
		toVec.NormalizeInPlace();

		to->curvature = 0.5f * (1.0f * fromVec.Dot(toVec));

		Vector2D right(-fromVec.y, fromVec.x);

		if (toVec.Dot(right) < 0.0f)
		{
			to->curvature = -to->curvature; // negate curvature
		}
	}

	m_segments[0]->curvature = 0.0f; // path start doesn't have curvature

	// path goal segment (end) keeps the direction of the last segment
	size_t last = m_segments.size() - 1;
	auto seglast = m_segments[last];
	seglast->forward = m_segments[last - 1]->forward;
	seglast->length = 0.0f;
	seglast->distance = currentDistance;
	seglast->curvature = 0.0f;

	m_ageTimer.Start();
}

const Vector& CPath::GetStartPosition() const
{
	if (IsValid())
	{
		return m_segments[0]->goal;
	}
	
	return vec3_origin;
}

const Vector& CPath::GetEndPosition() const
{
	if (IsValid())
	{
		return m_segments[m_segments.size() - 1]->goal;
	}

	return vec3_origin;
}

const CBasePathSegment* CPath::GetFirstSegment() const
{
	if (m_segments.size() == 0)
	{
		return nullptr;
	}

	return m_segments[0];
}

const CBasePathSegment* CPath::GetLastSegment() const
{
	if (m_segments.size() == 0)
	{
		return nullptr;
	}

	return m_segments[m_segments.size() - 1];
}

const CBasePathSegment* CPath::GetNextSegment(const CBasePathSegment* current) const
{
	if (m_segments.size() == 0)
	{
		return nullptr;
	}

	for (size_t i = 0; i < m_segments.size(); i++)
	{
		CBasePathSegment* seg = m_segments[i];

		if (seg == current)
		{
			int next = i + 1;

			if (next >= m_segments.size())
			{
				return nullptr;
			}

			return m_segments[next];
		}
	}

	return nullptr;
}

const CBasePathSegment* CPath::GetPriorSegment(const CBasePathSegment* current) const
{
	if (m_segments.size() == 0)
	{
		return nullptr;
	}

	for (size_t i = 0; i < m_segments.size(); i++)
	{
		CBasePathSegment* seg = m_segments[i];

		if (seg == current)
		{
			int next = i + -1;

			if (next < 0)
			{
				return nullptr;
			}

			return m_segments[next];
		}
	}

	return nullptr;
}

// The segment the bot will try to reach.
const CBasePathSegment* CPath::GetGoalSegment() const
{
	return nullptr;
}
