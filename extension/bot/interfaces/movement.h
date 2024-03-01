#ifndef SMNAV_BOT_MOVEMENT_INTERFACE_H_
#define SMNAV_BOT_MOVEMENT_INTERFACE_H_

#include <bot/interfaces/base_interface.h>
#include <sdkports/sdk_timers.h>
#include <util/UtilTrace.h>

class CNavLadder;
class CNavArea;
class IMovement;

class CMovementTraverseFilter : public CTraceFilterSimple
{
public:
	CMovementTraverseFilter(CBaseBot* bot, IMovement* mover, const bool now = true);

	virtual bool ShouldHitEntity(IHandleEntity* pHandleEntity, int contentsMask) override;

private:
	CBaseBot* m_me;
	IMovement* m_mover;
	bool m_now;
};

class CTraceFilterOnlyActors : public CTraceFilterSimple
{
public:
	CTraceFilterOnlyActors(const IHandleEntity* handleentity);

	virtual TraceType_t	GetTraceType() const override
	{
		return TRACE_ENTITIES_ONLY;
	}

	virtual bool ShouldHitEntity(IHandleEntity* pHandleEntity, int contentsMask) override;
};

// Interface responsible for managing the bot's momvement and sending proper inputs to IPlayerController
class IMovement : public IBotInterface
{
public:
	IMovement(CBaseBot* bot);
	virtual ~IMovement();

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
	virtual void Reset() override;
	// Called at intervals
	virtual void Update() override;
	// Called every server frame
	virtual void Frame() override;

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
	virtual void DoubleJump() {}
	virtual void JumpAcrossGap(const Vector& landing, const Vector& forward);
	virtual bool ClimbUpToLedge(const Vector& landingGoal, const Vector& landingForward, edict_t* obstacle);
	// Perform a blast jump
	virtual void BlastJumpTo(const Vector& start, const Vector& landingGoal, const Vector& forward) {}

	virtual bool IsAbleToClimb() { return true; }
	virtual bool IsAbleToJumpAcrossGap() { return true; }
	virtual bool IsAbleToClimbOntoEntity(edict_t* entity);
	virtual bool IsAbleToDoubleJump() { return false; }
	// Can the bot perform a 'blast jump' (Example: TF2's rocket jump)
	virtual bool IsAbleToBlastJump() { return false; }
	// Return true if the movement interface has taken control of the bot to perform an advanced movement.
	// This is used by the navigator to know if it should wait before continuing following the path.
	virtual bool IsOnAutoPilot() { return false; }

	// The speed the bot will move at (capped by the game player movements)
	virtual float GetMovementSpeed() { return m_basemovespeed; }
	virtual float GetMinimumMovementSpeed() { return m_basemovespeed * 0.4f; }
	virtual bool IsJumping() { return !m_jumptimer.IsElapsed(); }
	virtual bool IsClimbingOrJumping();
	inline virtual bool IsUsingLadder() { return m_ladderState != NOT_USING_LADDER; } // true if the bot is using a ladder
	virtual bool IsAscendingOrDescendingLadder();
	virtual bool IsOnLadder(); // true if the bot is on a ladder right now
	virtual bool IsGap(const Vector& pos, const Vector& forward);
	virtual bool IsPotentiallyTraversable(const Vector& from, const Vector& to, float &fraction, const bool now = true);
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

protected:
	CountdownTimer m_jumptimer; // Jump timer
	const CNavLadder* m_ladder; // Ladder the bot is trying to climb
	CNavArea* m_ladderExit; // Nav area after the ladder
	CountdownTimer m_ladderTimer; // Max time to use a ladder
	Vector m_landingGoal; // jump landing goal position
	LadderState m_ladderState; // ladder operation state
	bool m_isJumpingAcrossGap;
	bool m_isClimbingObstacle;
	bool m_isAirborne;

	// stuck monitoring
	StuckStatus m_stuck;

	virtual void StuckMonitor();

	virtual bool TraverseLadder();

private:
	float m_speed; // Bot current speed
	float m_groundspeed; // Bot ground (2D) speed
	Vector m_motionVector; // Unit vector of the bot current movement
	Vector2D m_groundMotionVector; // Unit vector of the bot current ground (2D) movement
	int m_internal_jumptimer; // Tick based jump timer
	float m_basemovespeed;

	LadderState ApproachUpLadder();
	LadderState ApproachDownLadder();
	LadderState UseLadderUp();
	LadderState UseLadderDown();
	LadderState DismountLadderTop();
	LadderState DismountLadderBottom();
};

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

