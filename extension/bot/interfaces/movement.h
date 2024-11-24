#ifndef SMNAV_BOT_MOVEMENT_INTERFACE_H_
#define SMNAV_BOT_MOVEMENT_INTERFACE_H_

#include <memory>

#include <bot/interfaces/base_interface.h>
#include <sdkports/sdk_timers.h>
#include <sdkports/sdk_traces.h>

class CNavLadder;
class CNavArea;
class IMovement;

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
		bool isstuck; // is the bot stuck
		IntervalTimer stucktimer; // how long the bot has been stuck
		CountdownTimer stuckrechecktimer; // for delays between stuck events
		Vector stuckpos; // position where the bot got stuck
		IntervalTimer movementrequesttimer; // Time since last request to move the bot

		inline void ClearStuck(const Vector& pos)
		{
			this->isstuck = false;
			this->stucktimer.Invalidate();
			this->stuckrechecktimer.Invalidate();
			this->movementrequesttimer.Invalidate();
			this->stuckpos = pos;
		}

		inline void Stuck(CBaseBot* bot)
		{
			this->isstuck = true;
			this->stucktimer.Start();
			this->stuckrechecktimer.Start(1.0f);
		}

		inline void UpdateNotStuck(const Vector& pos)
		{
			this->isstuck = false;
			this->stucktimer.Start();
			this->stuckrechecktimer.Start(1.0f);
			this->stuckpos = pos;
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
		}

		inline void OnMovementRequested()
		{
			this->movementrequesttimer.Start();
		}
	};

private:

	/**
	 * @brief Interface for movement actions. A movement action represents a single action done by the bot. Examples: crouch jumps, rocket jumps, double jumps, etc...
	 */
	class IAction
	{
	public:

		virtual ~IAction() {}

		// Called once when the action is about to be executed. This is called on the same frame as the action is created
		virtual void OnStart() {}

		// Called every frame while this action is running. Note: there is a 1 frame delay from OnStart
		virtual void Execute() = 0;

		// Called once when the action is about to be discarded.
		virtual void OnEnd() {}

		/**
		 * @brief Does this action takes full control of the bot movements? If yes, blocks the navigation from sending movement inputs.
		 * @return True if this movement action should block inputs from the navigator. False otherwise.
		 */
		virtual bool BlockNavigator() { return false; }

		/**
		 * @brief Does this action counts as the bot climbing over an obstacle or jumping?
		 * @return True if this is a climb or jump. False otherwise.
		 */
		virtual bool IsClimbingOrJumping() { return false; }

		// Override me! This tells the movement interface that the action is done and can be discarded.
		virtual bool IsDone() = 0;
	};

public:

	/**
	 * @brief Movement actions should derive from this.
	 * @tparam T Bot class.
	 */
	template<typename T>
	class Action : public IAction
	{
	public:
		Action(T* bot)
		{
			m_me = bot;
		}

		~Action() override {}

		inline T* GetBot() const { return m_me; }

	private:
		T* m_me;
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

	// Makes the bot walk/run towards the given position
	virtual void MoveTowards(const Vector& pos);
	/**
	 * @brief Makes the bot look at the given position (used for movement)
	 * @param pos Position the bot will look at
	 * @param important if true, send a high priority look request
	*/
	virtual void FaceTowards(const Vector& pos, const bool important = false);
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
	virtual bool DoubleJumpToLedge(const Vector& landingGoal, const Vector& landingForward, edict_t* obstacle) { return false; }

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
	virtual bool IsControllingMovements()
	{
		if (m_action)
		{
			return m_action->BlockNavigator();
		}

		return false;
	}

	// The speed the bot will move at (capped by the game player movements)
	virtual float GetMovementSpeed() { return m_basemovespeed; }
	virtual float GetMinimumMovementSpeed() { return m_basemovespeed * 0.4f; }
	virtual bool IsJumping() { return !m_jumptimer.IsElapsed(); }

	virtual bool IsClimbingOrJumping()
	{
		if (m_action)
		{
			return m_action->IsClimbingOrJumping();
		}

		return false;
	}

	inline virtual bool IsUsingLadder() { return m_ladderState != NOT_USING_LADDER; } // true if the bot is using a ladder
	virtual bool IsAscendingOrDescendingLadder();
	virtual bool IsOnLadder(); // true if the bot is on a ladder right now
	virtual bool IsGap(const Vector& pos, const Vector& forward);
	virtual bool IsPotentiallyTraversable(const Vector& from, const Vector& to, float* fraction, const bool now = true, CBaseEntity** obstacle = nullptr);
	// Checks if there is a possible gap/hole on the ground between 'from' and 'to' vectors
	virtual bool HasPotentialGap(const Vector& from, const Vector& to, float& fraction);

	virtual bool IsEntityTraversable(edict_t* entity, const bool now = true);

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

	/**
	 * @brief Executes a movement action.
	 * @tparam T Action to execute.
	 * @tparam ...Types Arguments passed to the action.
	 * @param force if true, will override any existing action.
	 * @param ...args Arguments passed to the action
	 * @return True if the action was created, false if not.
	 */
	template <typename T, class... Types>
	bool ExecuteMovementAction(bool force, Types... args)
	{
		// Don't override any existing actions unless force is true
		if (force == false && m_action)
		{
			return false;
		}

		std::unique_ptr<T> action = std::make_unique<T>(std::forward<Types>(args)...);

		m_action = std::move(action);

		m_action->OnStart();

		return true;
	}

protected:
	CountdownTimer m_jumptimer; // Jump timer
	CountdownTimer m_braketimer; // Timer for forced braking
	const CNavLadder* m_ladder; // Ladder the bot is trying to climb
	CNavArea* m_ladderExit; // Nav area after the ladder
	CountdownTimer m_ladderTimer; // Max time to use a ladder
	LadderState m_ladderState; // ladder operation state

	// stuck monitoring
	StuckStatus m_stuck;

	virtual void StuckMonitor();

	virtual bool TraverseLadder();

	const std::unique_ptr<IAction>& GetRunningAction() const { return m_action; }

private:
	float m_speed; // Bot current speed
	float m_groundspeed; // Bot ground (2D) speed
	Vector m_motionVector; // Unit vector of the bot current movement
	Vector2D m_groundMotionVector; // Unit vector of the bot current ground (2D) movement
	float m_basemovespeed;
	std::unique_ptr<IAction> m_action; // Movement action being performed

	LadderState ApproachUpLadder();
	LadderState ApproachDownLadder();
	LadderState UseLadderUp();
	LadderState UseLadderDown();
	LadderState DismountLadderTop();
	LadderState DismountLadderBottom();
};

#endif // !SMNAV_BOT_MOVEMENT_INTERFACE_H_

