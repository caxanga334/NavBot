#include <extension.h>
#include <bot/interfaces/playercontrol.h>
#include <bot/interfaces/movement.h>
#include <manager.h>
#include <mods/basemod.h>
#include <util/UtilTrace.h>

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

	if (ProcessGroundPath(bot, start, insertstack) == false)
	{
		return false;
	}

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

bool CPath::ProcessGroundPath(CBaseBot* bot, const Vector& start, std::stack<PathInsertSegmentInfo>& pathinsert)
{
	IMovement* mover = bot->GetMovementInterface();

	for (size_t i = 1; i < m_segments.size(); i++)
	{
		auto to = m_segments[i];
		auto from = m_segments[i - 1];

		if (to->how > GO_WEST) // only ground movement for now
		{
			continue;
		}

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
	}

	return true;
}

void CPath::ComputeAreaCrossing(CBaseBot* bot, CNavArea* from, const Vector& frompos, CNavArea* to, NavDirType dir, Vector* crosspoint)
{
	from->ComputeClosestPointInPortal(to, dir, frompos, crosspoint);
}
