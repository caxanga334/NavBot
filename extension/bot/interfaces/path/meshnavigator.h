#ifndef SMNAV_BOT_NAV_MESH_NAVIGATOR_H_
#define SMNAV_BOT_NAV_MESH_NAVIGATOR_H_
#pragma once

#include "basepath.h"

// Nav Mesh navigator
class CMeshNavigator : public CPath
{
public:
	CMeshNavigator();
	~CMeshNavigator() override;

	void Invalidate() override;
	void OnPathChanged(CBaseBot* bot, AIPath::ResultType result) override;

	// Moves the bot along the path
	virtual void Update(CBaseBot* bot);

	const BotPathSegment* GetGoalSegment() const override { return m_goal; }
	// Distance to consider the goal reached
	virtual void SetGoalTolerance(const float tolerance) { m_goalTolerance = tolerance; }
	virtual const float GetGoalTolerance() const { return m_goalTolerance; }
	// Maximum distance to skip path segments and walk in a straight line (if not blocked). Allows smooth movement
	virtual void SetSkipAheadDistance(const float distance) { m_skipAheadDistance = distance; }
	virtual const float GetSkipAheadDistance() { return m_skipAheadDistance; }
	// Updates the goal segment to the segment nearest to the bot.
	void AdvanceGoalToNearest();
	// Returns true if the bot is currently trying to +USE an useable entity blocking the path.
	bool IsUsingEntity() const { return !m_useEntityTimer.IsElapsed(); }
	// Gets the last +USE target entity. NULL if none or entity was deleted.
	CBaseEntity* GetUseTarget() const { return m_useTarget.Get(); }
	bool ObstacleAvoidanceIsLeftBlocked() const { return !m_avoidIsLeftClear; }
	bool ObstacleAvoidanceIsRightBlocked() const { return !m_avoidIsRightClear; }
	bool ObstacleAvoidanceIsPathFullyBlocked() const { return !m_avoidIsLeftClear && !m_avoidIsRightClear; }
	bool ObstacleAvoidanceIsPathBlockedBySomething() const { return !m_avoidIsLeftClear || !m_avoidIsRightClear; }
	CBaseEntity* ObstacleAvoidanceGetLeftBlocker() const { return m_avoidLeftEntity.Get(); }
	CBaseEntity* ObstacleAvoidanceGetRightBlocker() const { return m_avoidRightEntity.Get(); }
	CBaseEntity* ObstacleAvoidanceGetNearestObstacle() const { return m_avoidObstacle.Get(); }
	CBaseEntity* GetAvoidingEntity() const { return m_avoidingEntity.Get(); }
	const Vector& GetMoveToPos() const { return m_moveToPos; }
	const Vector& GetDodgePos() const { return m_dodgeGoal; }
	bool IsDodging() const { return m_isDodging; }

protected:
	// true while the bot is using ladders
	bool LadderUpdate(CBaseBot* bot);
	bool ElevatorUpdate(CBaseBot* bot);
	bool Climbing(CBaseBot* bot, const BotPathSegment* segment, const Vector& forward, const Vector& right, const float goalRange);
	bool JumpOverGaps(CBaseBot* bot, const BotPathSegment* segment, const Vector& forward, const Vector& right, const float goalRange);
	Vector Avoid(CBaseBot* bot, const Vector& goalPos, const Vector& forward, const Vector& left);
	CBaseEntity* FindBlocker(CBaseBot* bot);
	bool OffMeshLinksUpdate(CBaseBot* bot);
	void OnGoalSegmentReached(const BotPathSegment* goal, const BotPathSegment* next) const;

	bool IsAtGoal(CBaseBot* bot);
	bool IsAtUnderwaterGoal(CBaseBot* bot);
	bool CheckProgress(CBaseBot* bot);
	const BotPathSegment* CheckSkipPath(CBaseBot* bot, const BotPathSegment* from) const;

	void SetBot(CBaseBot* bot) { m_me = bot; }
	// Bot using this navigator, may be NULL
	CBaseBot* GetBot() const { return m_me; }
	// Waiting for obstacles
	bool IsWaitingForSomething() { return !m_waitTimer.IsElapsed(); }

	bool HandleWaterMove(CBaseBot* bot, Vector& goalPos, const BotPathSegment* seg);
	/**
	 * @brief Scans the path for obstacles.
	 * @param bot Bot currently using this navigator.
	 * @param goal Current segment the bot is trying to reach.
	 * @return Return true to return early in the navigator update function. False to continue the function.
	 */
	bool CheckForObstacles(CBaseBot* bot, const BotPathSegment* goal);
	/**
	 * @brief Instructs the navigator to +USE an entity on the bot's path.
	 * @param bot Bot that will +USE the entity.
	 * @param entity Entity to +USE
	 * @param time Timeout timer.
	 */
	void UseEntity(CBaseBot* bot, CBaseEntity* entity, const float time = 3.0f);
	// Handles the +USE entity logic
	void UpdateUseEntity(CBaseBot* bot, Vector& moveGoal);
	void StopUsingEntity() { m_useEntityTimer.Invalidate(); }
	void StartUseEntityCooldown(float time) { m_useEntityCooldown.Start(time); }
	void SetAvoidingEntity(CBaseEntity* entity) { m_avoidingEntity = entity; }
	bool IsUseEntityInCooldown() const { return !m_useEntityCooldown.IsElapsed(); }
	bool IsUseEntityADoor() const { return m_isUsingADoor; }
	void SetMoveToPos(const Vector& pos) { m_moveToPos = pos; }
	void SetLastPathObstacle(CBaseEntity* pEnt) { m_pLastObstacle = pEnt; }
	// This entity pointer may be dangling, use it only for comparing with another pointer.
	CBaseEntity* GetLastPathObstacle() const { return m_pLastObstacle; }
	// Sets the goal segment (the segment the navigator will move the bot to).
	void SetGoalSegment(const BotPathSegment* goal)
	{
		OnGoalSegmentReached(m_goal, goal);
		m_goal = goal;
	}
	/**
	 * @brief Queries if it' possible to teleport the bot to the given segment.
	 * @param bot Bot to teleport.
	 * @param segment Segment to teleport to.
	 * @return True if possible, false otherwise.
	 */
	bool IsTeleportPossible(CBaseBot* bot, const BotPathSegment* segment) const;
	/**
	 * @brief Teleports the bot to the given segment.
	 * @param bot Bot to teleport.
	 * @param segment Segment to teleport to.
	 */
	void TeleportToSegment(CBaseBot* bot, const BotPathSegment* segment) const;

	CountdownTimer& GetDodgeTimer() { return m_dodgeTimer; }

	void DodgeTo(const Vector& goal, const float duration)
	{
		m_isDodging = true;
		m_dodgeGoal = goal;
		m_dodgeTimer.Start(duration);
	}

private:
	CBaseBot* m_me; // bot that is using this navigator
	const BotPathSegment* m_goal; // the segment the bot is currently trying to reach
	CountdownTimer m_waitTimer; // timer for the bot to wait for stuff (lifts, doors, obstacles, ...)
	CountdownTimer m_avoidTimer; // timer for collision avoidance
	CountdownTimer m_useableTimer; // timer for checking for useable obstacles on the path
	CountdownTimer m_obstructedTimer; // timer for checking how long the path has been obstructed
	CountdownTimer m_obstacleScanTimer; // timer for checking for obstacles
	CountdownTimer m_dodgeTimer; // timer for dodging path obstacles
	IntervalTimer m_sameObstacleTimer;
	CHandle<CBaseEntity> m_blocker; // Entity blocking my path
	CHandle<CBaseEntity> m_avoidLeftEntity;
	CHandle<CBaseEntity> m_avoidRightEntity;
	CHandle<CBaseEntity> m_avoidObstacle; // Entity nearest blocking our path
	CHandle<CBaseEntity> m_avoidingEntity;
	CBaseEntity* m_pLastObstacle; // last obstacle on the bot's path
	bool m_didAvoidCheck;
	bool m_avoidIsLeftClear;
	bool m_avoidIsRightClear;
	bool m_isDodging;
	bool m_isUsingADoor;
	float m_goalTolerance;
	float m_skipAheadDistance;
	CountdownTimer m_useEntityTimer;
	CountdownTimer m_useEntityCooldown;
	CHandle<CBaseEntity> m_useTarget;
	Vector m_useEntityMoveTo;
	Vector m_useEntityAimAt;
	Vector m_moveToPos; // The position the bot is trying to move to since the last Update call
	Vector m_dodgeGoal;

	bool ShouldBreakObstacles(CBaseBot* bot);
	bool UpdateDodging(CBaseBot* bot);
	void ClassifyUseEntity(CBaseEntity* entity);
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
		m_failCount = 0;
		m_lastGoal = vec3_origin;
	}

	void Invalidate() override
	{
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
	// Within goal reach tolerance, don't move
	if (bot->GetRangeTo(goal) <= GetGoalTolerance())
	{
		bot->OnMoveToSuccess(this);
		return;
	}

	// Refresh path if needed
	RefreshPath(bot, goal, costFunctor);

	// Move bot along path
	CMeshNavigator::Update(bot);
}

template<typename CF>
inline void CMeshNavigatorAutoRepath::RefreshPath(CBaseBot* bot, const Vector& goal, CF& costFunctor)
{
	CountdownTimer* repathtimer = InternalGetRepathTimer();

	if (IsValid() && !repathtimer->IsElapsed())
	{
		return;
	}

	auto mover = bot->GetMovementInterface();

	// Don't repath on these conditions but also force a repath as soon as possible.
	if (mover->IsOnLadder() || mover->IsControllingMovements())
	{
		repathtimer->Invalidate();
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
			repathtimer->Start(m_repathinterval);
		}
	}
}

/**
 * @brief Simple counter for path failures/stuck events
 */
class CPathFailCounter
{
public:
	CPathFailCounter();

	// Increases the counter, retursn true if above the limit
	bool Increase()
	{
		return (++m_count) > m_limit;
	}
	int GetCounter() const { return m_count; }
	void Reset() { m_count = 0; }
	void SetLimit(int v) { m_limit = v; }
	int GetLimit() const { return m_limit; }

private:
	int m_count;
	int m_limit;
};

#endif // !SMNAV_BOT_NAV_MESH_NAVIGATOR_H_
