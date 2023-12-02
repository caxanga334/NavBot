#include <extension.h>
#include <bot/basebot.h>
#include <bot/interfaces/movement.h>
#include <bot/interfaces/playercontrol.h>
#include "meshnavigator.h"

CMeshNavigator::CMeshNavigator() : CPath()
{
	m_goal = nullptr;
	m_goalTolerance = 20.0f;
	m_skipAheadDistance = -1.0f;
}

CMeshNavigator::~CMeshNavigator()
{
}

void CMeshNavigator::Invalidate()
{
	m_goal = nullptr;
	m_waitTimer.Invalidate();
	CPath::Invalidate();
}

void CMeshNavigator::OnPathChanged(CBaseBot* bot, AIPath::ResultType result)
{
	switch (result)
	{
	case AIPath::COMPLETE_PATH:
	case AIPath::PARTIAL_PATH:
		m_goal = GetFirstSegment();
		break;
	case AIPath::NO_PATH:
	default:
		m_goal = nullptr;
		break;
	}
}

void CMeshNavigator::Update(CBaseBot* bot)
{


}

bool CMeshNavigator::IsAtGoal(CBaseBot* bot)
{
	IMovement* mover = bot->GetMovementInterface();
	auto current = GetPriorSegment(m_goal);
	auto origin = bot->GetAbsOrigin();
	Vector toGoal = m_goal->goal - origin;

	// m_goal is the segment the bot is moving towards, if there is no prior segment, the bot is at the goal
	if (current == nullptr)
	{
		return true;
	}

	switch (m_goal->type)
	{
	case AIPath::SegmentType::SEGMENT_DROP_FROM_LEDGE:
	{
		auto landing = GetNextSegment(m_goal);

		if (landing == nullptr)
		{
			return true; // goal reached or path is corrupt
		}
		else if (origin.z - landing->goal.z < mover->GetStepHeight())
		{
			return true;
		}

		break;
	}
	case AIPath::SegmentType::SEGMENT_CLIMB_UP:
	{
		auto landing = GetNextSegment(m_goal);

		if (landing == nullptr)
		{
			return true; // goal reached or path is corrupt
		}
		else if (origin.z > m_goal->goal.z + mover->GetStepHeight())
		{
			// bot origin Z is above the climb start
			return true;
		}

		break;
	}
	default:
		break;
	}

	auto next = GetNextSegment(m_goal);

	if (next != nullptr)
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

		if (dot < 0.0001f && fabsf(toGoal.z) < mover->GetStandingHullHeigh())
		{
			float fraction = 0.0f;

			// If it's reachable, then the bot reached it
			if (toGoal.z < mover->GetStepHeight() && 
				mover->IsPotentiallyTraversable(origin, next->goal, fraction, false) == true && 
				mover->HasPotentialGap(origin, next->goal, fraction) == false)
			{
				return true;
			}
		}
	}

	// Check 2D distance to goal
	if (toGoal.IsLengthLessThan(GetGoalTolerance()) == true)
	{
		return true;
	}

	return false;
}
