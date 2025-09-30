#include NAVBOT_PCH_FILE
#include <algorithm>
#include <cmath>

#include <extension.h>
#include <sdkports/debugoverlay_shared.h>
#include <sdkports/sdk_traces.h>
#include <util/entprops.h>
#include <util/helpers.h>
#include <util/librandom.h>
#include <entities/baseentity.h>
#include <manager.h>
#include "meshnavigator.h"

#ifdef EXT_VPROF_ENABLED
#include <tier0/vprof.h>
#endif // EXT_VPROF_ENABLED

#undef min
#undef max
#undef clamp // ugly hack to be able to use std functions without conflicts with valve's mathlib

ConVar sm_navbot_path_debug_climbing("sm_navbot_path_debug_climbing", "0", FCVAR_CHEAT | FCVAR_DONTRECORD, "Debugs automatic object climbing");
ConVar sm_navbot_path_goal_tolerance("sm_navbot_path_goal_tolerance", "32", FCVAR_DONTRECORD, "Default navigator goal tolerance");
ConVar sm_navbot_path_skip_ahead_distance("sm_navbot_path_skip_ahead_distance", "350", FCVAR_DONTRECORD, "Default navigator skip ahead distance");
ConVar sm_navbot_path_useable_scan("sm_navbot_path_useable_scan", "0.5", FCVAR_DONTRECORD, "How frequently the navigation will scan for useable entities on the bot's path.");

#ifdef EXT_DEBUG
static ConVar sm_navbot_path_debug_goal("sm_navbot_path_debug_goal", "0", FCVAR_CHEAT | FCVAR_DONTRECORD, "Debugs navigator path goal.");
#endif // EXT_DEBUG


CMeshNavigator::CMeshNavigator() : CPath()
{
	float goaltolerance = sm_navbot_path_goal_tolerance.GetFloat();
	float skipahead = sm_navbot_path_skip_ahead_distance.GetFloat();

	goaltolerance = std::clamp(goaltolerance, 5.0f, 40.0f);
	skipahead = std::clamp(skipahead, -1.0f, 512.0f);

	m_goal = nullptr;
	m_goalTolerance = goaltolerance;
	m_skipAheadDistance = skipahead;
	m_waitTimer.Invalidate();
	m_avoidTimer.Invalidate();
	m_blocker.Term();
	m_didAvoidCheck = false;
	m_me = nullptr;
}

CMeshNavigator::~CMeshNavigator()
{
	if (m_me != nullptr)
	{
		m_me->NotifyNavigatorDestruction(this);
	}
}

void CMeshNavigator::Invalidate()
{
	m_goal = nullptr;
	m_waitTimer.Invalidate();
	m_avoidTimer.Invalidate();
	m_blocker.Term();
	CPath::Invalidate();
}

void CMeshNavigator::OnPathChanged(CBaseBot* bot, AIPath::ResultType result)
{
	switch (result)
	{
	case AIPath::COMPLETE_PATH:
	case AIPath::PARTIAL_PATH:
		m_goal = GetFirstSegment();
		/* Avoid backtracking by advancing the goal to the nearest segment, this occurs when the bot is performing air movements */
		SetBot(bot);
		AdvanceGoalToNearest();
		break;
	case AIPath::NO_PATH:
	default:
		m_goal = nullptr;
		break;
	}
}

void CMeshNavigator::Update(CBaseBot* bot)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("CMeshNavigator::Update", "NavBot");
#endif // EXT_VPROF_ENABLED

	if (!IsValid() || m_goal == nullptr)
	{
		return; // no path or goal
	}

	auto mover = bot->GetMovementInterface();
	m_me = bot;
	bot->SetActiveNavigator(this);

	if (!m_waitTimer.IsElapsed())
	{
		return; // wait for something to stop blocking our path (like a door opening)
	}

	if (LadderUpdate(bot) || ElevatorUpdate(bot))
	{
		return; // bot is using a ladder or elevator
	}

	if (!CheckProgress(bot))
	{
		return; // goal reached
	}

	if (mover->IsStoppedAndWaiting())
	{
		return;
	}

	if (mover->IsControllingMovements())
	{
		// bot movements are under control of the movement interface, likely performing an advanced movement that requires multiple steps such as a rocket jump.
		// Wait until it's done to continue moving the bot.
		return;
	}

	if (m_useableTimer.IsElapsed())
	{
		m_useableTimer.Start(sm_navbot_path_useable_scan.GetFloat());
		SearchForUseableObstacles(bot);
	}

	mover->DetermineIdealPostureForPath(this);
	BreakIfNeeded(bot);
	mover->AdjustSpeedForPath(this);

	if (OffMeshLinksUpdate(bot))
	{
		return; // handling special off-mesh connection
	}

	Vector origin = bot->GetAbsOrigin();
	Vector forward = m_goal->goal - origin;
	auto input = bot->GetControlInterface();

	if (m_goal->type == AIPath::SegmentType::SEGMENT_CLIMB_UP || m_goal->type == AIPath::SegmentType::SEGMENT_CLIMB_DOUBLE_JUMP || m_goal->type == AIPath::SegmentType::SEGMENT_BLAST_JUMP)
	{
		auto next = GetNextSegment(m_goal);
		if (next != nullptr)
		{
			forward = next->goal - origin;
		}
	}

	forward.z = 0.0f;
	const float goalRange = forward.NormalizeInPlace();
	Vector left(-forward.y, forward.x, 0.0f);

	// Causes issues when the bot is underwater
	if (left.IsZero() && !bot->IsUnderWater())
	{
		bot->OnMoveToFailure(this, IEventListener::FAIL_STUCK);

		// invalid path
		if (GetAge() > 0.0f)
		{
			Invalidate();
		}

		return;
	}

	Vector normal(0.0f, 0.0f, 1.0f);

	forward = CrossProduct(left, normal);
	left = CrossProduct(normal, forward);

	if (!Climbing(bot, m_goal, forward, left, goalRange))
	{
		if (!IsValid())
		{
			return; // path might become invalid from a failed climb
		}

		JumpOverGaps(bot, m_goal, forward, left, goalRange);
	}

	// It's possible that the path become invalid after a jump or climb attempt
	if (!IsValid())
	{
		return; 
	}

	CNavArea* myarea = bot->GetLastKnownNavArea();
	const bool isOnStairs = myarea->HasAttributes(NAV_MESH_STAIRS);
	const float tooHighDistance = mover->GetMaxJumpHeight();

	if (m_goal->ladder == nullptr && !mover->IsClimbingOrJumping() && !isOnStairs && m_goal->goal.z > origin.z + tooHighDistance)
	{
		constexpr auto CLOSE_RANGE = 25.0f;
		Vector2D to(origin.x - m_goal->goal.x, origin.y - m_goal->goal.y);

		if (mover->IsStuck() || to.IsLengthLessThan(CLOSE_RANGE))
		{
			auto next = GetNextSegment(m_goal);
			if (mover->IsStuck() || !next || (next->goal.z - origin.z > mover->GetMaxJumpHeight()) || !mover->IsPotentiallyTraversable(origin, next->goal, nullptr))
			{
				bot->OnMoveToFailure(this, IEventListener::FAIL_FELL_OFF_PATH);

				if (GetAge() > 0.0f)
				{
					Invalidate();
				}

				return;
			}
		}
	}

	Vector goalPos = m_goal->goal;

	// avoid small obstacles
	forward = goalPos - origin;
	forward.z = 0.0f;
	float rangeToGoal = forward.NormalizeInPlace();

	left.x = -forward.y;
	left.y = forward.x;
	left.z = 0.0f;

	constexpr auto nearLedgeRange = 50.0f;
	if (rangeToGoal > nearLedgeRange || (m_goal && m_goal->type != AIPath::SegmentType::SEGMENT_CLIMB_UP))
	{
		auto next = GetNextSegment(m_goal);
		bool shouldavoid = true;

		constexpr auto nearLadderRange = 55.0f; // avoid range is 50.0f * model scale. Most of the time the model scale will be 1.0f

		if (next && 
			(next->type == AIPath::SegmentType::SEGMENT_LADDER_UP || next->type == AIPath::SegmentType::SEGMENT_LADDER_DOWN) &&
			rangeToGoal < nearLadderRange)
		{
			// Ladders are generally placed against a wall
			// Don't try to walk around it
			shouldavoid = false;
		}

		if (shouldavoid)
		{
			goalPos = Avoid(bot, goalPos, forward, left);
		}
	}

	if (m_goal->area->IsUnderwater() || bot->GetWaterLevel() >= static_cast<int>(entityprops::WaterLevel::WL_Waist))
	{
		goalPos = AdjustGoalForUnderWater(bot, goalPos, m_goal);
		input->AimAt(goalPos, IPlayerController::LOOK_MOVEMENT, 0.1f, "Looking at move goal (Underwater).");
	}
	else if (mover->IsOnGround())
	{
		auto eyes = bot->GetEyeOrigin();
		Vector lookat(goalPos.x, goalPos.y, eyes.z);

		// low priority look towards movement goal so the bot doesn't walk with a fixed look
		input->AimAt(lookat, IPlayerController::LOOK_PATH, 0.1f, "Looking at move goal (Ground).");
	}

	// move bot along path
	mover->MoveTowards(goalPos, IMovement::MOVEWEIGHT_NAVIGATOR);

#ifdef EXT_DEBUG
	if (sm_navbot_path_debug_goal.GetInt() > 1 && m_goal)
	{
		META_CONPRINTF("%s PATH GOAL SEG %s <%g %g %g> %g RANGE TO GOAL: %g\n",
			bot->GetDebugIdentifier(), AIPath::SegmentTypeToString(m_goal->type), m_goal->goal.x, m_goal->goal.y, m_goal->goal.z, m_goal->curvature, rangeToGoal);
	}
#endif // EXT_DEBUG

	if (bot->IsDebugging(BOTDEBUG_PATH))
	{
		auto start = GetGoalSegment();

		if (start != nullptr)
		{
			start = GetPriorSegment(start);
		}

		if (start != nullptr)
		{
			Draw(start, 0.1f);
		}

		NDebugOverlay::Sphere(goalPos, 5.0f, 255, 255, 0, true, 0.1f);
	}
}

bool CMeshNavigator::IsAtGoal(CBaseBot* bot)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("CMeshNavigator::IsAtGoal", "NavBot");
#endif // EXT_VPROF_ENABLED

	IMovement* mover = bot->GetMovementInterface();
	auto current = GetPriorSegment(m_goal);
	Vector origin = bot->GetAbsOrigin();
	Vector toGoal = m_goal->goal - origin;

	// m_goal is the segment the bot is moving towards, if there is no prior segment, the bot is at the goal
	if (current == nullptr)
	{
		return true;
	}

#ifdef EXT_DEBUG
	const float DebugdistToGoal = (m_goal->goal - origin).Length();

	if (sm_navbot_path_debug_goal.GetInt() > 2 && m_goal)
	{
		META_CONPRINTF("%s IS AT GOAL %s <%g %g %g> %g RANGE TO GOAL: %g\n",
			bot->GetDebugIdentifier(), AIPath::SegmentTypeToString(m_goal->type), m_goal->goal.x, m_goal->goal.y, m_goal->goal.z, m_goal->curvature, DebugdistToGoal);
	}
#endif // EXT_DEBUG

	// For off-mesh connections, ask the movement interface first
	if (m_goal->how == NavTraverseType::GO_OFF_MESH_CONNECTION)
	{
		bool ret = false;
		if (mover->IsPathSegmentReached(this, m_goal, ret))
		{
			return ret;
		}
	}

	if (m_goal->type == AIPath::SegmentType::SEGMENT_DROP_FROM_LEDGE)
	{
		auto landing = GetNextSegment(m_goal);

		if (landing == nullptr)
		{
			return true; // goal reached or path is corrupt
		}
		if (origin.z - landing->goal.z < mover->GetStepHeight())
		{
			return true;
		}
	}
	else if (m_goal->type == AIPath::SegmentType::SEGMENT_CLIMB_DOUBLE_JUMP ||
		m_goal->type == AIPath::SegmentType::SEGMENT_BLAST_JUMP ||
		m_goal->type == AIPath::SegmentType::SEGMENT_CLIMB_UP)
	{
		auto landing = GetNextSegment(m_goal);

		if (landing == nullptr)
		{
			return true; // goal reached or path is corrupt
		}
		if (origin.z > m_goal->goal.z + mover->GetStepHeight())
		{
			// bot can step to goal, reached
			return true;
		}
	}
	else if (m_goal->type == AIPath::SegmentType::SEGMENT_CATAPULT)
	{
		if (origin.z > m_goal->goal.z + mover->GetStepHeight())
		{
			// bot is above the catapult start
			return true;
		}
	}
	else if (m_goal->type == AIPath::SegmentType::SEGMENT_LADDER_UP || m_goal->type == AIPath::SegmentType::SEGMENT_LADDER_DOWN)
	{
		// experiental
		return bot->GetMovementInterface()->IsOnLadder();
	}
	else
	{
		auto next = GetNextSegment(m_goal);

		if (next)
		{
			Vector2D plane;

			if (current->ladder != nullptr)
			{
				plane = m_goal->forward.AsVector2D();
			}
			else
			{
				plane = current->forward.AsVector2D() + m_goal->forward.AsVector2D();
			}

			float dot = DotProduct2D(toGoal.AsVector2D(), plane);

			if (dot < 0.0001f && fabsf(toGoal.z) < mover->GetStandingHullHeight())
			{
				// If it's reachable, then the bot reached it
				if (toGoal.z < mover->GetStepHeight() &&
					mover->IsPotentiallyTraversable(origin, next->goal, nullptr, false) == true &&
					mover->HasPotentialGap(origin, next->goal, nullptr) == false)
				{
					return true;
				}
			}
		}
	}

	if (m_goal->type == AIPath::SegmentType::SEGMENT_STRAFE_JUMP && mover->IsStrafeJumping())
	{
		return true;
	}

	if (bot->IsUnderWater())
	{
		if (IsAtUnderwaterGoal(bot))
		{
			return true;
		}
	}

	// Check distance to goal
	if (toGoal.AsVector2D().IsLengthLessThan(GetGoalTolerance()))
	{
		return true;
	}

	return false;
}

bool CMeshNavigator::IsAtUnderwaterGoal(CBaseBot* bot)
{
	constexpr float UNDERWATER_MULT = 1.4f;

	const float distance2d = (bot->GetAbsOrigin() - m_goal->goal).AsVector2D().Length();

	if (m_goal->type == AIPath::SegmentType::SEGMENT_GROUND && distance2d <= GetGoalTolerance() * UNDERWATER_MULT)
	{
		return true;
	}

	return false;
}

/**
 * @brief Checks if the bot has reached the current path goal. If yes, iterate to the next segment
 * @return true if the bot still needs to move, false if the path goal was reached
*/
bool CMeshNavigator::CheckProgress(CBaseBot* bot)
{
	if (IsAtGoal(bot))
	{
		auto next = CheckSkipPath(bot, m_goal);
		auto mover = bot->GetMovementInterface();

		if (next == nullptr) // no segment to skip, move to next on the path
		{
			next = GetNextSegment(m_goal);
		}
#ifdef EXT_DEBUG
		else
		{
			if (sm_navbot_path_debug_goal.GetBool() && m_goal)
			{
				META_CONPRINTF("%s GOAL SEG REACHED %s NEXT PATH GOAL IS %s\n", 
					bot->GetDebugIdentifier(), AIPath::SegmentTypeToString(m_goal->type), AIPath::SegmentTypeToString(next->type));
			}
		}
#endif // EXT_DEBUG

		if (next == nullptr) // no next segment, only one segment remains on the path
		{
			if (mover->IsOnGround()) // must be on the ground to end the path
			{
				bot->OnMoveToSuccess(this); // Notify the path goal was reached

				// The bot might calculate a new path as a result of the move to success event
				// Only invalidate the current path if no new path was made
				if (GetAge() > 0.0f)
				{
					Invalidate();
				}

				return false;
			}
		}
		else // There are still some segments left to follow, move to the next
		{
			OnGoalSegmentReached(m_goal, next);
			m_goal = next;
		}
	}

	return true;
}

const CBasePathSegment* CMeshNavigator::CheckSkipPath(CBaseBot* bot, const CBasePathSegment* from) const
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("CMeshNavigator::CheckSkipPath", "NavBot");
#endif // EXT_VPROF_ENABLED

	auto mover = bot->GetMovementInterface();

	const CBasePathSegment* skip = nullptr;

	if (m_skipAheadDistance > 0.0f)
	{
		skip = from;
		const Vector origin = bot->GetAbsOrigin();
		const float squared_range = m_skipAheadDistance * m_skipAheadDistance;

		while (skip != nullptr && skip->type == AIPath::SegmentType::SEGMENT_GROUND)
		{
			const float range = (skip->goal - origin).LengthSqr();

			if (range < squared_range)
			{
				auto next = GetNextSegment(skip);

				if (next == nullptr || next->type != AIPath::SegmentType::SEGMENT_GROUND)
				{
					break; // only skip ground segments
				}

				if (next->goal.z > origin.z + mover->GetStepHeight())
				{
					break; // going up a hill/stairs, don't skip
				}

				if (from->area->HasAttributes(NAV_MESH_PRECISE) || next->area->HasAttributes(NAV_MESH_PRECISE))
				{
					break; // don't skip precise areas
				}

				if (!mover->NavigatorAllowSkip(next->area))
				{
					break; // movement interface disallows skipping the next segment area
				}

				bool IsPotentiallyTraversable = mover->IsPotentiallyTraversable(origin, next->goal, nullptr, false);
				if (IsPotentiallyTraversable && mover->HasPotentialGap(origin, next->goal, nullptr) == false)
				{
					// only skip a segment if the bot is able to move directly to it from it's current position
					// and there isn't any holes on the ground

					if (bot->IsDebugging(BOTDEBUG_PATH))
					{
						NDebugOverlay::Text(next->goal + Vector(0.0f, 0.0f, 8.0f), "Segment Skipped!", false, 0.5f);
						NDebugOverlay::Sphere(next->goal, 6.0f, 0, 230, 255, true, 0.5f);
					}

					skip = next;
				}
				else
				{

					break; // unreachable, just keep following the path
				}
			}
			else
			{
				break; // max skip distance reached
			}
		}

		if (skip == from) // no segment to skip, return NULL
		{
			return nullptr;
		}
	}

	return skip;
}

Vector CMeshNavigator::AdjustGoalForUnderWater(CBaseBot* bot, const Vector& goalPos, const CBasePathSegment* seg)
{
	Vector origin = bot->GetAbsOrigin();

	// If climbing, don't touch Z
	if (goalPos.z >= origin.z)
	{
		return goalPos;
	}

	Vector adjusted = goalPos;
	adjusted.z = origin.z;
	adjusted.z += navgenparams->human_crouch_height;

	// Try to maintain the current height unless there's something blocking it
	if (bot->GetMovementInterface()->IsPotentiallyTraversable(origin, adjusted, nullptr, true, nullptr))
	{
		return adjusted;
	}

	return goalPos;
}

bool CMeshNavigator::Climbing(CBaseBot* bot, const CBasePathSegment* segment, const Vector& forward, const Vector& right, const float goalRange)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("CMeshNavigator::Climbing", "NavBot");
#endif // EXT_VPROF_ENABLED

	auto mover = bot->GetMovementInterface();
	auto input = bot->GetControlInterface();
	auto myarea = bot->GetLastKnownNavArea();
	Vector origin = bot->GetAbsOrigin();
	const bool debug = sm_navbot_path_debug_climbing.GetBool();

	if (!mover->IsAbleToClimb())
	{
		return false;
	}

	if (mover->IsOnLadder())
	{
		return false; // Don't bother with climbing when using ladders
	}

	if (myarea && myarea->HasAttributes(NAV_MESH_NO_JUMP))
	{
		return false; // Don't try to climb on NO_JUMP areas.
	}

	Vector climbDir = forward;
	climbDir.z = 0.0f;
	climbDir.NormalizeInPlace();

	const float ledgeLookAheadDist = mover->GetHullWidth() - 1.0f;

	if (mover->IsClimbingOrJumping() || mover->IsAscendingOrDescendingLadder() || !mover->IsOnGround())
	{
		return false;
	}

	if (m_goal == nullptr)
	{
		return false;
	}

	float rangeToGoal = (origin - m_goal->goal).Length();

	if (rangeToGoal - (mover->GetHullWidth() * 0.5f) > GetGoalTolerance())
	{
		return false;
	}

	if (TheNavMesh->IsAuthoritative())
	{
		// Authorative nav mesh can be trusted for climbing

		/* For normal jumps and double jumps, wait until the bot is aligned before jumping */
		if (m_goal->type == AIPath::SegmentType::SEGMENT_CLIMB_UP || m_goal->type == AIPath::SegmentType::SEGMENT_CLIMB_DOUBLE_JUMP)
		{
			// t = cos(deg2rad(X)) where X is a degree in angle
			constexpr float tolerance = 0.5f; // 60 degrees tolerance

			if (bot->GetAbsVelocity().Length() >= 100.0f && !bot->IsMovingTowards2D(m_goal->goal, tolerance))
			{
#ifdef EXT_DEBUG
				if (sm_navbot_path_debug_goal.GetInt() > 2)
				{
					META_CONPRINTF("%s CLIMB REJECTED! NOT ALIGNED! RANGE: %g\n",
						bot->GetDebugIdentifier(), rangeToGoal);
				}
#endif // EXT_DEBUG

				return false;
			}
		}

		if (m_goal->type == AIPath::SegmentType::SEGMENT_CLIMB_UP)
		{
			auto seg = GetNextSegment(m_goal);

			if (seg != nullptr)
			{
				Vector nearClimb;
				seg->area->GetClosestPointOnArea(origin, &nearClimb);
				climbDir = nearClimb - origin;
				climbDir.z = 0.0f;
				climbDir.NormalizeInPlace();

				if (mover->ClimbUpToLedge(nearClimb, climbDir, nullptr))
					return true;
			}
		}
		else if (m_goal->type == AIPath::SegmentType::SEGMENT_CLIMB_DOUBLE_JUMP)
		{
			auto seg = GetNextSegment(m_goal);

			if (seg != nullptr)
			{
				Vector nearClimb;
				seg->area->GetClosestPointOnArea(origin, &nearClimb);
				climbDir = nearClimb - origin;
				climbDir.z = 0.0f;
				climbDir.NormalizeInPlace();

				if (mover->DoubleJumpToLedge(nearClimb, climbDir, nullptr))
					return true;
			}
		}
		else if (m_goal->type == AIPath::SegmentType::SEGMENT_BLAST_JUMP)
		{
			auto seg = GetNextSegment(m_goal);

			if (seg != nullptr)
			{
				Vector nearClimb = seg->goal;
				climbDir = nearClimb - origin;
				climbDir.z = 0.0f;
				climbDir.NormalizeInPlace();
				mover->BlastJumpTo(m_goal->goal, seg->goal, climbDir);
				return true;
			}
		}

		return false;
	}

	float heightDelta = -1.0f;
	const float tolerance = mover->GetCrouchedHullHeight();

	if (m_goal->type == AIPath::SegmentType::SEGMENT_CLIMB_UP)
	{
		auto after = GetNextSegment(m_goal);

		if (after)
		{
			Vector nearClimb;
			after->area->GetClosestPointOnArea(origin, &nearClimb);

			climbDir = nearClimb - origin;
			heightDelta = climbDir.z;
			climbDir.z = 0.0f;
			climbDir.NormalizeInPlace();
		}
	}

	// don't try to climb on stairs
	if (m_goal->area->HasAttributes(NAV_MESH_STAIRS) || (myarea && myarea->HasAttributes(NAV_MESH_STAIRS)))
	{
		return false;
	}

	// get the segment before our current goal (the one we just passed)
	auto current = GetPriorSegment(m_goal);

	if (current == nullptr)
	{
		return false;
	}

	Vector toGoal = m_goal->goal - origin;
	toGoal.NormalizeInPlace();

	if (toGoal.z < mover->GetTraversableSlopeLimit() && m_goal->type != AIPath::SegmentType::SEGMENT_CLIMB_UP &&
		mover->IsPotentiallyTraversable(origin, origin * ledgeLookAheadDist * toGoal, nullptr, true))
	{
		return false;
	}

	constexpr auto CLIMB_LOOK_AHEAD_RANGE = 150.0f;
	bool isPlannedClimb = false; // true if the path itself wants us to climb
	float plannedClimbZ = 0.0f;

	for (auto segment = current; segment != nullptr; segment = GetNextSegment(segment))
	{
		if (segment != current && (segment->goal - origin).AsVector2D().IsLengthGreaterThan(CLIMB_LOOK_AHEAD_RANGE))
		{
			break;
		}

		if (segment->type == AIPath::SegmentType::SEGMENT_CLIMB_UP)
		{
			isPlannedClimb = true;
			auto next = GetNextSegment(current);
			
			if (next)
			{
				plannedClimbZ = next->goal.z;
			}
			break;
		}
	}

	auto mask = mover->GetMovementTraceMask();
	trace_t result;
	CMovementTraverseFilter filter(bot, mover, true);

	const float hullWidth = mover->GetHullWidth();
	const float halfWidth = hullWidth * 0.5f;
	const float minHullHeight = mover->GetCrouchedHullHeight();
	const float minLedgeHeight = mover->GetStepHeight() + 0.1f;

	Vector skipStepHeightHullMin(-halfWidth, -halfWidth, minLedgeHeight);
	Vector skipStepHeightHullMax(halfWidth, halfWidth, minHullHeight + 0.1f);
	float ceilingFraction = 0.0f;

	Vector feet = origin;
	Vector ceiling(feet + Vector(0.0f, 0.0f, mover->GetMaxJumpHeight()));

	trace::hull(feet, ceiling, skipStepHeightHullMin, skipStepHeightHullMax, mask, &filter, result);

	ceilingFraction = result.fraction;

	bool backupTraceUsed = false;
	if (ceilingFraction < 1.0f || result.startsolid)
	{
		trace_t backupresult;
		const float backupDist = hullWidth * 0.25f;
		Vector backupFeet(feet - climbDir * backupDist);
		Vector backupCeiling(backupFeet + Vector(0, 0, mover->GetMaxJumpHeight()));

		trace::hull(backupFeet, backupCeiling, skipStepHeightHullMin, skipStepHeightHullMax, mask, &filter, backupresult);

		if (backupresult.startsolid == false && backupresult.fraction > ceilingFraction)
		{
			result = backupresult;
			ceilingFraction = result.fraction;
			feet = backupFeet;
			ceiling = backupCeiling;
			backupTraceUsed = true;
		}
	}

	float maxLedgeHeight = ceilingFraction * mover->GetMaxJumpHeight();

	if (maxLedgeHeight <= mover->GetStepHeight())
	{
		return false;
	}

	Vector climbHullMax(halfWidth, halfWidth, maxLedgeHeight);

	Vector ledgePos = feet;

	trace::hull(feet, feet + climbDir * ledgeLookAheadDist, skipStepHeightHullMin, climbHullMax, mask, &filter, result);

	if (bot->IsDebugging(BOTDEBUG_PATH) && debug)
	{
		NDebugOverlay::SweptBox(feet, feet + climbDir * ledgeLookAheadDist, skipStepHeightHullMin, climbHullMax, vec3_angle, 255, 100, 0, 255, 0.2f);
	}

	bool wasPotentialLedgeFound = result.DidHit() && !result.startsolid;

	if (wasPotentialLedgeFound)
	{
		if (bot->IsDebugging(BOTDEBUG_PATH) && debug)
		{
			NDebugOverlay::SweptBox(feet, feet + climbDir * ledgeLookAheadDist, skipStepHeightHullMin, climbHullMax, vec3_angle, 255, 100, 0, 255, 0.2f);
		}

		// what are we climbing over?
		CBaseEntity* hitentity = result.m_pEnt;
		int entindex = gamehelpers->EntityToBCompatRef(hitentity);
		edict_t* obstacle = gamehelpers->EdictOfIndex(entindex);

		if (!result.DidHitNonWorldEntity() || mover->IsAbleToClimbOntoEntity(obstacle))
		{

			// the low hull sweep hit an obstacle - note how 'far in' this is
			float ledgeFrontWallDepth = ledgeLookAheadDist * result.fraction;

			float minLedgeDepth = mover->GetHullWidth() / 2.0f;
			if (m_goal->type == AIPath::SegmentType::SEGMENT_CLIMB_UP)
			{
				// Climbing up to a narrow nav area indicates a narrow ledge.  We need to reduce our minLedgeDepth
				// here or our path will say we should climb but we'll forever fail to find a wide enough ledge.
				auto afterClimb = GetNextSegment(m_goal);
				if (afterClimb && afterClimb->area)
				{
					Vector depthVector = climbDir * minLedgeDepth;
					depthVector.z = 0;
					if (fabsf(depthVector.x) > afterClimb->area->GetSizeX())
					{
						depthVector.x = (depthVector.x > 0) ? afterClimb->area->GetSizeX() : -afterClimb->area->GetSizeX();
					}
					if (fabsf(depthVector.y) > afterClimb->area->GetSizeY())
					{
						depthVector.y = (depthVector.y > 0) ? afterClimb->area->GetSizeY() : -afterClimb->area->GetSizeY();
					}

					float areaDepth = depthVector.NormalizeInPlace();
					minLedgeDepth = std::min(minLedgeDepth, areaDepth);
				}
			}

			float ledgeHeight = minLedgeHeight;
			const float ledgeHeightIncrement = mover->GetStepHeight() / 2.0f;
			bool foundWall = false;
			bool foundLedge = false;
			float ledgeTopLookAheadRange = ledgeLookAheadDist;
			Vector climbHullMin(-halfWidth, -halfWidth, 0.0f);
			Vector climbHullMax(halfWidth, halfWidth, minHullHeight);
			// Vector wallPos;
			float wallDepth = 0.0f;

			bool isLastIteration = false;
			while (true)
			{

				trace::hull(feet + Vector(0.0f, 0.0f, ledgeHeight), feet + Vector(0.0f, 0.0f, ledgeHeight) + climbDir * ledgeTopLookAheadRange,
					climbHullMin, climbHullMax, mask, &filter, result);

				float traceDepth = ledgeTopLookAheadRange * result.fraction;

				if (!result.startsolid)
				{
					if (foundWall)
					{
						if ((traceDepth - ledgeFrontWallDepth) > minLedgeDepth)
						{
							bool isUsable = true;
							ledgePos = result.endpos;

							trace::hull(ledgePos, ledgePos + Vector(0, 0, -ledgeHeightIncrement), climbHullMin, climbHullMax, mask, &filter, result);

							ledgePos = result.endpos;

							constexpr auto MinGroundNormal = 0.7f;
							if (result.allsolid || !result.DidHit() || result.plane.normal.z < MinGroundNormal)
							{
								isUsable = false; // to steep for players to climb
							}
							else
							{
								if (heightDelta > 0.0f)
								{
									// if we're climbing to a specific ledge via a CLIMB_UP link, only climb to that ledge.
									// Do this only for the world (which includes static props) so we can still opportunistically
									// climb up onto breakable railings and physics props.
									if (result.DidHitWorld())
									{
										float potentialLedgeHeight = result.endpos.z - feet.z;
										if (fabsf(potentialLedgeHeight - heightDelta) > tolerance)
										{
											isUsable = false;
										}
									}
								}
							}

							if (isUsable)
							{
								Vector validLedgePos = ledgePos;
								constexpr auto edgeTolerance = 4.0f;
								const float maxBackUp = hullWidth;
								float backUpSoFar = edgeTolerance;
								Vector testPos = ledgePos;

								while (backUpSoFar < maxBackUp)
								{
									testPos -= edgeTolerance * climbDir;
									backUpSoFar += edgeTolerance;

									trace::hull(testPos, Vector(0.0f, 0.0f, -ledgeHeightIncrement), climbHullMin, climbHullMax, mask, &filter, result);

									if (bot->IsDebugging(BOTDEBUG_PATH) && debug)
									{
										NDebugOverlay::SweptBox(testPos, testPos + Vector(0, 0, -mover->GetStepHeight()),
											climbHullMin, climbHullMax, vec3_angle, 255, 0, 0, 255, 5.0f);
									}

									if (result.DidHit() && result.plane.normal.z >= MinGroundNormal)
									{
										ledgePos = result.endpos;
									}
									else
									{
										break;
									}
								}

								ledgePos += climbDir * halfWidth;
								Vector climbHullMinStep(climbHullMin);

								trace::hull(validLedgePos, ledgePos, climbHullMinStep, climbHullMax, mask, &filter, result);

								ledgePos = result.endpos;

								// now that we have a ledge position, trace to find the actual ground
								trace::hull(ledgePos + Vector(0.0f, 0.0f, mover->GetStepHeight()), ledgePos, climbHullMin, climbHullMax, mask, &filter, result);

								if (!result.startsolid)
								{
									ledgePos = result.endpos;
								}
							}

							if (isUsable)
							{
								// found a usable ledge here
								foundLedge = true;
								break;
							}
						}
					}
					else if (result.DidHit())
					{
						foundWall = true;
						wallDepth = traceDepth;

						// make sure the subsequent traces are at least minLedgeDepth deeper than
						// the wall we just found, or all ledge checks will fail
						float minTraceDepth = traceDepth + minLedgeDepth + 0.1f;

						if (ledgeTopLookAheadRange < minTraceDepth)
						{
							ledgeTopLookAheadRange = minTraceDepth;
						}
					}
					else if (ledgeHeight > mover->GetCrouchedHullHeight() && !isPlannedClimb)
					{
						break;
					}
				}

				ledgeHeight += ledgeHeightIncrement;

				if (ledgeHeight >= maxLedgeHeight)
				{
					if (isLastIteration)
					{
						break;
					}

					isLastIteration = true; // exit the inf loop
					ledgeHeight = maxLedgeHeight;
				}
			}

			if (foundLedge)
			{
				if (!mover->ClimbUpToLedge(ledgePos, climbDir, obstacle))
				{
					return false;
				}

				return true;
			}
		}
	}

	return false;
}

bool CMeshNavigator::JumpOverGaps(CBaseBot* bot, const CBasePathSegment* segment, const Vector& forward, const Vector& right, const float goalRange)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("CMeshNavigator::JumpOverGaps", "NavBot");
#endif // EXT_VPROF_ENABLED

	auto mover = bot->GetMovementInterface();
	auto input = bot->GetControlInterface();

	if (!mover->IsAbleToJumpAcrossGap())
	{
		return false;
	}

	if (mover->IsClimbingOrJumping() || mover->IsAscendingOrDescendingLadder() || !mover->IsOnGround())
	{
		return false;
	}

	if (m_goal == nullptr)
	{
		return false;
	}

	// trace_t result;
	// CMovementTraverseFilter filter(bot, mover, true);

	const float hullWidth = mover->GetHullWidth();

	auto current = GetPriorSegment(m_goal);
	if (current == nullptr)
	{
		return false;
	}

	const float minGapJumpRange = 2.0f * hullWidth;

	const CBasePathSegment* gap = nullptr;

	if (current->type == AIPath::SegmentType::SEGMENT_JUMP_OVER_GAP)
	{
		gap = current;
	}
	else
	{
		float searchRange = goalRange;

		for (auto seg = m_goal; seg != nullptr; seg = GetNextSegment(seg))
		{
			if (searchRange > minGapJumpRange)
			{
				break;
			}

			if (seg->type == AIPath::SegmentType::SEGMENT_JUMP_OVER_GAP)
			{
				gap = seg;
				break;
			}

			searchRange += seg->length;
		}
	}

	if (gap != nullptr)
	{

		const float halfWidth = hullWidth / 2.0f;

		if (mover->IsGap(bot->GetAbsOrigin() + halfWidth * gap->forward, gap->forward))
		{
			// there is a gap to jump over
			auto landing = GetNextSegment(gap);
			if (landing != nullptr)
			{
				mover->JumpAcrossGap(landing->goal, landing->forward);

				// if we're jumping over this gap, make sure our goal is the landing so we aim for it
				m_goal = landing;
				
				if (bot->IsDebugging(BOTDEBUG_PATH))
				{
					NDebugOverlay::Cross3D(m_goal->goal, 5.0f, 0, 255, 255, true, 5.0f);
					bot->DebugPrintToConsole(0, 255, 255, "%s [CMeshNavigator] JUMPING OVER GAP \n", bot->GetDebugIdentifier());
				}

				return true;
			}
		}
	}

	return false;
}

Vector CMeshNavigator::Avoid(CBaseBot* bot, const Vector& goalPos, const Vector& forward, const Vector& left)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("CMeshNavigator::Avoid", "NavBot");
#endif // EXT_VPROF_ENABLED

	if (!m_avoidTimer.IsElapsed())
	{
		return goalPos;
	}

	// don't check too frequently unless we hit something
	constexpr auto avoidInterval = 0.5f;
	m_avoidTimer.Start(avoidInterval);

	auto mover = bot->GetMovementInterface();

	if (mover->IsClimbingOrJumping() || !mover->IsOnGround())
	{
		return goalPos;
	}

	//
	// Check for potential blockers along our path and wait if we're blocked
	//
	const Vector botorigin = bot->GetAbsOrigin();
	auto blocker = FindBlocker(bot);
	if (blocker != nullptr)
	{
		// wait for some time
		m_waitTimer.Start(avoidInterval * randomgen->GetRandomReal<float>(1.0f, 2.0f));
		gamehelpers->SetHandleEntity(m_blocker, blocker);

		return botorigin;
	}

	// don't avoid in precise navigation areas
	CNavArea* area = bot->GetLastKnownNavArea();
	if (area != nullptr && (area->GetAttributes() & NAV_MESH_PRECISE) != 0)
	{
		return goalPos;
	}

	// don't avoid if the goal nav area needs precise navigation
	if (m_goal && (m_goal->area->GetAttributes() & NAV_MESH_PRECISE) != 0)
	{
		return goalPos;
	}

	m_didAvoidCheck = true;

	trace_t result;
	trace::CTraceFilterNoNPCsOrPlayers filter(bot->GetEntity(), COLLISION_GROUP_NONE);

	unsigned int mask = mover->GetMovementTraceMask();

	const float size = mover->GetHullWidth() / 4.0f;
	const float offset = size + 2.0f;

	float range = 50.0f;
	range *= bot->GetModelScale();

	auto hullMin = Vector(-size, -size, mover->GetStepHeight() + 0.1f);
	auto hullMax = Vector(size, size, mover->GetCrouchedHullHeight());

	Vector nextStepHullMin(-size, -size, 2.0f * mover->GetStepHeight() + 0.1f);

	CBaseEntity* door = nullptr;
	CBaseEntity* leftEnt = nullptr;
	CBaseEntity* rightEnt = nullptr;

	// check left side
	Vector leftFrom = bot->GetAbsOrigin() + offset * left;
	Vector leftTo = leftFrom + range * forward;

	bool isLeftClear = true;
	float leftAvoid = 0.0f;

	CMovementTraverseFilter movementfilter(bot, mover, false);
	trace::hull(leftFrom, leftTo, hullMin, hullMax, mask, &movementfilter, result);

	if (result.fraction < 1.0f || result.startsolid)
	{
		// emulate being against a wall
		if (result.startsolid)
		{
			result.fraction = 0.0f;
		}

		leftAvoid = std::clamp(1.0f - result.fraction, 0.0f, 1.0f);

		isLeftClear = false;

		// track any doors we need to avoid
		if (result.DidHitNonWorldEntity())
		{
			if (UtilHelpers::FClassnameIs(result.m_pEnt, "prop_door*"))
			{
				door = result.m_pEnt;
			}
		}

		leftEnt = result.m_pEnt;
	}

	// check right side
	Vector rightFrom = bot->GetAbsOrigin() - offset * left;
	Vector rightTo = rightFrom + range * forward;

	bool isRightClear = true;
	float rightAvoid = 0.0f;

	trace::hull(rightFrom, rightTo, hullMin, hullMax, mask, &movementfilter, result);

	if (result.fraction < 1.0f || result.startsolid)
	{
		// emulate being against a wall
		if (result.startsolid)
		{
			result.fraction = 0.0f;
		}

		rightAvoid = std::clamp(1.0f - result.fraction, 0.0f, 1.0f);

		isRightClear = false;

		if (result.DidHitNonWorldEntity())
		{
			if (UtilHelpers::FClassnameIs(result.m_pEnt, "prop_door*"))
			{
				door = result.m_pEnt;
			}
		}

		rightEnt = result.m_pEnt;
	}

	Vector adjustedGoal = goalPos;

	// avoid doors directly in our way
	if (door != nullptr && isLeftClear == false && isRightClear == false)
	{
		auto doorent = entities::HBaseEntity(door);

		Vector forward, right, up;
		AngleVectors(doorent.GetAbsAngles(), &forward, &right, &up);

		constexpr float doorWidth = 100.0f;
		Vector doorEdge = doorent.GetAbsOrigin() - doorWidth * right;

		adjustedGoal.x = doorEdge.x;
		adjustedGoal.y = doorEdge.y;

		m_avoidTimer.Invalidate();
	}
	else if (isLeftClear == false || isRightClear == false)
	{
		float avoidResult = 0.0f;
		if (isLeftClear)
		{
			avoidResult = -rightAvoid;
		}
		else if (isRightClear)
		{
			avoidResult = leftAvoid;
		}
		else
		{
			// both left and right are blocked, avoid nearest
			const float equalTolerance = 0.01f;
			if (fabsf(rightAvoid - leftAvoid) < equalTolerance)
			{
				return adjustedGoal;
			}
			if (rightAvoid > leftAvoid)
			{
				avoidResult = -rightAvoid;
			}
			else
			{
				avoidResult = leftAvoid;
			}
		}

		// adjust goal to avoid obstacle
		Vector avoidDir = 0.5f * forward - left * avoidResult;
		avoidDir.NormalizeInPlace();

		adjustedGoal = bot->GetAbsOrigin() + 100.0f * avoidDir;

		// do avoid check again next frame
		m_avoidTimer.Invalidate();
	}

	// If the bot did not hit a door and the blocking entity is the same for both left and right, notify movement
	if (door == nullptr && isLeftClear == false && isRightClear == false && leftEnt == rightEnt)
	{
		mover->ObstacleOnPath(leftEnt, goalPos, forward, left);
	}

	if (bot->IsDebugging(BOTDEBUG_PATH))
	{
		static QAngle dbgangles(0, 0, 0);

		if (isLeftClear)
			NDebugOverlay::SweptBox(leftFrom, leftTo, hullMin, hullMax, dbgangles, 0, 255, 0, 255, 0.2f);
		else
			NDebugOverlay::SweptBox(leftFrom, leftTo, hullMin, hullMax, dbgangles, 255, 0, 0, 255, 0.2f);

		if (isRightClear)
			NDebugOverlay::SweptBox(rightFrom, rightTo, hullMin, hullMax, dbgangles, 0, 255, 0, 255, 0.2f);
		else
			NDebugOverlay::SweptBox(rightFrom, rightTo, hullMin, hullMax, dbgangles, 255, 0, 0, 255, 0.2f);
	}

	return adjustedGoal;
}

edict_t* CMeshNavigator::FindBlocker(CBaseBot* bot)
{
	auto behavior = bot->GetBehaviorInterface();

	// check if the bot cares about blockers
	if (behavior->IsBlocker(bot, nullptr, true) != ANSWER_YES)
		return nullptr;

	auto mover = bot->GetMovementInterface();

	edict_t* blocker = nullptr;
	trace_t result;
	CTraceFilterOnlyActors filter(bot->GetEntity(), COLLISION_GROUP_NONE);

	const float size = mover->GetHullWidth() / 4.0f;
	Vector mins(-size, -size, mover->GetStepHeight());
	Vector maxs(-size, -size, mover->GetCrouchedHullHeight());
	Vector from = bot->GetAbsOrigin();
	float range = 0.0f;
	auto mask = mover->GetMovementTraceMask();

	// maximum distance to look for blockers
	constexpr auto LOOK_FOR_BLOCKERS_AHEAD_MAX = 750.0f;

	MoveCursorToClosestPosition(bot->GetAbsOrigin());

	for (auto segment = GetCursorData().segment; segment != nullptr && range < LOOK_FOR_BLOCKERS_AHEAD_MAX; segment = GetNextSegment(segment))
	{
		Vector traceforward = segment->goal - from;
		float traceRange = traceforward.NormalizeInPlace();

		const float minTraceRange = mover->GetHullWidth() * 2.0f;

		if (traceRange < minTraceRange)
		{
			traceRange = minTraceRange;
		}

		trace::hull(from, from + traceRange * traceforward, mins, maxs, mask, &filter, result);

		if (result.DidHitNonWorldEntity())
		{
			auto hitent = entities::HBaseEntity(result.m_pEnt);
			Vector toBlocker = hitent.GetAbsOrigin() - bot->GetAbsOrigin();
			Vector alongPath = segment->goal - from;
			alongPath.z = 0.0f;

			if (DotProduct(toBlocker, alongPath) > 0.0f && hitent.GetEntity(nullptr, &blocker) == true)
			{
				if (behavior->IsBlocker(bot, blocker) == ANSWER_YES)
				{
					return blocker; // found a blocker the bot cares about
				}
			}
		}

		from = segment->goal;
		range += segment->length;
	}

	return nullptr;
}

void CMeshNavigator::BreakIfNeeded(CBaseBot* bot)
{
	constexpr auto CHECK_DIST = 300.0f * 300.0f;

	if (bot->GetRangeToSqr(m_goal->goal) > CHECK_DIST)
	{
		return;
	}

	CBaseEntity* obstacle = nullptr;

	if (!bot->GetMovementInterface()->IsPotentiallyTraversable(bot->GetAbsOrigin(), m_goal->goal, nullptr, true, &obstacle))
	{
		if (obstacle != nullptr && bot->IsAbleToBreak(obstacle))
		{
			bot->GetMovementInterface()->BreakObstacle(obstacle);
		}
	}
}

void CMeshNavigator::SearchForUseableObstacles(CBaseBot* bot)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("CMeshNavigator::SearchForUseableObstacles", "NavBot");
#endif // EXT_VPROF_ENABLED

	auto goal = GetGoalSegment();

	if (!goal) { return; }

	Vector eyePos = bot->GetEyeOrigin();
	Vector to = (goal->goal - eyePos);
	to.NormalizeInPlace();
	Vector endPos = eyePos + (to * CBaseExtPlayer::PLAYER_USE_RADIUS);
	trace_t tr;
	trace::line(eyePos, endPos, MASK_PLAYERSOLID, bot->GetEntity(), COLLISION_GROUP_PLAYER, tr);

	CBaseEntity* obstacle = tr.m_pEnt;

	if (obstacle && tr.DidHitNonWorldEntity())
	{
		if (bot->GetMovementInterface()->IsUseableObstacle(obstacle))
		{
			Vector aimPos = UtilHelpers::getWorldSpaceCenter(obstacle);
			bot->GetControlInterface()->AimAt(aimPos, IPlayerController::LOOK_USE, sm_navbot_path_useable_scan.GetFloat() + 0.2f, "Looking at useable obstacle on my path!");
			bot->GetControlInterface()->PressUseButton();
		}
	}
}

bool CMeshNavigator::OffMeshLinksUpdate(CBaseBot* bot)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("CMeshNavigator::OffMeshLinksUpdate", "NavBot");
#endif // EXT_VPROF_ENABLED

	if (!m_goal)
	{
		return false;
	}

	switch (m_goal->type)
	{
	case AIPath::SegmentType::SEGMENT_CATAPULT:
	{
		const CBasePathSegment* next = GetNextSegment(m_goal);

		if (next)
		{
			if (bot->GetMovementInterface()->UseCatapult(m_goal->goal, next->goal))
			{
				return true;
			}
		}

		break;
	}
	/*
	case AIPath::SegmentType::SEGMENT_STRAFE_JUMP:
	{
		const CBasePathSegment* next = GetNextSegment(m_goal);
		const float range = bot->GetRangeTo(m_goal->goal) - (bot->GetMovementInterface()->GetHullWidth());

		if (next && range <= GetGoalTolerance())
		{
			if (bot->GetMovementInterface()->DoStrafeJump(m_goal->goal, next->goal))
			{
				return true;
			}
			else
			{
				if (bot->IsDebugging(BOTDEBUG_PATH))
				{
					bot->DebugPrintToConsole(255, 0, 0, "%s ERROR: PATH WANTS TO STRAFE JUMP BUT MOVEMENT RETURNED FALSE! \n", bot->GetDebugIdentifier());
				}
			}
		}

		break;
	}
	case AIPath::SegmentType::SEGMENT_GRAPPLING_HOOK:
	{
		const CBasePathSegment* next = GetNextSegment(m_goal);
		const float range = bot->GetRangeTo(m_goal->goal) - (bot->GetMovementInterface()->GetHullWidth());

		if (next && range <= GetGoalTolerance())
		{
			if (bot->GetMovementInterface()->UseGrapplingHook(m_goal->goal, next->goal))
			{
				return true;
			}
		}

		break;
	} */
	}

	return false;
}

void CMeshNavigator::OnGoalSegmentReached(const CBasePathSegment* goal, const CBasePathSegment* next) const
{
	CBaseBot* bot = GetBot();

	if (!bot || !next) { return; }

	IMovement* mover = bot->GetMovementInterface();
	const Vector origin = bot->GetAbsOrigin();

	switch (goal->type)
	{
	case AIPath::SegmentType::SEGMENT_GRAPPLING_HOOK:
	{
		mover->UseGrapplingHook(origin, next->goal);
		break;
	}
	case AIPath::SegmentType::SEGMENT_STRAFE_JUMP:
	{
		if (!bot->GetMovementInterface()->DoStrafeJump(goal->goal, next->goal))
		{
			if (bot->IsDebugging(BOTDEBUG_PATH))
			{
				bot->DebugPrintToConsole(255, 0, 0, "%s ERROR: PATH WANTS TO STRAFE JUMP BUT MOVEMENT RETURNED FALSE! \n", bot->GetDebugIdentifier());
			}
		}

		break;
	}
	}

}

void CMeshNavigator::AdvanceGoalToNearest()
{
	if (!IsValid() || GetBot() == nullptr)
	{
		return;
	}

	CBaseBot* me = GetBot();

	const CBasePathSegment* start = m_goal != nullptr ? m_goal : GetFirstSegment();

	if (!start)
	{
		return;
	}

	const CBasePathSegment* nearest = nullptr;
	float best_distance = me->GetRangeTo(start->goal);
	unsigned char max_i = 0U;
	constexpr unsigned char MAX_SEARCH_ITERATIONS = 15U;

	const CBasePathSegment* next = GetNextSegment(start);

	while (next != nullptr)
	{
		float dist = me->GetRangeTo(next->goal);

		if (dist < best_distance)
		{
			nearest = next;
			best_distance = dist;
			next = GetNextSegment(next);
		}
		else
		{
			if (max_i < MAX_SEARCH_ITERATIONS)
			{
				next = GetNextSegment(next);
				max_i++;
				continue;
			}

			break;
		}
	}

	if (nearest != nullptr && nearest != m_goal)
	{
		if (me->IsDebugging(BOTDEBUG_PATH))
		{
			me->DebugPrintToConsole(255, 215, 0, "%s PATH GOAL SEGMENT ADJUSTED FROM %g %g %g TO %g %g %g \n", me->GetDebugIdentifier(),
				start->goal.x, start->goal.y, start->goal.z, nearest->goal.x, nearest->goal.y, nearest->goal.z);
			Vector top = nearest->goal;
			top.z += 32.0f;
			NDebugOverlay::VertArrow(top, nearest->goal, 4.0f, 255, 215, 0, 255, true, 15.0f);

			top = start->goal;
			top.z += 32.0f;
			NDebugOverlay::VertArrow(top, start->goal, 4.0f, 255, 0, 0, 255, true, 15.0f);
		}

		m_goal = nearest;
	}
}

bool CMeshNavigator::LadderUpdate(CBaseBot* bot)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("CMeshNavigator::LadderUpdate", "NavBot");
#endif // EXT_VPROF_ENABLED

	auto input = bot->GetControlInterface();
	auto mover = bot->GetMovementInterface();
	Vector origin = bot->GetAbsOrigin();

	if (mover->IsUsingLadder())
	{
		return true; // movement is handled by the movement interface
	}

	if (m_goal->ladder == nullptr)
	{
		if (mover->IsOnLadder())
		{
			auto current = GetPriorSegment(m_goal);

			if (current == nullptr)
			{
				return false;
			}

			constexpr auto LADDER_LOOK_AHEAD_RANGE = 50.0f;
			for (auto seg = current; seg != nullptr; seg = GetNextSegment(seg))
			{
				if (seg != current && ((seg->goal - origin)).AsVector2D().IsLengthGreaterThan(LADDER_LOOK_AHEAD_RANGE))
				{
					break;
				}

				if (seg->ladder != nullptr && seg->how == GO_LADDER_DOWN && seg->ladder->m_length > mover->GetMaxJumpHeight())
				{
					float heightDelta = seg->goal.z - origin.z;

					if (fabsf(heightDelta) < mover->GetMaxJumpHeight())
					{
						m_goal = seg;
						break;
					}
				}
			}
		}

		if (m_goal->ladder == nullptr)
		{
			return false; // no ladder to use
		}
	}

	constexpr auto LADDER_MOUNT_RANGE = 50.0f;

	if (m_goal->how == GO_LADDER_UP)
	{
		if (!mover->IsUsingLadder() && origin.z > m_goal->ladder->m_top.z - mover->GetStepHeight())
		{
			// we've reached the ladder top
			m_goal = GetNextSegment(m_goal);
			return false;
		}

		Vector goal = m_goal->ladder->m_bottom;
		goal.z = m_goal->ladder->ClampZ(m_me->GetAbsOrigin().z);
		Vector2D to = (goal - origin).AsVector2D();
		input->AimAt(m_goal->ladder->m_top - 50.0f * m_goal->ladder->GetNormal() + Vector(0.0f, 0.0f, mover->GetCrouchedHullHeight()),
			IPlayerController::LOOK_MOVEMENT, 2.0f, "Looking at ladder to climb up!");

		const float range = to.NormalizeInPlace();
		constexpr float LADDER_ALIGN_RANGE = 128.0f;

		if (range < LADDER_ALIGN_RANGE)
		{
			Vector2D ladderNormal2D = m_goal->ladder->GetNormal().AsVector2D();
			float dot = DotProduct2D(ladderNormal2D, to);
			constexpr float cos = 0.9f;

			if (dot < cos)
			{
				// we're facing the ladder, just move straigh to it
				mover->MoveTowards(goal, IMovement::MOVEWEIGHT_PRIORITY);

				if (range < LADDER_MOUNT_RANGE)
				{
					// start climbing ladder
					mover->ClimbLadder(m_goal->ladder, m_goal->area);

				}
			}
			else // bot is not aligned with the ladder, rotate around it
			{
				Vector botperp(-to.y, to.x, 0.0f);
				Vector2D ladderperp(-ladderNormal2D.y, ladderNormal2D.x);

				Vector aligngoal = m_goal->ladder->m_bottom;
				float alignrange = LADDER_ALIGN_RANGE;

				if (dot < 0.0f)
				{
					// we're not behind the ladder
					alignrange = LADDER_MOUNT_RANGE + (1.0f * dot) * (alignrange - LADDER_MOUNT_RANGE);
				}

				aligngoal.x -= alignrange * to.x;
				aligngoal.y -= alignrange * to.y;

				if (DotProduct2D(to, ladderperp) < 0.0f)
				{
					aligngoal += 10.0f * botperp;
				}
				else
				{
					aligngoal -= 10.0f * botperp;
				}

				mover->MoveTowards(aligngoal, IMovement::MOVEWEIGHT_PRIORITY);
			}
		}
		else
		{
			return false; // use normal path finding to reach the ladder
		}	
	}
	else // going down a ladder
	{
		if (origin.z < m_goal->ladder->m_bottom.z + mover->GetStepHeight())
		{
			// bot origin is below the ladder botton origin, assume the bot is off the ladder
			m_goal = GetNextSegment(m_goal);
			return false;
		}
		
		auto ladder = m_goal->ladder;
		auto top = ladder->m_top;
		auto bottom = ladder->m_bottom;
		Vector mountPoint = top + (0.5f * mover->GetHullWidth() * ladder->GetNormal());
		mountPoint.z = m_goal->ladder->ClampZ(m_me->GetAbsOrigin().z);
		Vector2D to = (mountPoint - origin).AsVector2D();

		input->AimAt(bottom + 50.0f * ladder->GetNormal() + Vector(0.0f, 0.0f, mover->GetCrouchedHullHeight()), IPlayerController::LOOK_MOVEMENT, 0.25f);

		float range = to.NormalizeInPlace();

		// double mount range when going down, the movement interface handles this a lot better than us
		if (range < (LADDER_MOUNT_RANGE * 2.0f) || mover->IsOnLadder())
		{
			mover->DescendLadder(ladder, m_goal->area);

			// go to next goal since the movement interface will auto pilot the ladder
			m_goal = GetNextSegment(m_goal);
		}
		else
		{
			return false; // move towards the ladder top using normal pathing
		}
	}

	return false;
}

bool CMeshNavigator::ElevatorUpdate(CBaseBot* bot)
{
	if (bot->GetMovementInterface()->IsUsingElevator())
	{
		return true;
	}

	if (m_goal->type == AIPath::SegmentType::SEGMENT_ELEVATOR)
	{
		const CNavElevator* elevator = m_goal->area->GetElevator();

		auto next = GetNextSegment(m_goal);

		if (!next)
		{
			if (bot->IsDebugging(BOTDEBUG_MOVEMENT))
			{
				bot->DebugPrintToConsole(255, 0, 0, "%s CMeshNavigator::ElevatorUpdate next == nullptr! \n", bot->GetDebugIdentifier());
			}

			return false;
		}

		bot->GetMovementInterface()->UseElevator(elevator, m_goal->area, next->area);
		return true;
	}
	
	auto next = GetNextSegment(m_goal);

	// Skip some distance when entering elevators
	if (next && next->type == AIPath::SegmentType::SEGMENT_ELEVATOR)
	{
		const float range = bot->GetRangeTo(next->goal);
		Vector origin = bot->GetAbsOrigin();

		if (bot->GetMovementInterface()->IsPotentiallyTraversable(origin, next->goal, nullptr, true, nullptr) && range <= m_skipAheadDistance)
		{
			if (GetNextSegment(next) != nullptr)
			{
				bot->GetMovementInterface()->UseElevator(next->area->GetElevator(), m_goal->area, next->area);
				return true;
			}
		}
	}

	return false;
}

bool CMeshNavigatorAutoRepath::IsRepathNeeded(const Vector& goal)
{
	float tolerance = GetGoalTolerance() * GetGoalTolerance();

	return (goal - m_lastGoal).LengthSqr() > tolerance;
}
