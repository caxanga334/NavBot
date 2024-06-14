#ifndef SMNAV_BOT_BASE_PLAYER_CONTROL_IFACE_H_
#define SMNAV_BOT_BASE_PLAYER_CONTROL_IFACE_H_
#pragma once

#include <mathlib/vector.h>
#include <basehandle.h>
#include <bot/interfaces/base_interface.h>
#include <bot/interfaces/playerinput.h>

struct edict_t;

// Interface responsible for controller the bot's input
class IPlayerController : public IBotInterface, public IPlayerInput
{
public:
	IPlayerController(CBaseBot* bot);
	virtual ~IPlayerController();
	
	// Priority for look calls
	enum LookPriority
	{
		LOOK_NONE = 0, // Look with the lowest priority
		LOOK_AMBIENT, // Looking at the ambient
		LOOK_INTERESTING, // Something interesting
		LOOK_ALERT, // Something that alerts, gunfire, explosions
		LOOK_DANGER, // Something dangerous
		LOOK_OPERATE, // Operating a machine, buttons, levers, etc
		LOOK_COMBAT, // Enemies
		LOOK_VERY_IMPORTANT, // Something very important, more than enemies
		LOOK_MOVEMENT, // Movement that requires looking in a specific direction (ie: ladders, jumps)
		LOOK_CRITICAL, // Something of very high importance
		LOOK_MANDATORY, // Nothing can interrupt this

		MAX_LOOK_PRIORITY
	};

	virtual void OnDifficultyProfileChanged() override;

	// Reset the interface to it's initial state
	virtual void Reset() override;
	// Called at intervals
	virtual void Update() override;
	// Called every server frame
	virtual void Frame() override;

	virtual void ProcessButtons(int &buttons) override;
	virtual void RunLook();
	virtual void AimAt(const Vector& pos, const LookPriority priority, const float duration, const char* reason = nullptr);
	virtual void AimAt(edict_t* entity, const LookPriority priority, const float duration, const char* reason = nullptr);
	virtual void AimAt(const int entity, const LookPriority priority, const float duration, const char* reason = nullptr);
	// True if the bot aim is Steady
	virtual const bool IsAimSteady() const { return m_isSteady; }
	// True if the bot aim is currently on target
	virtual const bool IsAimOnTarget() const { return m_isOnTarget; }
	// True if the bot aim at some point looked directly on it's target
	virtual const bool DidLookAtTarget() const { return m_didLookAtTarget; }
	// How long the bot aim has been steady
	virtual const float GetSteadyTime() const { return m_steadyTimer.HasStarted() ? m_steadyTimer.GetElapsedTime() : 0.0f; }

private:

	struct AimData
	{
		float speed; // current aim speed
		float maxspeed; // aim speed to reach
		float base; // initial aim speed
		float acceleration; // how fast to reach max speed
	};

	LookPriority m_priority; // Current look priority
	CountdownTimer m_looktimer; // Timer for the current look at task
	Vector m_looktarget; // Look at target (Position Vector)
	CBaseHandle m_lookentity; // Look at target (Entity, overrides vector if present)
	QAngle m_lastangles; // Last bot view angles
	bool m_isSteady; // Is the bot aim steady?
	bool m_isOnTarget; // Is the bot looking at it's look timer
	bool m_didLookAtTarget; // Did the bot look at it's current aim target at some point
	IntervalTimer m_steadyTimer; // Aim stability timer
	AimData m_aimdata;
};

inline const char* GetLookPriorityName(IPlayerController::LookPriority priority)
{
	switch (priority)
	{
	case IPlayerController::LOOK_NONE:
		return "NONE";
	case IPlayerController::LOOK_AMBIENT:
		return "AMBIENT";
	case IPlayerController::LOOK_INTERESTING:
		return "INTERESTING";
	case IPlayerController::LOOK_ALERT:
		return "ALERT";
	case IPlayerController::LOOK_DANGER:
		return "DANGER";
	case IPlayerController::LOOK_OPERATE:
		return "OPERATE";
	case IPlayerController::LOOK_COMBAT:
		return "COMBAT";
	case IPlayerController::LOOK_VERY_IMPORTANT:
		return "VERY IMPORTANT";
	case IPlayerController::LOOK_MOVEMENT:
		return "MOVEMENT";
	case IPlayerController::LOOK_CRITICAL:
		return "CRITICAL";
	case IPlayerController::LOOK_MANDATORY:
		return "MANDATORY";
	case IPlayerController::MAX_LOOK_PRIORITY:
		return "ERROR PRIORITY == MAX_LOOK_PRIORITY";
	default:
		return "ERROR UNKNOWN PRIORITY";
	}
}

#endif // !SMNAV_BOT_BASE_PLAYER_CONTROL_IFACE_H_

