#ifndef SMNAV_BOT_MOVEMENT_INTERFACE_H_
#define SMNAV_BOT_MOVEMENT_INTERFACE_H_

#include <bot/interfaces/base_interface.h>
#include <sdkports/sdk_timers.h>
#include <sdkports/sdk_traces.h>
#include <navmesh/nav_elevator.h>

class CNavLadder;
class CNavArea;
class IMovement;
class CMeshNavigator;

class CMovementTraverseFilter : public trace::CTraceFilterSimple
{
public:
	CMovementTraverseFilter(CBaseBot* bot, IMovement* mover, const bool now = true);

	bool ShouldHitEntity(int entity, CBaseEntity* pEntity, edict_t* pEdict, const int contentsMask) override;

private:
	CBaseBot* m_me;
	IMovement* m_mover;
	bool m_now;
};

class CTraceFilterOnlyActors : public trace::CTraceFilterSimple
{
public:
	CTraceFilterOnlyActors(CBaseEntity* pPassEnt, int collisionGroup);

	virtual TraceType_t	GetTraceType() const override
	{
		return TRACE_ENTITIES_ONLY;
	}

	bool ShouldHitEntity(int entity, CBaseEntity* pEntity, edict_t* pEdict, const int contentsMask) override;
};

// Interface responsible for managing the bot's momvement and sending proper inputs to IPlayerController
class IMovement : public IBotInterface
{
public:
	IMovement(CBaseBot* bot);
	~IMovement() override;

	class StuckStatus
	{
	public:
		StuckStatus() :
			stuckpos(0.0f, 0.0f, 0.0f)
		{
			isstuck = false;
			counter = 0;
		}

		bool isstuck; // is the bot stuck
		IntervalTimer stucktimer; // how long the bot has been stuck
		CountdownTimer stuckrechecktimer; // for delays between stuck events
		Vector stuckpos; // position where the bot got stuck
		IntervalTimer movementrequesttimer; // Time since last request to move the bot
		int counter; // how many stuck checks returned as stuck

		inline void ClearStuck(const Vector& pos)
		{
			this->isstuck = false;
			this->stucktimer.Invalidate();
			this->stuckrechecktimer.Invalidate();
			this->movementrequesttimer.Invalidate();
			this->stuckpos = pos;
			this->counter = 0;
		}

		inline void Stuck(CBaseBot* bot)
		{
			this->isstuck = true;
			this->stucktimer.Start();
			this->stuckrechecktimer.Start(1.0f);
			this->counter++;
		}

		inline void UpdateNotStuck(const Vector& pos)
		{
			this->isstuck = false;
			this->stucktimer.Start();
			this->stuckrechecktimer.Start(1.0f);
			this->stuckpos = pos;
			this->counter = 0;
		}

		inline bool IsIdle() const
		{
			constexpr auto IDLE_TIME = 0.25f;
			return this->movementrequesttimer.IsGreaterThen(IDLE_TIME);
		}

		inline const Vector& GetStuckPos() const
		{
			return this->stuckpos;
		}

		inline void Reset()
		{
			this->isstuck = false;
			this->stucktimer.Invalidate();
			this->stuckrechecktimer.Invalidate();
			this->movementrequesttimer.Invalidate();
			this->stuckpos = vec3_origin;
			this->counter = 0;
		}

		inline void OnMovementRequested()
		{
			this->movementrequesttimer.Start();
		}
	};
	
	// Ladder usage state
	enum LadderState
	{
		NOT_USING_LADDER = 0, // not using a ladder
		APPROACHING_LADDER_UP, // moving towards a ladder the bot needs to go up
		EXITING_LADDER_UP, // exiting/dismouting a ladder the bot needs to go up
		USING_LADDER_UP, // moving up on a ladder
		APPROACHING_LADDER_DOWN, // moving towards a ladder the bot needs to go down
		EXITING_LADDER_DOWN, // exiting/dismouting a ladder the bot needs to go down
		USING_LADDER_DOWN, // moving down on a ladder

		MAX_LADDER_STATES
	};

	// States for the use elevator finite state machine
	enum ElevatorState : int
	{
		NOT_USING_ELEVATOR = 0, // not using an elevator
		MOVE_TO_WAIT_POS, // moving to wait position
		CALL_ELEVATOR, // call the elevator if needed
		WAIT_FOR_ELEVATOR, // waiting for the elevator to arrive on this floor
		ENTER_ELEVATOR, // enter the elevator
		OPERATE_ELEVATOR, // operate the elevator
		RIDE_ELEVATOR, // wait for the elevator to move
		EXIT_ELEVATOR, // move out of the elevator

		MAX_ELEVATOR_STATES,
	};

	/* Common Move Weights */
	static constexpr auto MOVEWEIGHT_DEFAULT = 100;
	static constexpr auto MOVEWEIGHT_NAVIGATOR = 1000; // for calls from the navigator
	static constexpr auto MOVEWEIGHT_DODGE = 2000; // trying to dodge something
	static constexpr auto MOVEWEIGHT_COUNTERSTRAFE = 250000; // counterstrafe move calls
	static constexpr auto MOVEWEIGHT_PRIORITY = 500000; // priority move toward calls
	static constexpr auto MOVEWEIGHT_CRITICAL = 900000; // critical move toward calls

	// Reset the interface to it's initial state
	void Reset() override;
	// Called at intervals
	void Update() override;
	// Called every server frame
	void Frame() override;

	// Max height difference that a player is able to climb without jumping
	virtual float GetStepHeight() const { return 18.0f; }
	// Max height a player can climb by jumping
	virtual float GetMaxJumpHeight() const { return 57.0f; }
	// Max health a player can blimb by double jumping
	virtual float GetMaxDoubleJumpHeight() const { return 0.0f; }
	// Max distance between a (horizontal) gap that the bot is able to jump
	virtual float GetMaxGapJumpDistance() const { return 200.0f; } // 200 is a generic safe value that should be compatible with most mods
	// Max height a player can safely fall
	virtual float GetMaxDropHeight() const { return 450.0f; }

	inline virtual float GetTraversableSlopeLimit() const { return 0.6f; }
	// (cheat) Set the bot's Z velocity to this value when crouch jumping
	inline virtual float GetCrouchJumpZBoost() const { return 282.0f; } // max vel should be around 277 (tf2), we added some extra to help bots

	// Bot collision hull width
	virtual float GetHullWidth();
	// Bot collision hull heigh
	virtual float GetStandingHullHeigh();
	virtual float GetCrouchedHullHeigh();
	virtual float GetProneHullHeigh();
	// Trace mask for collision detection
	virtual unsigned int GetMovementTraceMask();
	// Gets the bot running speed
	virtual float GetRunSpeed() const { return m_maxspeed; }
	// Gets the bot walking speed
	virtual float GetWalkSpeed() const { return m_maxspeed * 0.30f; }
	// Gets the bot desired move speed
	virtual float GetDesiredSpeed() const;
	virtual void SetDesiredSpeed(float speed);
	/**
	 * @brief Instructs the movement interface to generate the inputs necessary to cause the bot to move towars the given position.
	 * @param pos Position to move towards.
	 * @param weight Move call weight. If another call was made with a higher weight than the current call, then the current is ignored.
	 */
	virtual void MoveTowards(const Vector& pos, const int weight = MOVEWEIGHT_DEFAULT);
	/**
	 * @brief Makes the bot look at the given position (used for movement)
	 * @param pos Position the bot will look at
	 * @param important if true, send a high priority look request
	*/
	virtual void FaceTowards(const Vector& pos, const bool important = false);
	/**
	 * @brief Called by the Navigator to adjust the bot's speed for the current path.
	 * @param path Path navigator that called this.
	 */
	virtual void AdjustSpeedForPath(CMeshNavigator* path);
	// Makes the bot releases all movement keys, keeping momentum
	virtual void Stop();
	// Makes the bot loses it's momentum. Use if you need the bot to stop immediately
	virtual void Brake(const float brakeTime = 0.1f) { m_braketimer.Start(brakeTime); }
	// Makes the bot climb the given ladder
	virtual void ClimbLadder(const CNavLadder* ladder, CNavArea* dismount);
	// Makes the bot go down a ladder
	virtual void DescendLadder(const CNavLadder* ladder, CNavArea* dismount);
	// Makes the bot perform a simple jump
	virtual void Jump();
	// Makes the bot perform a crouch jump
	virtual void CrouchJump();
	// Makes the bot perform a double jump
	virtual void DoubleJump() {}
	virtual void JumpAcrossGap(const Vector& landing, const Vector& forward);
	virtual bool ClimbUpToLedge(const Vector& landingGoal, const Vector& landingForward, edict_t* obstacle);
	// Perform a blast jump
	virtual void BlastJumpTo(const Vector& start, const Vector& landingGoal, const Vector& forward) {}
	// Climb by double jumping
	virtual bool DoubleJumpToLedge(const Vector& landingGoal, const Vector& landingForward, edict_t* obstacle);

	virtual bool IsAbleToClimb() { return true; }
	virtual bool IsAbleToJumpAcrossGap() { return true; }
	virtual bool IsAbleToClimbOntoEntity(edict_t* entity);
	virtual bool IsAbleToDoubleJump() { return false; }
	// Can the bot perform a 'blast jump' (Example: TF2's rocket jump)
	virtual bool IsAbleToBlastJump() { return false; }
	/**
	 * @brief Checks if the movement interface has taken control of the bot movements to perform a maneuver
	 * @return True if controlling the bot's movements. False otherwise.
	 */
	virtual bool IsControllingMovements();
	/**
	 * @brief Current movement action needs to control the bot weapons (IE: use the rocket launcher to rocket jump)
	 * 
	 * Returning true will stop bots from firing their weapons at their enemies. False to allow normal behavior.
	 * @return True if the bot should let the movement interface control the weapons. False otherwise.
	 */
	virtual bool NeedsWeaponControl() { return m_isBreakingObstacle; }
	virtual float GetMinimumMovementSpeed() { return m_maxspeed * 0.4f; }
	virtual bool IsJumping() { return !m_jumptimer.IsElapsed(); }
	virtual bool IsClimbingOrJumping();
	inline virtual bool IsUsingLadder() { return m_ladderState != NOT_USING_LADDER; } // true if the bot is using a ladder
	virtual bool IsAscendingOrDescendingLadder();
	virtual bool IsOnLadder(); // true if the bot is on a ladder right now
	virtual bool IsGap(const Vector& pos, const Vector& forward);
	virtual bool IsPotentiallyTraversable(const Vector& from, const Vector& to, float* fraction, const bool now = true, CBaseEntity** obstacle = nullptr);
	// Checks if there is a possible gap/hole on the ground between 'from' and 'to' vectors
	virtual bool HasPotentialGap(const Vector& from, const Vector& to, float& fraction);

	virtual bool IsEntityTraversable(int index, edict_t* edict, CBaseEntity* entity, const bool now = true);

	virtual bool IsOnGround();

	virtual bool IsAttemptingToMove(const float time = 0.25f) const;

	virtual bool IsStuck() { return m_stuck.isstuck; }
	virtual float GetStuckDuration();
	virtual void ClearStuckStatus(const char* reason = nullptr);

	virtual float GetSpeed() const { return m_speed; }
	virtual float GetGroundSpeed() const { return m_groundspeed; }
	virtual const Vector& GetMotionVector() { return m_motionVector; }
	virtual const Vector2D& GetGroundMotionVector() { return m_groundMotionVector; }

	virtual bool IsAreaTraversable(const CNavArea* area) const;
	virtual bool NavigatorAllowSkip(const CNavArea* area) const { return true; }
	// Called after calculating the crossing point between two nav areas
	virtual void AdjustPathCrossingPoint(const CNavArea* fromArea, const CNavArea* toArea, const Vector& fromPos, Vector* crosspoint);
	// Called when the bot is determined to be stuck, try to unstuck it (IE: jumping)
	virtual void TryToUnstuck();
	// Called when there is an obstacle on the bot's path.
	virtual void ObstacleOnPath(CBaseEntity* obstacle, const Vector& goalPos, const Vector& forward, const Vector& left);
	// Returns the Nav Ladder the bot is using if one.
	inline const CNavLadder* GetNavLadder() const { return m_ladder; }	
	inline int GetStuckCount() const { return m_stuck.counter; }
	virtual bool IsUsingElevator() const;
	/**
	 * @brief Makes the bot uses the given elevator.
	 * @param elevator Elevator the bot will use.
	 * @param from Nav area to enter the elevator.
	 * @param to Nav area of the destination floor.
	 */
	virtual void UseElevator(const CNavElevator* elevator, const CNavArea* from, const CNavArea* to);
	void ClearMoveWeight() { m_lastMoveWeight = 0; }
	int GetLastMoveWeight() const { return m_lastMoveWeight; }
	virtual bool IsBreakingObstacle() const { return m_isBreakingObstacle; }
	virtual bool BreakObstacle(CBaseEntity* obstacle);
	/**
	 * @brief Called by the navigation when scanning for obstacle that can be dealt with by pressing the USE key.
	 * 
	 * Example: Closed doors that can be open with the USE key.
	 * @param entity Entity blocking the bot's path.
	 * @return true if this entity is useable.
	 */
	virtual bool IsUseableObstacle(CBaseEntity* entity);
	/**
	 * @brief Tell the movement interface to perform a counter-strafe. (Move in the opposite direction of motion).
	 * @param time How long to keep counter-strafing. Disabled automatically once the bot reaches near zero speed.
	 */
	void DoCounterStrafe(const float time = 0.1f) { m_counterStrafeTimer.Start(time); }
	// Is the bot counter-strafing?
	bool IsCounterStrafing() const { return m_counterStrafeTimer.HasStarted() && !m_counterStrafeTimer.IsElapsed(); }
protected:
	CountdownTimer m_jumptimer; // Jump timer
	CountdownTimer m_braketimer; // Timer for forced braking
	const CNavLadder* m_ladder; // Ladder the bot is trying to climb
	CNavArea* m_ladderExit; // Nav area after the ladder
	CountdownTimer m_ladderTimer; // Max time to use a ladder
	CountdownTimer m_useLadderTimer; // Timer for pressing the use key to climb a ladder.
	Vector m_landingGoal; // jump landing goal position
	LadderState m_ladderState; // ladder operation state
	CountdownTimer m_ladderWait; // ladder wait timer
	Vector m_ladderMoveGoal; // ladder move to goal vector
	float m_ladderGoalZ; // ladder exit Z coordinate
	bool m_isJumpingAcrossGap;
	bool m_isClimbingObstacle;
	bool m_isAirborne;
	bool m_isBreakingObstacle;
	const CNavElevator* m_elevator;
	const CNavElevator::ElevatorFloor* m_fromFloor;
	const CNavElevator::ElevatorFloor* m_toFloor;
	ElevatorState m_elevatorState;
	CountdownTimer m_elevatorTimeout;
	int m_lastMoveWeight;
	CHandle<CBaseEntity> m_obstacleEntity;
	CountdownTimer m_obstacleBreakTimeout;
	CountdownTimer m_counterStrafeTimer;

	// stuck monitoring
	StuckStatus m_stuck;

	virtual void StuckMonitor();
	virtual void TraverseLadder();
	virtual void ElevatorUpdate();
	virtual void ObstacleBreakUpdate();

	void CleanUpElevator()
	{
		m_elevator = nullptr;
		m_fromFloor = nullptr;
		m_toFloor = nullptr;
		m_elevatorTimeout.Invalidate();
		SetDesiredSpeed(GetRunSpeed());
	}
private:
	float m_speed; // Bot current speed
	float m_groundspeed; // Bot ground (2D) speed
	Vector m_motionVector; // Unit vector of the bot current movement
	Vector2D m_groundMotionVector; // Unit vector of the bot current ground (2D) movement
	CountdownTimer m_jump_zboost_timer; // Timer for the z boost when jumping
	float m_maxspeed; // the bot's maximum speed
	float m_desiredspeed; // speed the bot wants to move at

	LadderState ApproachUpLadder();
	LadderState ApproachDownLadder();
	LadderState UseLadderUp();
	LadderState UseLadderDown();
	LadderState DismountLadderTop();
	LadderState DismountLadderBottom();

	void ChangeLadderState(LadderState newState)
	{
		OnLadderStateChanged(m_ladderState, newState);
		m_ladderState = newState;
	}

	void OnLadderStateChanged(LadderState oldState, LadderState newState);

	static constexpr float MIN_LADDER_SPEED = 25.0f;

	/* Elevator FSM */

	ElevatorState EState_MoveToWaitPosition();
	ElevatorState EState_CallElevator();
	ElevatorState EState_WaitForElevator();
	ElevatorState EState_EnterElevator();
	ElevatorState EState_OperateElevator();
	ElevatorState EState_RideElevator();
	ElevatorState EState_ExitElevator();

	static constexpr float ELEV_MOVE_RANGE = 32.0f;
	static constexpr float ELEV_SPEED_DIV = 400.0f; // timeout is this value divided by speed
	static constexpr float ELEV_MOVESPEED_SCALE = 0.7f; // move at 70% of run speed
};

inline bool IMovement::IsControllingMovements()
{
	if (m_ladderState != NOT_USING_LADDER)
	{
		return true; // take full control when doing ladder operations
	}
	else if (m_elevatorState != NOT_USING_ELEVATOR)
	{
		return true; // take full control when doing elevator operations
	}
	else if (m_isBreakingObstacle)
	{
		return true; // take full control when breaking an obstacle on my path
	}

	return false;
}

inline bool IMovement::IsClimbingOrJumping()
{
	if (!m_jumptimer.IsElapsed())
	{
		return true;
	}

	if (m_isJumpingAcrossGap || m_isClimbingObstacle)
	{
		return true;
	}

	return false;
}

#endif // !SMNAV_BOT_MOVEMENT_INTERFACE_H_

