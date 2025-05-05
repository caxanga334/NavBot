#ifndef SMNAV_BOT_BASE_PLAYER_CONTROL_IFACE_H_
#define SMNAV_BOT_BASE_PLAYER_CONTROL_IFACE_H_
#pragma once

#include <string_view>
#include <string>
#include <array>

#include <sdkports/sdk_ehandle.h>
#include <bot/interfaces/base_interface.h>
#include <bot/interfaces/playerinput.h>
#include <bot/interfaces/decisionquery.h>

struct edict_t;

// Interface responsible for controller the bot's input
class IPlayerController : public IBotInterface, public IPlayerInput
{
public:
	IPlayerController(CBaseBot* bot);
	~IPlayerController() override;
	
	// Priority for look calls
	enum LookPriority
	{
		LOOK_NONE = 0, // Look with the lowest priority
		LOOK_AMBIENT, // Looking at the ambient
		LOOK_INTERESTING, // Something interesting
		LOOK_ALERT, // Something that alerts, gunfire, explosions
		LOOK_DANGER, // Something dangerous
		LOOK_ALLY, // Look at my teammate
		LOOK_OPERATE, // Operating a machine, buttons, levers, etc
		LOOK_COMBAT, // Enemies
		LOOK_SUPPORT, // Combat support, more important than enemies
		LOOK_VERY_IMPORTANT, // Something very important, more than enemies
		LOOK_MOVEMENT, // Movement that requires looking in a specific direction (ie: ladders, jumps)
		LOOK_CRITICAL, // Something of very high importance
		LOOK_MANDATORY, // Nothing can interrupt this

		MAX_LOOK_PRIORITY
	};

	void OnDifficultyProfileChanged() override;

	// Reset the interface to it's initial state
	void Reset() override;
	// Called at intervals
	void Update() override;
	// Called every server frame
	void Frame() override;

	void ProcessButtons(int &buttons) override;
	
	/**
	 * @brief Instructs the aim controller to aim at the given world position.
	 * @param pos World position to aim at.
	 * @param priority Aim task priority. If another AimAt task with higher priority is running, this call is rejected.
	 * @param duration How long in seconds to aim at the given position.
	 * @param reason Reason for this AimAt call (for debugging).
	 */
	virtual void AimAt(const Vector& pos, const LookPriority priority, const float duration, const char* reason = nullptr);
	/**
	 * @brief Instructs the aim controller to aim at the given target entity. The aiming system will track the entity position.
	 * @param entity Entity to aim at.
	 * @param priority Aim task priority. If another AimAt task with higher priority is running, this call is rejected.
	 * @param duration How long in seconds to aim at the given position.
	 * @param reason Reason for this AimAt call (for debugging).
	 */
	virtual void AimAt(CBaseEntity* entity, const LookPriority priority, const float duration, const char* reason = nullptr);
	// Translates the given angles into a world position and calls AimAt(const Vector& pos, const LookPriority priority, const float duration, const char* reason = nullptr)
	void AimAt(const QAngle& angles, const LookPriority priority, const float duration, const char* reason = nullptr);
	// Translates the given edict_t into a CBaseEntity and calls AimAt(CBaseEntity* entity, const LookPriority priority, const float duration, const char* reason = nullptr)
	void AimAt(edict_t* entity, const LookPriority priority, const float duration, const char* reason = nullptr);
	// Translates the given entity index into a CBaseEntity and calls AimAt(CBaseEntity* entity, const LookPriority priority, const float duration, const char* reason = nullptr)
	void AimAt(const int entity, const LookPriority priority, const float duration, const char* reason = nullptr);
	/**
	 * @brief Snaps the bot view angles to the given angles.
	 * @param angles Angles to snap to.
	 * @param priority Priority to prevent overriding an active AimAt task.
	 */
	virtual void SnapAimAt(const QAngle& angles, const LookPriority priority);
	// Translates the given position into angles and calls SnapAimAt(const QAngle& angles, const LookPriority priority)
	void SnapAimAt(const Vector& pos, const LookPriority priority);
	// True if the bot aim is Steady
	virtual const bool IsAimSteady() const { return m_isSteady; }
	// True if the bot aim is currently on target
	virtual const bool IsAimOnTarget() const { return m_isOnTarget; }
	// True if the bot aim at some point looked directly on it's target
	virtual const bool DidLookAtTarget() const { return m_didLookAtTarget; }
	// How long the bot aim has been steady
	virtual const float GetSteadyTime() const { return m_steadyTimer.HasStarted() ? m_steadyTimer.GetElapsedTime() : 0.0f; }

	void SetDesiredAimSpot(IDecisionQuery::DesiredAimSpot spot) { m_desiredAimSpot = spot; }
	void SetDesiredAimBone(const char* boneName)
	{
		if (boneName == nullptr)
		{
			m_desiredAimBone.clear();
		}
		else
		{
			m_desiredAimBone.assign(boneName);
		}
	}
	void SetDesiredAimOffset(const Vector& offset) { m_desiredAimOffset = offset; }
	IDecisionQuery::DesiredAimSpot GetCurrentDesiredAimSpot() const { return m_desiredAimSpot; }
	const std::string& GetCurrentDesiredAimBone() const { return m_desiredAimBone; }
	const Vector& GetCurrentDesiredAimOffset() const { return m_desiredAimOffset; }

private:

	void RunLook();

	LookPriority m_priority; // Current look priority
	CountdownTimer m_looktimer; // Timer for the current look at task
	Vector m_looktarget; // Look at target (Position Vector)
	CHandle<CBaseEntity> m_lookentity; // Look at target (Entity, overrides vector if present)
	QAngle m_lastangles; // Last bot view angles
	bool m_isSteady; // Is the bot aim steady?
	bool m_isOnTarget; // Is the bot looking at it's look target
	bool m_didLookAtTarget; // Did the bot look at it's current aim target at some point
	bool m_isSnapingViewAngles; // True if snapview was called
	QAngle m_snapToAngles;
	IntervalTimer m_steadyTimer; // Aim stability timer
	float m_aimSpeed;
	CountdownTimer m_trackingUpdateTimer; // Timer for tracking entities

	IDecisionQuery::DesiredAimSpot m_desiredAimSpot;
	std::string m_desiredAimBone; // bone name
	Vector m_desiredAimOffset; // offset for aim by offset
};

inline const char* GetLookPriorityName(IPlayerController::LookPriority priority)
{
	using namespace std::literals::string_view_literals;

	constexpr std::array names = {
		"NONE"sv,
		"AMBIENT"sv,
		"INTERESTING"sv,
		"ALERT"sv,
		"DANGER"sv,
		"ALLY"sv,
		"OPERATE"sv,
		"COMBAT"sv,
		"SUPPORT"sv,
		"VERY IMPORTANT"sv,
		"MOVEMENT"sv,
		"CRITICAL"sv,
		"MANDATORY"sv,
	};

	static_assert(names.size() == static_cast<size_t>(IPlayerController::MAX_LOOK_PRIORITY), "Look priority name array and enum size mismatch!");

	return names[static_cast<size_t>(priority)].data();
}


#endif // !SMNAV_BOT_BASE_PLAYER_CONTROL_IFACE_H_

