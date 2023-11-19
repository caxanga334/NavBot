#ifndef SMNAV_BOT_MOVEMENT_INTERFACE_H_
#define SMNAV_BOT_MOVEMENT_INTERFACE_H_

#include <bot/interfaces/base_interface.h>
#include <sdkports/sdk_timers.h>

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

protected:
	CountdownTimer m_jumptimer; // Jump timer
	CNavLadder* m_ladder; // Ladder the bot is trying to climb
	State m_state;

private:
	int m_internal_jumptimer; // Tick based jump timer
};

#endif // !SMNAV_BOT_MOVEMENT_INTERFACE_H_

