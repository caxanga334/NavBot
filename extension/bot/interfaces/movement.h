#ifndef SMNAV_BOT_MOVEMENT_INTERFACE_H_
#define SMNAV_BOT_MOVEMENT_INTERFACE_H_

#include <bot/interfaces/base_interface.h>
#include "path/path_shareddefs.h"
#include <sdkports/sdk_timers.h>
#include <sdkports/sdk_traces.h>
#include <navmesh/nav_consts.h>
#include <navmesh/nav_elevator.h>

class CNavLadder;
class CNavArea;
class IMovement;
class CMeshNavigator;
class CBasePathSegment;
class NavOffMeshConnection;

class CMovementTraverseFilter : public trace::CTraceFilterSimple
{
public:
	CMovementTraverseFilter(CBaseBot* bot, IMovement* mover, const bool now = true);

	bool ShouldHitEntity(IHandleEntity* pHandleEntity, int contentsMask) override;

	void SetAvoidEntity(CBaseEntity* entity) { m_avoidEnt = entity; }
	bool IsAvoidEntity(CBaseEntity* entity) const { return entity == m_avoidEnt; }
	bool HasAvoidEntity() const { return m_avoidEnt != nullptr; }

private:
	CBaseBot* m_me;
	IMovement* m_mover;
	bool m_now;
	CBaseEntity* m_avoidEnt; // additional entity to avoid (always solid)
};

class CTraceFilterOnlyActors : public trace::CTraceFilterSimple
{
public:
	CTraceFilterOnlyActors(CBaseEntity* pPassEnt, int collisionGroup);

	virtual TraceType_t	GetTraceType() const override
	{
		return TRACE_ENTITIES_ONLY;
	}

	bool ShouldHitEntity(IHandleEntity* pHandleEntity, int contentsMask) override;
};

// Interface responsible for managing the bot's momvement and sending proper inputs to IPlayerController
class IMovement : public IBotInterface
{
public:
	IMovement(CBaseBot* bot);
	~IMovement() override;

	// Movement type requests priorities
	enum MovementRequestPriority
	{
		MOVEREQUEST_PRIO_LOW = 0, // Lowest priority
		MOVEREQUEST_PRIO_MEDIUM, // Medium priority
		MOVEREQUEST_PRIO_HIGH, // High priority
		MOVEREQUEST_PRIO_VERY_HIGH, // Very high priority
		MOVEREQUEST_PRIO_CRITICAL, // Critical priority
		MOVEREQUEST_PRIO_MANDATORY, // Mandatory, highest priority, nothing can prevent this

		MAX_MOVEREQUEST_PRIO_TYPES
	};

	static const char* MovementRequestPriorityTypeToString(MovementRequestPriority type);

	enum MovementType : std::uint8_t
	{
		MOVE_WALKING = 0U,
		MOVE_RUNNING,
		MOVE_SPRINTING,

		MAX_MOVEMENT_TYPES
	};

	static const char* MovementTypeToString(MovementType type);

	struct PlayerHull
	{
		PlayerHull()
		{
			stand_height = 72.0f;
			crouch_height = 36.0f;
			prone_height = 0.0f;
			width = 32.0f;
		}

		float stand_height; // standing player hull height
		float crouch_height; // crouching player hull height
		float prone_height; // proning player hull height
		float width;
	};

	inline static PlayerHull s_playerhull{};

	/**
	 * @brief Invoked during the extension load process.
	 * @param cfgnavbot Pointer to a gameconfig instance of NavBot's gamedata file.
	 * @return Returing false will prevent the extension from loading.
	 */
	static bool InitializeGameData(SourceMod::IGameConfig* cfgnavbot);
	// Returns the current movement type
	MovementType GetMovementType() const { return m_movementType; }
	// Is the bot walking
	const bool IsWalking() const { return GetMovementType() == MovementType::MOVE_WALKING; }
	// Is the bot running
	const bool IsRunning() const { return GetMovementType() == MovementType::MOVE_RUNNING; }
	// Is the bot sprinting
	const bool IsSprinting() const { return GetMovementType() == MovementType::MOVE_SPRINTING; }

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

	// States for the strafe jump FSM
	enum StrafeJumpState : int
	{
		NOT_STRAFE_JUMPING = 0,
		STRAFEJUMP_INIT, // init, on ground
		STRAFEJUMP_TO_MIDPOINT, // moving to mid point
		STRAFEJUMP_TO_ENDPOINT, // moving to end point

		MAX_STRAFEJUMP_STATES
	};

	/* Common Move Weights */
	static constexpr auto MOVEWEIGHT_DEFAULT = 100;
	static constexpr auto MOVEWEIGHT_NAVIGATOR = 1000; // for calls from the navigator
	static constexpr auto MOVEWEIGHT_DODGE = 2000; // trying to dodge something
	static constexpr auto MOVEWEIGHT_STANDARD_JUMPS = 10000; // standard jumps
	static constexpr auto MOVEWEIGHT_COUNTERSTRAFE = 250000; // counterstrafe move calls
	static constexpr auto MOVEWEIGHT_PRIORITY = 500000; // priority move toward calls
	static constexpr auto MOVEWEIGHT_CRITICAL = 900000; // critical move toward calls

	// Reset movement when killed to stop any movement action
	void OnKilled(const CTakeDamageInfo& info) override { this->Reset(); }

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
	// Bot collision hull width
	virtual float GetHullWidth() const;
	// Bot collision hull height
	virtual float GetStandingHullHeight() const;
	virtual float GetCrouchedHullHeight() const;
	virtual float GetProneHullHeight() const;
	// Trace mask for collision detection
	virtual unsigned int GetMovementTraceMask() const;
	// Collision group used for detecting collision within the bot movement code.
	virtual int GetMovementCollisionGroup() const;
	static constexpr float BASE_AVOID_DISTANCE = 50.0f;
	// The base distance the navigator should check for obstacle to avoid.
	virtual float GetAvoidDistance() const { return BASE_AVOID_DISTANCE; }
	// Gets the bot running speed
	virtual float GetRunSpeed() const { return m_maxspeed; }
	// Gets the bot walking speed
	virtual float GetWalkSpeed() const { return m_maxspeed * 0.40f; }
	// Gets the bot sprinting speed
	virtual float GetSprintingSpeed() const { return m_maxspeed; }
	// Gets the bot desired move speed
	virtual float GetDesiredSpeed() const;
	virtual void SetDesiredSpeed(float speed);
	// Returns the bot's maximum movement speed
	float GetMaxSpeed() const { return m_maxspeed; }
	// Sets the bot's maximum movement speed
	void SetMaxSpeed(float spd) { m_maxspeed = spd; }
	/**
	 * @brief Instructs the movement interface to generate the inputs necessary to cause the bot to move towars the given position.
	 * @param pos Position to move towards.
	 * @param weight Move call weight. If another call was made with a higher weight than the current call, then the current is ignored.
	 */
	virtual void MoveTowards(const Vector& pos, const int weight = MOVEWEIGHT_DEFAULT);
	/**
	 * @brief Moves the bot by setting their velocity
	 * @param pos Position to move towards.
	 * @param speed Optional speed, otherwise run speed is used.
	 * @param delta Optional delta for speed.
	 * @param weight Move call weight. If another call was made with a higher weight than the current call, then the current is ignored.
	 */
	virtual void AccelerateTowards(const Vector& pos, const float* speed = nullptr, const float* delta = nullptr, const int weight = MOVEWEIGHT_DEFAULT);
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
	/**
	 * @brief Called by the Navigator to determine if the bot needs to crouch for the path ahead.
	 * @param path Path navigator that called this function.
	 */
	virtual void DetermineIdealPostureForPath(const CMeshNavigator* path);
	// Makes the bot releases all movement keys, keeping momentum
	virtual void Stop();
	// Makes the bot climb the given ladder
	virtual void ClimbLadder(const CNavLadder* ladder, CNavArea* dismount);
	// Makes the bot go down a ladder
	virtual void DescendLadder(const CNavLadder* ladder, CNavArea* dismount);
	// Makes the bot perform a simple jump
	virtual void Jump();
	// Makes the bot perform a crouch jump
	virtual void CrouchJump();
	// Makes the bot perform a double jump
	virtual void DoubleJump();
	virtual void JumpAcrossGap(const Vector& landing, const Vector& forward);
protected:
	// If true, the bot will perform a double jump over a gap. Called by JumpAcrossGap.
	virtual bool GapJumpRequiresDoubleJump(const Vector& landing, const Vector& forward) const { return false; }
public:
	virtual bool ClimbUpToLedge(const Vector& landingGoal, const Vector& landingForward, CBaseEntity* obstacle);
	// Perform a blast jump
	virtual void BlastJumpTo(const Vector& start, const Vector& landingGoal, const Vector& forward) {}
	// Climb by double jumping
	virtual bool DoubleJumpToLedge(const Vector& landingGoal, const Vector& landingForward, edict_t* obstacle);
	// Can the bot climb obstacles
	virtual bool IsAbleToClimb() const { return true; }
	// Can the bot jump across a gap on the ground
	virtual bool IsAbleToJumpAcrossGap() const { return true; }
	virtual bool IsAbleToClimbOntoEntity(edict_t* entity) const;
	// Can the bot perform a double jump
	virtual bool IsAbleToDoubleJump() const { return false; }
	// Can the bot perform a 'blast jump' (Example: TF2's rocket jump)
	virtual bool IsAbleToBlastJump() const { return false; }
	// Can the bot use a grappling hook
	virtual bool IsAbleToUseGrapplingHook() const { return false; }
	// Can the bot perform a strafe jump
	virtual bool IsAbleToStrafeJump() const { return true; }
	// Can the bot use the given off mesh connection?
	virtual bool IsAbleToUseOffMeshConnection(OffMeshConnectionType type, const NavOffMeshConnection* connection) const;
	/**
	 * @brief Checks if the movement interface has taken control of the bot movements to perform a maneuver
	 * @return True if controlling the bot's movements. False otherwise.
	 */
	virtual bool IsControllingMovements() const;
	// Returns true if the bot is allowed to compute paths.
	virtual bool IsPathingAllowed() const;
	/**
	 * @brief This is invoked by the navigator to determine if the current goal path segment should be considered as reached and move onto the next segment in the path.
	 * 
	 * This is only called for off-mesh connections.
	 * @param nav Navigator that called this function.
	 * @param goal Current goal path segment.
	 * @param resultoverride return value override. Set to true to consider as reached, false to not reached.
	 * @return Return true to use the value of resultoverride, otherwise resultoverride is ignored.
	 */
	virtual bool IsPathSegmentReached(const CMeshNavigator* nav, const CBasePathSegment* goal, bool &resultoverride) const { return false; }
	/**
	 * @brief Current movement action needs to control the bot weapons (IE: use the rocket launcher to rocket jump)
	 * 
	 * Returning true will stop bots from firing their weapons at their enemies. False to allow normal behavior.
	 * @return True if the bot should let the movement interface control the weapons. False otherwise.
	 */
	virtual bool NeedsWeaponControl() const { return m_isBreakingObstacle; }
	virtual float GetMinimumMovementSpeed() { return m_maxspeed * 0.4f; }
	virtual bool IsClimbingOrJumping();
	inline virtual bool IsUsingLadder() { return m_ladderState != NOT_USING_LADDER; } // true if the bot is using a ladder
	virtual bool IsAscendingOrDescendingLadder();
	virtual bool IsOnLadder(); // true if the bot is on a ladder right now
	virtual bool IsGap(const Vector& pos, const Vector& forward);
	virtual bool IsPotentiallyTraversable(const Vector& from, const Vector& to, float* fraction, const bool now = true, CBaseEntity** obstacle = nullptr);
	// Checks if there is a possible gap/hole on the ground between 'from' and 'to' vectors
	virtual bool HasPotentialGap(const Vector& from, const Vector& to, float* fraction = nullptr);
	virtual bool IsEntityTraversable(int index, edict_t* edict, CBaseEntity* entity, const bool now = true);
	// returns true if the bot is on ground
	virtual bool IsOnGround();
	// Returns true if the bot is fully crouched
	virtual bool IsCompletelyCrouched() const;
	// Returns true if the bot is currently in a transition of stand/crouch states
	virtual bool IsInCrouchTransition() const;
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
	/**
	 * @brief Instructs the bot to use a catapult.
	 * 
	 * Catapult is any entity that will push the bot towards a specific position.
	 * 
	 * Examples: trigger_push, trigger_catapult
	 * @param start The starting position (where the trigger is).
	 * @param landing The landing position.
	 * @param cheatVelocity If true, manually set the bot velocity.
	 * @return True if the bot is able to perform this movement.
	 */
	virtual bool UseCatapult(const Vector& start, const Vector& landing);
	// returns true if the bot is using a catapult
	bool IsUsingACatapult() const { return m_isUsingCatapult; }
	/**
	 * @brief Makes the bot stop moving and wait a given time.
	 * @param duration Time to wait.
	 */
	void StopAndWait(const float duration) { m_isStopAndWait = true; m_stopAndWaitTimer.Start(duration); }
	// returns true if the bot is stopped (won't move) and waiting
	const bool IsStoppedAndWaiting() const { return m_isStopAndWait; }
	/**
	 * @brief Starts a strafe jump.
	 * @param start Strafe jump start position.
	 * @param end Strafe jump end position.
	 * @return True if it was possible to start a strafe jump
	 */
	virtual bool DoStrafeJump(const Vector& start, const Vector& end);
	// returns true if the bot is performing a strafe jump
	const bool IsStrafeJumping() const { return m_strafeJumpState != StrafeJumpState::NOT_STRAFE_JUMPING; }
	// Returns the calculated 'middle point' for a strafe jump.
	const Vector& GetStrafeJumpMidPoint() const { return m_sjMidPoint; }
	// Returns the strafe jump end point
	const Vector& GetStrafeJumpEndPoint() const { return m_sjEndPoint; }
	// Returns the current strafe jump FSM state
	StrafeJumpState GetStrafeJumpFSMState() const { return m_strafeJumpState; }
	/**
	 * @brief Starts using a grappling hook.
	 * @param start Start position.
	 * @param end End position.
	 * @return True if the movement action started, false if not.
	 */
	virtual bool UseGrapplingHook(const Vector& start, const Vector& end) { return false; }
	/**
	 * @brief Called when the bot is done breaking an obstacle.
	 * @param obstacle Obstacle the bot was trying to break, may be NULL.
	 * @param istimedout If true, the bot stopped breaking due to a timeout.
	 */
	virtual void OnDoneBreakingObstacle(CBaseEntity* obstacle, const bool istimedout);
	/**
	 * @brief Request the movement type to be changed.
	 * @param type Type to change to.
	 * @param priority Request change priority.
	 * @param duration Duration of the request.
	 * @param reason Optional reason for debugging purposes.
	 * @return Returns true if the type was changed, false if the request was denied.
	 */
	virtual bool RequestMovementTypeChange(MovementType type, MovementRequestPriority priority, float duration, const char* reason = nullptr);
protected:
	const CNavLadder* m_ladder; // Ladder the bot is trying to climb
	CNavArea* m_ladderExit; // Nav area after the ladder
	CountdownTimer m_ladderTimer; // Max time to use a ladder
	CountdownTimer m_useLadderTimer; // Timer for pressing the use key to climb a ladder.
	Vector m_landingGoal; // jump landing goal position
	LadderState m_ladderState; // ladder operation state
	CountdownTimer m_ladderWait; // ladder wait timer
	Vector m_ladderMoveGoal; // ladder move to goal vector
	float m_ladderGoalZ; // ladder exit Z coordinate
	bool m_ladderIsAligned;
	CountdownTimer m_jumpCooldown;
	CountdownTimer m_jumpTimer;
	CountdownTimer m_doMidAirCJ; // do a mid air crouch jump (for double jumps)
	Vector m_catapultStartPosition;
	bool m_isJumping;
	bool m_isJumpingAcrossGap;
	bool m_isClimbingObstacle;
	bool m_isAirborne;
	bool m_isBreakingObstacle;
	bool m_crouchToBreak;
	bool m_isUsingCatapult;
	bool m_wasLaunched; // was the bot launched already? (CHEAT)
	CountdownTimer m_catapultCorrectVelocityTimer;
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

	/**
	 * @brief Changes the bot movement type.
	 * @param type Type to change to.
	 */
	void ChangeMovementType(MovementType type)
	{
		if (m_movementType == type) { return; }

		OnMovementTypeChanged(GetMovementType(), type);
		m_movementType = type;
	}

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

	// Bot has completed a jump
	void OnJumpComplete();
	/**
	 * @brief Calculates the strafe jump mid point.
	 * @param start Jump start position.
	 * @param end Jump end position.
	 * @return True if a mid point was found, false otherwise.
	 */
	bool CalculateStrafeJumpMidPoint(const Vector& start, const Vector& end);
	// call to drive the strafe jump FSM. returns false if not strafe jumping
	void StrafeJumpUpdate();
	/**
	 * @brief Invoked when the movement type changes.
	 * @param oldtype Old movement type.
	 * @param newtype New movement type.
	 */
	virtual void OnMovementTypeChanged(MovementType oldtype, MovementType newtype) {}
	/**
	 * @brief Invoked to update the buttons the bot needs to press.
	 */
	virtual void UpdateMovementButtons() {}
	/**
	 * @brief Invoked when teleporting a stuck bot.
	 * @param bot The bot that is stuck.
	 * @param navigator The bot's current navigator.
	 * @param goal Current goal segment.
	 */
	virtual void UnstuckTeleport(CBaseBot* bot, CMeshNavigator* navigator, const CBasePathSegment* goal);
	/**
	 * @brief Invoked when the bot is jumping to assist the bot with the jump.
	 */
	virtual void DoJumpAssist();
private:
	float m_speed; // Bot current speed
	float m_groundspeed; // Bot ground (2D) speed
	Vector m_motionVector; // Unit vector of the bot current movement
	Vector2D m_groundMotionVector; // Unit vector of the bot current ground (2D) movement
	float m_maxspeed; // the bot's maximum speed
	float m_desiredspeed; // speed the bot wants to move at
	bool m_isStopAndWait;
	bool m_doJumpAssist;
	MovementType m_movementType;
	MovementRequestPriority m_lastMTRequestPriority; // Last movement type request priority
	CountdownTimer m_MTRequestTimer; // movement type request timer
	CountdownTimer m_stopAndWaitTimer;
	StrafeJumpState m_strafeJumpState; // strafe jump FSM current state
	Vector m_sjMidPoint; // strafe jump mid point
	Vector m_sjEndPoint; // strafe jump end point
	bool m_sjIsToTheLeft;

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

	StrafeJumpState StrafeJump_Init();
	StrafeJumpState StrafeJump_ToMidPoint();
	StrafeJumpState StrafeJump_ToEndPoint();

	static constexpr float ELEV_MOVE_RANGE = 32.0f;
	static constexpr float ELEV_SPEED_DIV = 400.0f; // timeout is this value divided by speed
	static constexpr float ELEV_MOVESPEED_SCALE = 0.7f; // move at 70% of run speed

	void _Reset();
};

inline bool IMovement::IsControllingMovements() const
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
	else if (m_counterStrafeTimer.HasStarted() && !m_counterStrafeTimer.IsElapsed())
	{
		return true; // counter strafing
	}
	else if (m_isUsingCatapult) // using catapult, block standard pathing
	{
		return true;
	}
	else if (IsStrafeJumping())
	{
		return true;
	}

	return false;
}

inline bool IMovement::IsPathingAllowed() const
{
	// Don't allow navigators to compute a path in these states
	if (m_ladderState != NOT_USING_LADDER || m_elevatorState != NOT_USING_ELEVATOR || m_isUsingCatapult)
	{
		return false;
	}

	return true;
}

inline bool IMovement::IsClimbingOrJumping()
{
	if (!m_isJumping)
	{
		return false;
	}

	if (m_jumpTimer.IsElapsed() && IsOnGround())
	{
		m_isJumping = false;
		return false;
	}

	return true;
}

#endif // !SMNAV_BOT_MOVEMENT_INTERFACE_H_

