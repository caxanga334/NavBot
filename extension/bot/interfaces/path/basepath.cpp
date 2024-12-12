#include <limits>
#include <cmath>

#include <extension.h>
#include <manager.h>
#include <bot/interfaces/playercontrol.h>
#include <bot/interfaces/movement.h>
#include <mods/basemod.h>
#include <sdkports/debugoverlay_shared.h>

#include "basepath.h"

#undef min
#undef max
#undef clamp

static ConVar sm_navbot_path_segment_draw_limit("sm_navbot_path_segment_draw_limit", "25", FCVAR_GAMEDLL | FCVAR_DONTRECORD, "Path segment draw limit.");
static ConVar sm_navbot_path_max_segments("sm_navbot_path_max_segments", "128", FCVAR_GAMEDLL | FCVAR_DONTRECORD, "Maximum number of path segments. Affects performance.");

CPath::CPath() :
	m_ageTimer()
{
	m_segments.reserve(512);
	m_cursorPos = 0.0f;
}

CPath::~CPath()
{
	m_segments.clear();
}

bool CPath::BuildTrivialPath(const Vector& start, const Vector& goal)
{
	constexpr float NAV_MAX_DIST = 1024.0f;

	CNavArea* startArea = TheNavMesh->GetNearestNavArea(start, NAV_MAX_DIST, false, true);
	CNavArea* goalArea = TheNavMesh->GetNearestNavArea(goal, NAV_MAX_DIST, false, true);

	if (startArea == nullptr || goalArea == nullptr)
		return false;

	Invalidate(); // destroy any path that exists

	std::shared_ptr<CBasePathSegment> startSeg = CreateNewSegment();
	std::shared_ptr<CBasePathSegment> endSeg = CreateNewSegment();

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

	m_segments.push_back(std::move(startSeg));
	m_segments.push_back(std::move(endSeg));
	m_ageTimer.Start();

	return true;
}

void CPath::Draw(const CBasePathSegment* start, const float duration)
{
	bool isstart = true;
	Vector v1, v2;
	int i = 0;
	const int limit = sm_navbot_path_segment_draw_limit.GetInt();

	constexpr float ARROW_WIDTH = 4.0f;

	const CBasePathSegment* next = start;

	while (next != nullptr)
	{
		if (isstart == true)
		{
			v1 = next->goal;
			isstart = false;
			next = GetNextSegment(next);
			continue;
		}

		v2 = next->goal;

		if (next->ladder)
		{
			Drawladder(next->ladder, next->type, duration);
		}

		DrawSingleSegment(v1, v2, next->type, duration);

		v1 = next->goal;

		next = GetNextSegment(next);

		if (++i > limit)
		{
			return;
		}
	}
}

void CPath::DrawFullPath(const float duration)
{
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

		if (segment->ladder)
		{
			Drawladder(segment->ladder, segment->type, duration);
		}

		DrawSingleSegment(v1, v2, segment->type, duration);

		v1 = segment->goal; // set v1 to the current segment position
	}
}

// Gets the total length of the current path.
float CPath::GetPathLength() const
{
	if (m_segments.size() == 0)
	{
		return 0.0f;
	}

	return m_segments[m_segments.size() - 1]->distance;
}

void CPath::MoveCursorToClosestPosition(const Vector& pos, SeekType type, float alongLimit)
{
	if (!IsValid())
	{
		return;
	}

	if (type == SEEK_ENTIRE_PATH || type == SEEK_AHEAD)
	{
		const CBasePathSegment* segment;

		if (type == SEEK_AHEAD)
		{
			// continue search from existing data
			if (m_cursor.segment != nullptr)
			{
				segment = m_cursor.segment;
			}
			else
			{
				// get start instead
				segment = GetFirstSegment();
			}
		}
		else
		{
			// search entire path from the start
			segment = GetFirstSegment();
		}

		m_cursor.position = pos;
		m_cursor.segment = segment;
		float closeRangeSq = std::numeric_limits<float>::max();

		float distanceSoFar = 0.0f;
		while (alongLimit == 0.0f || distanceSoFar <= alongLimit)
		{
			auto next = GetNextSegment(segment);

			if (next != nullptr)
			{
				Vector close;
				CalcClosestPointOnLineSegment(pos, segment->goal, next->goal, close);

				float rangeSq = (close - pos).LengthSqr();
				if (rangeSq < closeRangeSq)
				{
					m_cursor.position = close;
					m_cursor.segment = segment;

					closeRangeSq = rangeSq;
				}
			}
			else
			{
				break;
			}

			distanceSoFar += segment->length;
			segment = next;
		}

		segment = m_cursor.segment;

		float t = (m_cursor.position - segment->goal).Length() / segment->length;

		m_cursorPos = segment->distance + t * segment->length;
		m_cursor.outdated = true;
	}
}

const CPath::PathCursor& CPath::GetCursorData()
{
	if (IsValid())
	{
		if (m_cursor.outdated == true)
		{
			constexpr auto error = 0.0001f;

			if (m_cursorPos < error || m_segments.size() < 2)
			{
				// path start
				auto& seg = m_segments[0];
				m_cursor.position = seg->goal;
				m_cursor.forward = seg->forward;
				m_cursor.curvature = seg->curvature;
				m_cursor.segment = seg.get();
			}
			else if (m_cursorPos > GetPathLength() + error)
			{
				// path end
				auto& seg = m_segments[m_segments.size() - 1];
				m_cursor.position = seg->goal;
				m_cursor.forward = seg->forward;
				m_cursor.curvature = seg->curvature;
				m_cursor.segment = seg.get();
			}
			else
			{
				// find segment along the path
				float lengthSoFar = 0.0f;
				auto current = GetFirstSegment();
				auto next = GetNextSegment(current);

				while (next != nullptr)
				{
					float length = current->length;

					if (lengthSoFar + length > m_cursorPos)
					{
						float overlap = m_cursorPos - lengthSoFar;
						float t = 0.0f;

						if (length > 0.0f)
						{
							t = overlap / length;
						}

						// apply interpolation
						m_cursor.position = current->goal + t * (next->goal - current->goal);
						m_cursor.forward = current->forward + t * (next->forward - current->forward);
						m_cursor.segment = current;

						constexpr float INFLUENCE_RADIUS = 100.0f;

						if (overlap < INFLUENCE_RADIUS)
						{
							if (length - overlap < INFLUENCE_RADIUS)
							{
								// near both start and end, needs interpolation
								float startCurvature = current->curvature * (1.0f - (overlap / INFLUENCE_RADIUS));
								float endCurvature = next->curvature * (1.0f - ((length - overlap) / INFLUENCE_RADIUS));

								m_cursor.curvature = (startCurvature + endCurvature) / 2.0f;
							}
							else
							{
								// near start
								m_cursor.curvature = current->curvature * (1.0f - (overlap / INFLUENCE_RADIUS));
							}
						}
						else if (length - overlap < INFLUENCE_RADIUS)
						{
							// near end
							m_cursor.curvature = next->curvature * (1.0f - ((length - overlap) / INFLUENCE_RADIUS));
						}

						break;
					}

					lengthSoFar += length;

					current = next;
					next = GetNextSegment(next);
				}
			}

			// cursor updated
			m_cursor.outdated = false;
		}
	}
	else // invalid path
	{
		m_cursor.Invalidate();
	}

	return m_cursor;
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

	// We don't need to process the entire path since path are supposed to be recalculated from time to time
	// this only affects long paths

	size_t maxSegments = static_cast<size_t>(sm_navbot_path_max_segments.GetInt());

	if (maxSegments >= 32U && m_segments.size() > maxSegments)
	{
		m_segments.erase(std::next(m_segments.begin(), maxSegments - 1), m_segments.end());
	}

	auto& startSeg = m_segments[0];

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
		auto& to = m_segments[i];
		auto& from = m_segments[i - 1];

		switch (to->how)
		{
		case GO_NORTH:
			[[fallthrough]];
		case GO_EAST:
			[[fallthrough]];
		case GO_SOUTH:
			[[fallthrough]];
		case GO_WEST:
			if (!ProcessGroundPath(bot, start, from, to, insertstack))
			{
				failed = true;
			}
			break;
		case GO_LADDER_UP:
			[[fallthrough]];
		case GO_LADDER_DOWN:
			if (!ProcessLaddersInPath(bot, from, to, insertstack))
			{
				failed = true;
			}
			break;

		case GO_SPECIAL_LINK:
		{
			if (!ProcessSpecialLinksInPath(bot, from, to, insertstack))
			{
				failed = true;
			}
			break;
		}
		default: // other types are not supported right now
			break;
		}

		if (failed)
			break; // exit loop
	}

	if (!failed)
	{
		// Second iteration, handle jump and climbing
		for (size_t i = 1; i < m_segments.size(); i++)
		{
			auto& to = m_segments[i];
			auto& from = m_segments[i - 1];

			if (from->how != NUM_TRAVERSE_TYPES && from->how > GO_WEST)
				continue;

			if (to->how > GO_WEST || to->type != AIPath::SegmentType::SEGMENT_GROUND)
				continue;

			if (to->how == GO_SPECIAL_LINK)
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
			// auto& insert = insertstack.top();
			// delete insert.newSeg;
			insertstack.pop();
			return false;
		}
	}

	// process insert stack and add any new needed segments into the path
	while (insertstack.empty() == false)
	{
		auto& insert = insertstack.top(); // get top element
		auto& seg = insert.Seg;
		auto it = std::find(m_segments.begin(), m_segments.end(), seg);

		if (it != m_segments.end() || (it == m_segments.end() && !insert.after)) // got iterator of where we need to insert
		{
			if (insert.after)
			{
				it++; // by default, it will be added before the element of the iterator
			}

			m_segments.insert(it, std::move(insert.newSeg));
		}
		else
		{
			// delete insert.newSeg;
		}

		insertstack.pop(); // remove top element
	}


	return true;
}

bool CPath::ProcessGroundPath(CBaseBot* bot, const Vector& start, std::shared_ptr<CBasePathSegment>& from, std::shared_ptr<CBasePathSegment>& to, std::stack<PathInsertSegmentInfo>& pathinsert)
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
			trace::CTraceFilterNoNPCsOrPlayers filter(bot->GetEntity(), COLLISION_GROUP_NONE);
			Vector v1(-halfWidth, -halfWidth, mover->GetStepHeight());
			Vector v2(halfWidth, halfWidth, hullHeight);

			trace::hull(pos, lowerPos, v1, v2, mover->GetMovementTraceMask(), &filter, result);

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
				to->type = AIPath::SegmentType::SEGMENT_GROUND;

				std::shared_ptr<CBasePathSegment> newSegment = CreateNewSegment();
				newSegment->CopySegment(to.get());

				newSegment->goal.x = endDrop.x;
				newSegment->goal.y = endDrop.y;
				newSegment->goal.z = ground;
				newSegment->type = AIPath::SegmentType::SEGMENT_DROP_FROM_LEDGE;

				// Sometimes there is a railing between the drop and the ground


#if PROBLEMATIC
				trace::CTraceFilterNoNPCsOrPlayers filter(bot->GetEntity(), COLLISION_GROUP_NONE);
				trace_t result;
				Vector mins(-halfWidth, -halfWidth, mover->GetStepHeight());
				Vector maxs(halfWidth, halfWidth, hullHeight);

				trace::hull(from->goal, startDrop, mins, maxs, mover->GetMovementTraceMask(), &filter, result);

				if (result.DidHit()) // probably collided with a railing
				{
					to->goal = result.endpos;
					std::shared_ptr<CBasePathSegment> seg2 = CreateNewSegment();
					seg2->CopySegment(to.get());
					seg2->goal = startDrop + Vector(0.0f, 0.0f, mover->GetStepHeight());
					seg2->type = AIPath::SegmentType::SEGMENT_CLIMB_UP;

					std::shared_ptr<CBasePathSegment> jumpRunSegment = CreateNewSegment();
					jumpRunSegment->CopySegment(from.get());
					jumpRunSegment->type = AIPath::SegmentType::SEGMENT_GROUND;

					Vector runDir = (jumpRunSegment->goal - result.endpos);
					runDir.z = 0.0f;
					runDir.NormalizeInPlace();
					jumpRunSegment->goal = result.endpos + (runDir * (mover->GetHullWidth() * 1.2f));

					pathinsert.emplace(seg2, jumpRunSegment, false);
					pathinsert.emplace(newSegment, seg2, false);
				}

#endif

				pathinsert.emplace(to, newSegment, true);
			}
		}
	}

	return true;
}

bool CPath::ProcessLaddersInPath(CBaseBot* bot, std::shared_ptr<CBasePathSegment>& from, std::shared_ptr<CBasePathSegment>& to, std::stack<PathInsertSegmentInfo>& pathinsert)
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
			auto& connections = ladder->GetConnections();

			for (auto& connect : connections)
			{
				if (connect.IsConnectedToLadderTop() && connect.GetConnectedArea() == to->area)
				{
					to->ladder = ladder;
					to->goal = connect.GetConnectionPoint() + ladder->GetNormal() * bot->GetMovementInterface()->GetHullWidth();
					to->type = AIPath::SegmentType::SEGMENT_LADDER_UP;
					break;
				}
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
			auto& connections = ladder->GetConnections();

			for (auto& connect : connections)
			{
				if (connect.IsConnectedToLadderBottom() && connect.GetConnectedArea() == to->area)
				{
					to->ladder = ladder;
					to->goal = connect.GetConnectionPoint() + ladder->GetNormal() * bot->GetMovementInterface()->GetHullWidth();
					to->type = AIPath::SegmentType::SEGMENT_LADDER_UP;
					break;
				}
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

bool CPath::ProcessPathJumps(CBaseBot* bot, std::shared_ptr<CBasePathSegment>& from, std::shared_ptr<CBasePathSegment>& to, std::stack<PathInsertSegmentInfo>& pathinsert)
{
	auto mover = bot->GetMovementInterface();
	// get closest point from areas to test for a gap
	Vector closeto, closefrom;
	to->area->GetClosestPointOnArea(from->goal, &closeto);
	from->area->GetClosestPointOnArea(closeto, &closefrom);

	// hull width used for calculations is the bot hull multiplied by this value
	constexpr float HULL_WIDTH_SAFETY_MARGIN = 1.1f;

	const float zdiff = closeto.z - closefrom.z;
	const float gaplength = (closeto - closefrom).AsVector2D().Length();
	const float fullstepsize = mover->GetStepHeight();
	const float halfstepsize = fullstepsize * 0.5f;
	const float hullwidth = mover->GetHullWidth() * HULL_WIDTH_SAFETY_MARGIN;
	const float halfwidth = hullwidth * 0.5f;

	// not too high and gap is greater than the bot hull (with some tolerance)
	if (gaplength > hullwidth && fabsf(zdiff) < halfstepsize)
	{
		Vector landing;
		to->area->GetClosestPointOnArea(to->goal, &landing);
		Vector jumpfrom;
		from->area->GetClosestPointOnArea(landing, &jumpfrom);
		Vector jumpforward = landing - jumpfrom;
		jumpforward.NormalizeInPlace();

		// Adjust goal for optimal jump landing
		to->goal = landing + jumpforward * halfwidth;

		std::shared_ptr<CBasePathSegment> newSegment = CreateNewSegment();

		newSegment->CopySegment(from);
		newSegment->goal = jumpfrom - jumpforward * halfwidth;
		newSegment->type = AIPath::SegmentType::SEGMENT_JUMP_OVER_GAP;

		pathinsert.emplace(from, std::move(newSegment), true);
	}
	else if (zdiff > fullstepsize) // too high to just walk, must climb/jump
	{
		AIPath::SegmentType type = AIPath::SegmentType::SEGMENT_CLIMB_UP; // default to simple jump

		if (zdiff > mover->GetMaxJumpHeight() && zdiff < mover->GetMaxDoubleJumpHeight())
		{
			type = AIPath::SEGMENT_CLIMB_DOUBLE_JUMP;
		}

		// when climbing, start from the area center
		to->goal = to->area->GetCenter();
		to->type = type; // mark this segment for jumping

		Vector jumppos;
		from->area->GetClosestPointOnArea(to->goal, &jumppos);
		
		// Create a new ground segment towards the jump position
		std::shared_ptr<CBasePathSegment> newSegment = CreateNewSegment();

		newSegment->CopySegment(from);
		newSegment->goal = jumppos;
		newSegment->type = AIPath::SegmentType::SEGMENT_GROUND;

		pathinsert.emplace(from, std::move(newSegment), true);
	}

	return true;
}

bool CPath::ProcessSpecialLinksInPath(CBaseBot* bot, std::shared_ptr<CBasePathSegment>& from, std::shared_ptr<CBasePathSegment>& to, std::stack<PathInsertSegmentInfo>& pathinsert)
{
	auto link = from->area->GetSpecialLinkConnectionToArea(to->area);

	if (!link)
	{
		Warning("to->how == GO_SPECIAL_LINK but from->area nas no special link to to->area! \n");
		return false;
	}

	switch (link->GetType())
	{
	case NavLinkType::LINK_GROUND:
		[[fallthrough]];
	case NavLinkType::LINK_TELEPORTER:
	{
		// link ends at the destination area's center.
		to->goal = to->area->GetCenter();

		auto between = CreateNewSegment();

		between->CopySegment(from);
		between->goal = link->GetStart();
		between->how = GO_SPECIAL_LINK;
		between->type = AIPath::SEGMENT_GROUND;
		// add a segments between from and to.
		// the bot will go to from, them move to between and then move to to.
		// between goal is the special link position on the nav area.

		auto post = CreateNewSegment();

		post->CopySegment(between);
		post->how = GO_SPECIAL_LINK;
		post->goal = link->GetEnd();
		post->type = AIPath::SEGMENT_GROUND;

		pathinsert.emplace(to, std::move(post), false);
		pathinsert.emplace(to, std::move(between), false); // insert before 'to'
		break;
	}
	case NavLinkType::LINK_BLAST_JUMP:
	{
		// link ends at the destination area's center.
		to->goal = to->area->GetCenter();
		// 'to' starts at the link position and ends at it's area center
		// change it to a blast jump segment type
		to->type = AIPath::SEGMENT_BLAST_JUMP;

		auto between = CreateNewSegment();

		between->CopySegment(from);
		between->goal = link->GetStart();
		between->how = GO_SPECIAL_LINK;
		between->type = AIPath::SEGMENT_GROUND;
		// add a segments between from and to.
		// the bot will go to from, them move to between and then move to to.
		// between goal is the special link position on the nav area.

		pathinsert.emplace(to, std::move(between), false); // insert before 'to'
		break;
	}
	case NavLinkType::LINK_DOUBLE_JUMP:
	{
		// link ends at the destination area's center.
		to->goal = to->area->GetCenter();

		auto between = CreateNewSegment();

		between->CopySegment(from);
		between->goal = link->GetStart();
		between->how = GO_SPECIAL_LINK;
		between->type = AIPath::SEGMENT_GROUND;
		// add a segments between from and to.
		// the bot will go to from, them move to between and then move to to.
		// between goal is the special link position on the nav area.

		auto post = CreateNewSegment();

		post->CopySegment(between);
		post->how = GO_SPECIAL_LINK;
		post->goal = link->GetEnd();
		post->type = AIPath::SEGMENT_CLIMB_DOUBLE_JUMP;

		pathinsert.emplace(to, std::move(post), false);
		pathinsert.emplace(to, std::move(between), false); // insert before 'to'
		break;
	}
	default:
		Warning("CPath::ProcessSpecialLinksInPath unhandled link type!");
		return false;
	}

	return true;
}

void CPath::ComputeAreaCrossing(CBaseBot* bot, CNavArea* from, const Vector& frompos, CNavArea* to, NavDirType dir, Vector* crosspoint)
{
	from->ComputeClosestPointInPortal(to, dir, frompos, crosspoint);
	
	bot->GetMovementInterface()->AdjustPathCrossingPoint(from, to, frompos, crosspoint);
}

void CPath::PostProcessPath()
{
	if (m_segments.size() == 0)
		return;

	if (m_segments.size() == 1)
	{
		auto& seg = m_segments[0];

		seg->forward = vec3_origin;
		seg->length = 0.0f;
		seg->distance = 0.0f;
		seg->curvature = 0.0f;
		return;
	}

	float currentDistance = 0.0f;
	for (size_t i = 0; i < m_segments.size() - 1; i++)
	{
		auto& from = m_segments[i];
		auto& to = m_segments[i + 1];

		from->forward = to->goal - from->goal;
		from->length = from->forward.NormalizeInPlace();
		from->distance = currentDistance;
		currentDistance += from->length;
	}

	// calculate path curvature
	Vector2D fromVec, toVec;

	for (size_t i = 1; i < m_segments.size() - 1; i++)
	{
		auto& to = m_segments[i];
		auto& from = m_segments[i - 1];

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
	auto& seglast = m_segments[last];
	seglast->forward = m_segments[last - 1]->forward;
	seglast->length = 0.0f;
	seglast->distance = currentDistance;
	seglast->curvature = 0.0f;

	m_ageTimer.Start();
}

void CPath::DrawSingleSegment(const Vector& v1, const Vector& v2, AIPath::SegmentType type, const float duration)
{
	constexpr float ARROW_WIDTH = 4.0f;

	NDebugOverlay::Sphere(v1, 5.0f, 0, 210, 255, true, duration);
	char name[64];
	ke::SafeSprintf(name, sizeof(name), "%s", AIPath::SegmentTypeToString(type));
	NDebugOverlay::Text(v1, name, false, duration);

	switch (type)
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
	case AIPath::SegmentType::SEGMENT_CLIMB_DOUBLE_JUMP:
	{
		NDebugOverlay::VertArrow(v1, v2, ARROW_WIDTH, 204, 0, 255, 255, true, duration);
		break;
	}
	case AIPath::SegmentType::SEGMENT_BLAST_JUMP:
	{
		NDebugOverlay::VertArrow(v1, v2, ARROW_WIDTH, 255, 0, 0, 255, true, duration);
		break;
	}
	default:
	{
		NDebugOverlay::HorzArrow(v1, v2, ARROW_WIDTH, 0, 255, 0, 255, true, duration);
		break;
	}
	}
}

void CPath::Drawladder(const CNavLadder* ladder, AIPath::SegmentType type, const float duration)
{
	constexpr auto LADDER_WIDTH = 8.0f;

	if (type == AIPath::SegmentType::SEGMENT_LADDER_UP)
	{
		NDebugOverlay::VertArrow(ladder->m_bottom, ladder->m_top, LADDER_WIDTH, 0, 255, 0, 255, true, duration);
	}
	else
	{
		NDebugOverlay::VertArrow(ladder->m_top, ladder->m_bottom, LADDER_WIDTH, 0, 100, 0, 255, true, duration);
	}
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

// The segment the bot will try to reach.
const CBasePathSegment* CPath::GetGoalSegment() const
{
	return nullptr;
}
