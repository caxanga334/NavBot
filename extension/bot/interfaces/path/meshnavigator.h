#ifndef SMNAV_BOT_NAV_MESH_NAVIGATOR_H_
#define SMNAV_BOT_NAV_MESH_NAVIGATOR_H_
#pragma once

#include "basepath.h"

// Nav Mesh navigator
class CMeshNavigator : public CPath
{
public:
	CMeshNavigator();
	virtual ~CMeshNavigator();

	virtual void Invalidate() override;
	virtual void OnPathChanged(CBaseBot* bot, AIPath::ResultType result) override;

	// Moves the bot along the path
	virtual void Update(CBaseBot* bot);

	virtual const CBasePathSegment* GetGoalSegment() const override { return m_goal; }

	// Distance to consider the goal reached
	inline virtual void SetGoalTolerance(const float tolerance) { m_goalTolerance = tolerance; }
	inline virtual const float GetGoalTolerance() const { return m_goalTolerance; }
	// Maximum distance to skip path segments and walk in a straight line (if not blocked). Allows smooth movement
	inline virtual void SetSkipAheadDistance(const float distance) { m_skipAheadDistance = distance; }
	inline virtual const float GetSkipAheadDistance() { return m_skipAheadDistance; }

protected:
	// true while the bot is using ladders
	virtual bool LadderUpdate(CBaseBot* bot);
	virtual bool ElevatorUpdate(CBaseBot* bot);
	virtual bool Climbing(CBaseBot* bot, const CBasePathSegment* segment, const Vector& forward, const Vector& right, const float goalRange);
	virtual bool JumpOverGaps(CBaseBot* bot, const CBasePathSegment* segment, const Vector& forward, const Vector& right, const float goalRange);
	virtual Vector Avoid(CBaseBot* bot, const Vector& goalPos, const Vector& forward, const Vector& left);
	virtual edict_t* FindBlocker(CBaseBot* bot);
	virtual void CrouchIfNeeded(CBaseBot* bot);
	virtual void BreakIfNeeded(CBaseBot* bot);

	bool IsAtGoal(CBaseBot* bot);
	bool CheckProgress(CBaseBot* bot);
	const CBasePathSegment* CheckSkipPath(CBaseBot* bot, const CBasePathSegment* from) const;

	void SetBot(CBaseBot* bot) { m_me = bot; }
	// Bot using this navigator, may be NULL
	CBaseBot* GetBot() const { return m_me; }
	// Waiting for obstacles
	bool IsWaitingForSomething() { return !m_waitTimer.IsElapsed(); }

private:
	CBaseBot* m_me; // bot that is using this navigator
	const CBasePathSegment* m_goal; // the segment the bot is currently trying to reach
	CountdownTimer m_waitTimer; // timer for the bot to wait for stuff (lifts, doors, obstacles, ...)
	CountdownTimer m_avoidTimer; // timer for collision avoidance
	CBaseHandle m_blocker; // Entity blocking my path
	bool m_didAvoidCheck;
	float m_goalTolerance;
	float m_skipAheadDistance;
};

/**
 * @brief Nav Mesh navigator that automatically recalculates the path when needed
 */
class CMeshNavigatorAutoRepath : public CMeshNavigator
{
public:
	CMeshNavigatorAutoRepath(float repathInterval = 1.0f)
	{
		m_repathinterval = repathInterval;
		m_repathTimer.Invalidate();
		m_failCount = 0;
	}

	void Invalidate() override
	{
		m_repathTimer.Invalidate();
		CMeshNavigator::Invalidate();
	}

	template <typename CF>
	void Update(CBaseBot* bot, const Vector& goal, CF& costFunctor);

	// Resets the path failure counter
	void ResetFailures() { m_failCount = 0; }

	// Number of times it failed to build a path to the goal position
	int GetPathBuildFailureCount() const { return m_failCount; }

private:
	float m_repathinterval;
	CountdownTimer m_repathTimer; // Time until next repath
	CountdownTimer m_failTimer; // Time to wait if the path failed
	Vector m_lastGoal; // goal from the last valid path
	int m_failCount; // number of times it failed to build a path

	template <typename CF>
	void RefreshPath(CBaseBot* bot, const Vector& goal, CF& costFunctor);

	bool IsRepathNeeded(const Vector& goal);

	void Update(CBaseBot* bot) override
	{
		CMeshNavigator::Update(bot);
	}
};

template<typename CF>
inline void CMeshNavigatorAutoRepath::Update(CBaseBot* bot, const Vector& goal, CF& costFunctor)
{
	// Refresh path if needed
	RefreshPath(bot, goal, costFunctor);

	// Move bot along path
	CMeshNavigator::Update(bot);
}

template<typename CF>
inline void CMeshNavigatorAutoRepath::RefreshPath(CBaseBot* bot, const Vector& goal, CF& costFunctor)
{
	if (IsValid() && !m_repathTimer.IsElapsed())
	{
		return;
	}

	auto mover = bot->GetMovementInterface();

	// Don't repath on these conditions but also force a repath as soon as possible.
	if (mover->IsOnLadder() || mover->IsControllingMovements())
	{
		m_repathTimer.Invalidate();
		return;
	}

	if (!m_failTimer.IsElapsed())
	{
		return;
	}

	if (!IsValid() || IsRepathNeeded(goal))
	{
		bool foundpath = this->ComputePathToPosition<CF>(bot, goal, costFunctor);

		if (!foundpath)
		{
			Invalidate();
			m_failTimer.Start(1.0f); // Wait one second before repath
			m_failCount++;
			bot->OnMoveToFailure(this, IEventListener::MovementFailureType::FAIL_NO_PATH);
		}
		else
		{
			m_repathTimer.Start(m_repathinterval);
		}
	}
}

#endif // !SMNAV_BOT_NAV_MESH_NAVIGATOR_H_
