#include <extension.h>
#include <bot/basebot.h>
#include <sdkports/debugoverlay_shared.h>
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

	IMovement* mover = bot->GetMovementInterface();
	SetBot(bot);
	bot->SetActiveNavigator(this);
	const bool runcmd = bot->RunPlayerCommands();

	if (runcmd)
	{
		if (mover->IsControllingMovements())
		{
			// bot movements are under control of the movement interface, likely performing an advanced movement that requires multiple steps such as a rocket jump.
			// Wait until it's done to continue moving the bot.
			return;
		}

		if (IsWaitingForSomething())
		{
			return; // wait for something to stop blocking our path (like a door opening)
		}

		if (LadderUpdate(bot))
		{
			return; // bot is using a ladder
		}
	}

	if (!CheckProgress(bot))
	{
		return; // goal reached
	}

	if (runcmd)
	{
		mover->DetermineIdealPostureForPath(this);
		mover->AdjustSpeedForPath(this);
	}
	
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

	if (runcmd)
	{
		if (!Climbing(bot, goal, forward, left, goalRange))
		{
			if (!IsValid())
			{
				return; // path might become invalid from a failed climb
			}

			JumpOverGaps(bot, goal, forward, left, goalRange);
		}
	}

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

	if (mover->IsOnGround())
	{
		auto eyes = bot->GetEyeOrigin();
		Vector lookat(goalPos.x, goalPos.y, eyes.z);

		// low priority look towards movement goal so the bot doesn't walk with a fixed look
		input->AimAt(lookat, IPlayerController::LOOK_PATH, 0.1f);
	}

	// move bot along path
	mover->MoveTowards(goalPos, IMovement::MOVEWEIGHT_NAVIGATOR);
	m_moveGoal = goalPos;

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
