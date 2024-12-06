#include <extension.h>
#include <bot/basebot.h>
#include "pluginnavigator.h"

CPluginMeshNavigator::CPluginMeshNavigator()
{
}

CPluginMeshNavigator::~CPluginMeshNavigator()
{
}

void CPluginMeshNavigator::Update(CBaseBot* bot)
{
	auto goal = GetGoalSegment();

	if (!IsValid() || goal == nullptr)
	{
		return;
	}

	SetBot(bot);
	bot->SetActiveNavigator(this);

	if (!CheckProgress(bot))
	{
		return; // goal reached
	}

	IMovement* mover = bot->GetMovementInterface();
	Vector origin = bot->GetAbsOrigin();
	Vector forward = goal->goal - origin;
	auto input = bot->GetControlInterface();

	if (goal->type == AIPath::SegmentType::SEGMENT_CLIMB_UP || goal->type == AIPath::SegmentType::SEGMENT_CLIMB_DOUBLE_JUMP)
	{
		auto next = GetNextSegment(goal);
		if (next != nullptr)
		{
			forward = next->goal - origin;
		}
	}

	forward.z = 0.0f;
	const float goalRange = forward.NormalizeInPlace();
	Vector left(-forward.y, forward.x, 0.0f);

	if (left.IsZero())
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

	forward = CrossProduct(normal, forward);
	left = CrossProduct(left, normal);

	// It's possible that the path become invalid after a jump or climb attempt
	if (!IsValid())
	{
		return;
	}

	CNavArea* myarea = bot->GetLastKnownNavArea();
	const bool isOnStairs = myarea->HasAttributes(NAV_MESH_STAIRS);
	const float tooHighDistance = mover->GetMaxJumpHeight();

	if (goal->ladder == nullptr && !mover->IsClimbingOrJumping() && !isOnStairs && goal->goal.z > origin.z + tooHighDistance)
	{
		constexpr auto CLOSE_RANGE = 25.0f;
		Vector2D to(origin.x - goal->goal.x, origin.y - goal->goal.y);

		if (mover->IsStuck() || to.IsLengthLessThan(CLOSE_RANGE))
		{
			auto next = GetNextSegment(goal);
			if (mover->IsStuck() || !next || (next->goal.z - origin.z > mover->GetMaxJumpHeight()) || !mover->IsPotentiallyTraversable(origin, next->goal, nullptr))
			{
				bot->OnMoveToFailure(this, IEventListener::FAIL_FELL_OFF_PATH);

				if (GetAge() > 0.0f)
				{
					Invalidate();
				}

				mover->ClearStuckStatus();

				return;
			}
		}
	}

	Vector goalPos = goal->goal;

	// avoid small obstacles
	forward = goalPos - origin;
	forward.z = 0.0f;
	float rangeToGoal = forward.NormalizeInPlace();

	left.x = -forward.y;
	left.y = forward.x;
	left.z = 0.0f;

	constexpr auto nearLedgeRange = 50.0f;
	if (rangeToGoal > nearLedgeRange || (goal && goal->type != AIPath::SegmentType::SEGMENT_CLIMB_UP))
	{
		auto next = GetNextSegment(goal);
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

	m_moveGoal = goalPos;
}
