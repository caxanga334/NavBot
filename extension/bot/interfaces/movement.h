#ifndef SMNAV_BOT_MOVEMENT_INTERFACE_H_
#define SMNAV_BOT_MOVEMENT_INTERFACE_H_

#include <bot/interfaces/base_interface.h>
#include <sdkports/sdk_timers.h>

class CNavLadder;

// Interface responsible for managing the bot's momvement and sending proper inputs to IPlayerController
class IMovement : public IBotInterface
{
public:
	IMovement(CBaseBot* bot);
	virtual ~IMovement();

	
	enum State // Internal state of the movement interface
	{
		STATE_NONE = 0, // No special movement states
		STATE_CLIMBING, // Climbing something
		STATE_JUMPING_OVER_GAP, // Jumping over a gap
		// TO-DO: ladder states

		MAX_INTERNAL_STATES
	};

	// Reset the interface to it's initial state
	virtual void Reset() override;
	// Called at intervals
	virtual void Update() override;
	// Called every server frame
	virtual void Frame() override;

	// Max height difference that a player is able to climb without jumping
	virtual float GetStepHeight() { return 18.0f; }
	// Max height a player can climb by jumping
	virtual float GetMaxJumpHeight() { return 57.0f; }
	// Max distance between a (horizontal) gap that the bot is able to jump
	virtual float GetMaxGapJumpDistance() { return 200.0f; } // 200 is a generic safe value that should be compatible with most mods

	inline virtual float GetTraversableSlopeLimit() { return 0.6f; }

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
	// Makes the bot climb the given ladder
	virtual void ClimbLadder(CNavLadder* ladder);
	// Makes the bot perform a simple jump
	virtual void Jump();

	// TO-DO: Climb over obstacles, jump over gap

	// The speed the bot will move at (capped by the game player movements)
	virtual float GetMovementSpeed() { return 500.0f; }
	virtual bool IsJumping() { return !m_jumptimer.IsElapsed(); }
	virtual bool IsOnLadder();
	virtual bool IsGap(const Vector& pos, const Vector& forward);
	virtual bool IsPotentiallyTraversable(const Vector& from, const Vector& to, float &fraction, const bool now = true);
	// Checks if there is a possible gap/hole on the ground between 'from' and 'to' vectors
	virtual bool HasPotentialGap(const Vector& from, const Vector& to, float& fraction);

	virtual bool IsEntityTraversable(edict_t* entity, const bool now = true);

protected:
	CountdownTimer m_jumptimer; // Jump timer
	CNavLadder* m_ladder; // Ladder the bot is trying to climb
	State m_state;

private:
	int m_internal_jumptimer; // Tick based jump timer
};

#endif // !SMNAV_BOT_MOVEMENT_INTERFACE_H_

