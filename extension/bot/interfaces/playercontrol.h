#ifndef SMNAV_BOT_BASE_PLAYER_CONTROL_IFACE_H_
#define SMNAV_BOT_BASE_PLAYER_CONTROL_IFACE_H_
#pragma once

#include <bot/interfaces/base_interface.h>
#include <bot/interfaces/playerinput.h>
#include <basehandle.h>

// Interface responsible for controller the bot's input
class IPlayerController : public IBotInterface, public IPlayerInput
{
public:
	IPlayerController(CBaseBot* bot);
	virtual ~IPlayerController();
	
	// Priority for look calls
	enum LookPriority
	{
		LOOK_NONE = 0, // Not looking at anything
		LOOK_AMBIENT, // Looking at the ambient
		LOOK_INTERESTING, // Something interesting
		LOOK_ALERT, // Something that alerts, gunfire, explosions
		LOOK_DANGER, // Something dangerous
		LOOK_OPERATE, // Operating a machine
		LOOK_COMBAT, // Enemies
		LOOK_MOVEMENT, // Movement that requires looking in a specific direction (ie: ladders)
		LOOK_CRITICAL, // Something of very high importance
		LOOK_MANDATORY, // Nothing can interrupt this

		MAX_LOOK_PRIORITY
	};

	// Reset the interface to it's initial state
	virtual void Reset() override;
	// Called at intervals
	virtual void Update() override;
	// Called every server frame
	virtual void Frame() override;

	virtual void ProcessButtons(CBotCmd* cmd) override;

	virtual void RunLook();
	
	virtual void AimAt(const Vector& pos, const LookPriority priority, const float duration);
	virtual void AimAt(edict_t* entity, const LookPriority priority, const float duration);
	virtual void AimAt(const int entity, const LookPriority priority, const float duration);
private:

	LookPriority m_priority; // Current look priority
	CountdownTimer m_looktimer; // Timer for the current look at task
	Vector m_looktarget; // Look at target (Position Vector)
	CBaseHandle m_lookentity; // Look at target (Entity, overrides vector if present)
};

#endif // !SMNAV_BOT_BASE_PLAYER_CONTROL_IFACE_H_

