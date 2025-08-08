#include NAVBOT_PCH_FILE
#include <extension.h>
#include <bot/basebot.h>
#include <sdkports/debugoverlay_shared.h>
#include <util/helpers.h>
#include <util/librandom.h>
#include <util/sdkcalls.h>
#include <entities/baseentity.h>
#include "playercontrol.h"

#ifdef EXT_VPROF_ENABLED
#include <tier0/vprof.h>
#endif // EXT_VPROF_ENABLED

ConVar smnav_bot_aim_stability_max_rate("sm_navbot_aim_stability_max_rate", "100.0", FCVAR_GAMEDLL, "Maximum angle change rate to consider the bot aim to be stable.");
#ifdef PROBLEMATIC
ConVar smnav_bot_aim_lookat_settle_duration("sm_navbot_aim_lookat_settle_duration", "0.3", FCVAR_GAMEDLL, "Amount of time the bot will wait for it's aim to stabilize before looking at a target of the same priority again.");
#endif // PROBLEMATIC

IPlayerController::IPlayerController(CBaseBot* bot) : IBotInterface(bot), IPlayerInput()
{
	ReleaseAllButtons();
	ResetInputData();
	m_looktimer.Invalidate();
	m_lookentity.Term();
	m_looktarget.Init();
	m_lastangles.Init();
	m_priority = LOOK_IDLE;
	m_isSteady = false;
	m_isOnTarget = false;
	m_didLookAtTarget = false;
	m_isSnapingViewAngles = false;
	m_snapToAngles = vec3_angle;
	m_steadyTimer.Invalidate();
	m_aimSpeed = GetBot()->GetDifficultyProfile()->GetAimSpeed();
	m_desiredAimSpot = IDecisionQuery::DesiredAimSpot::AIMSPOT_NONE;
	m_desiredAimBone.reserve(32);
	m_desiredAimOffset.Init();
}

IPlayerController::~IPlayerController()
{
}

const char* IPlayerController::GetLookPriorityName(IPlayerController::LookPriority priority)
{
	using namespace std::literals::string_view_literals;

	constexpr std::array names = {
		"IDLE"sv,
		"PATH"sv,
		"INTERESTING"sv,
		"ALLY"sv,
		"SEARCH"sv,
		"ALERT"sv,
		"DANGER"sv,
		"USE"sv,
		"COMBAT"sv,
		"SUPPORT"sv,
		"PRIORITY"sv,
		"MOVEMENT"sv,
		"CRITICAL"sv,
		"MANDATORY"sv,
	};

	static_assert(names.size() == static_cast<size_t>(IPlayerController::MAX_LOOK_PRIORITY), "Look priority name array and enum size mismatch!");

	return names[static_cast<size_t>(priority)].data();
}

void IPlayerController::OnDifficultyProfileChanged()
{
	m_aimSpeed = GetBot()->GetDifficultyProfile()->GetAimSpeed();
}

void IPlayerController::Reset()
{
	ReleaseAllButtons();
	ResetInputData();
	m_looktimer.Invalidate();
	m_lookentity.Term();
	m_looktarget.Init();
	m_lastangles.Init();
	m_priority = LOOK_IDLE;
	m_isSteady = false;
	m_isOnTarget = false;
	m_didLookAtTarget = true;
	m_isSnapingViewAngles = false;
	m_snapToAngles = vec3_angle;
	m_steadyTimer.Invalidate();
	m_aimSpeed = GetBot()->GetDifficultyProfile()->GetAimSpeed();
	m_trackingUpdateTimer.Invalidate();
	m_desiredAimSpot = IDecisionQuery::DesiredAimSpot::AIMSPOT_NONE;
	m_desiredAimBone.clear();
	m_desiredAimOffset.Init();
}

void IPlayerController::Update()
{
}

void IPlayerController::Frame()
{
	RunLook();
}

/**
 * @brief Compiles the buttons currently pressed for being sent as a user command to the server.
 * @param buttons Reference to store the buttons to be sent
*/
void IPlayerController::ProcessButtons(int& buttons)
{
	CompileButtons();
	m_oldbuttons = m_buttons;
	buttons = m_buttons;
	// m_buttons = 0;
}

void IPlayerController::ForceUpdateAimOnTarget(const float tolerance)
{
	if (m_didLookAtTarget && m_looktimer.IsElapsed())
	{
		return;
	}

	CBaseBot* me = GetBot<CBaseBot>();
	auto eyepos = me->GetEyeOrigin();
	auto to = m_looktarget - eyepos;
	to.NormalizeInPlace();
	QAngle desiredAngles;
	VectorAngles(to, desiredAngles);
	QAngle finalAngles(0.0f, 0.0f, 0.0f);
	Vector forward;
	me->EyeVectors(&forward);
	const float dot = DotProduct(forward, to);

	if (dot > tolerance)
	{
		// bot is looking at it's target
		m_isOnTarget = true;

		if (m_didLookAtTarget == false)
		{
			m_didLookAtTarget = true; // first time the bot aim was on target since the last AimAt call
		}
	}
	else
	{
		m_isOnTarget = false; // aim is off target
	}
}

// This function handles the bot aiming/looking process
void IPlayerController::RunLook()
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("IPlayerController::RunLook", "NavBot");
#endif // EXT_VPROF_ENABLED

	CBaseBot* me = GetBot<CBaseBot>();

	if (m_isSnapingViewAngles)
	{
		m_isSnapingViewAngles = false;
		me->SetViewAngles(m_snapToAngles);
		return;
	}

	float deltaTime = gpGlobals->frametime;

	if (deltaTime < 0.00001f)
	{
		return;
	}
	
	QAngle currentAngles = me->GetEyeAngles() + me->GetPunchAngle();

	bool isSteady = true;

	// remember, pitch is X, yaw is Y and roll is Z

	float pitchChangeRate = AngleDiff(currentAngles.x, m_lastangles.x);
	float yawChangeRate = AngleDiff(currentAngles.y, m_lastangles.y);

	if (fabsf(pitchChangeRate) > smnav_bot_aim_stability_max_rate.GetFloat() * deltaTime)
	{
		isSteady = false;
	}
	else if (fabsf(yawChangeRate) > smnav_bot_aim_stability_max_rate.GetFloat() * deltaTime)
	{
		isSteady = false;
	}

	if (isSteady)
	{
		if (!m_steadyTimer.HasStarted())
		{
			m_steadyTimer.Start();
		}
	}
	else
	{
		m_steadyTimer.Invalidate();
	}

	m_isSteady = isSteady;
	m_lastangles = currentAngles; // store the last frame angles

	// Bot aimed at the last call target and the look timer expired
	if (m_didLookAtTarget && m_looktimer.IsElapsed())
	{
		return;
	}

	if (m_lookentity.IsValid())
	{
		CBaseEntity* pEntity = m_lookentity.Get();

		if (pEntity)
		{
			if (m_trackingUpdateTimer.IsElapsed())
			{
				m_looktarget = GetBot<CBaseBot>()->GetBehaviorInterface()->GetTargetAimPos(GetBot<CBaseBot>(), pEntity, m_desiredAimSpot);
				m_trackingUpdateTimer.Start(GetBot<CBaseBot>()->GetDifficultyProfile()->GetAimTrackingInterval());
			}
		}
	}

	constexpr auto tolerance = IPlayerController::AIM_ON_TARGET_DOT_TOLERANCE;
	auto eyepos = me->GetEyeOrigin();
	auto to = m_looktarget - eyepos;
	to.NormalizeInPlace();
	QAngle desiredAngles;
	VectorAngles(to, desiredAngles);
	QAngle finalAngles(0.0f, 0.0f, 0.0f);
	Vector forward;
	me->EyeVectors(&forward);
	const float dot = DotProduct(forward, to);

	if (dot > tolerance)
	{
		// bot is looking at it's target
		m_isOnTarget = true;

		if (m_didLookAtTarget == false)
		{
			m_didLookAtTarget = true; // first time the bot aim was on target since the last AimAt call

			if (me->IsDebugging(BOTDEBUG_LOOK))
			{
				me->DebugPrintToConsole(0, 180, 0, "%s: LOOK AT ON TARGET! \n", me->GetDebugIdentifier());
			}
		}
	}
	else
	{
		m_isOnTarget = false; // aim is off target
	}

	finalAngles.x = ApproachAngle(desiredAngles.x, currentAngles.x, m_aimSpeed * deltaTime);
	finalAngles.y = ApproachAngle(desiredAngles.y, currentAngles.y, m_aimSpeed * deltaTime);

	finalAngles.x = AngleNormalize(finalAngles.x);
	finalAngles.y = AngleNormalize(finalAngles.y);

	if (me->IsDebugging(BOTDEBUG_LOOK))
	{
		constexpr auto DRAW_TIME = 0.0f;
		NDebugOverlay::Line(eyepos, eyepos + 256.0f * forward, 255, 255, 0, true, DRAW_TIME);

		float width = isSteady ? 2.0f : 6.0f;
		int red = m_isOnTarget ? 255 : 0;
		int green = m_lookentity.IsValid() ? 255 : 0;

		NDebugOverlay::HorzArrow(eyepos, m_looktarget, width, red, green, 255, 255, true, DRAW_TIME);
	}

	// Updates the bot view angle, this is later sent on the User Command
	me->SetViewAngles(finalAngles);
}

void IPlayerController::AimAt(const Vector& pos, const LookPriority priority, const float duration, const char* reason)
{
	CBaseBot* me = GetBot<CBaseBot>();

	if (!m_looktimer.IsElapsed() && m_priority > priority)
	{
		if (me->IsDebugging(BOTDEBUG_LOOK))
		{
			me->DebugPrintToConsole(BOTDEBUG_LOOK, 255, 0, 0, "%s: AimAt (%3.2f, %3.2f, %3.2f) rejected! Higher priority look is active! Priority: %s Reason: %s \n", me->GetDebugIdentifier(),
				pos.x, pos.y, pos.z, GetLookPriorityName(priority), reason ? reason : "");
		}

		return;
	}

	/* Disabled: causes issues with short duration look at commands. */
#ifdef PROBLEMATIC
	if (m_priority == priority)
	{
		// Don't reaim too frequently
		if (IsAimSteady() == false || GetSteadyTime() < smnav_bot_aim_lookat_settle_duration.GetFloat())
		{
			if (me->IsDebugging(BOTDEBUG_LOOK))
			{
				me->DebugPrintToConsole(BOTDEBUG_LOOK, 255, 0, 0, "%s: AimAt (%3.2f, %3.2f, %3.2f) rejected! Aim is not steady! Priority: %s Reason: %s \n", me->GetDebugIdentifier(),
					pos.x, pos.y, pos.z, GetLookPriorityName(priority), reason ? reason : "");
			}

			return;
		}
	}
#endif // PROBLEMATIC

	if (me->IsDebugging(BOTDEBUG_LOOK))
	{
		me->DebugPrintToConsole(BOTDEBUG_LOOK, 255, 100, 0, "%s: AimAt (%3.2f, %3.2f, %3.2f) for %3.2f seconds. Priority: %s Reason: %s \n", me->GetDebugIdentifier(),
			pos.x, pos.y, pos.z, duration, GetLookPriorityName(priority), reason ? reason : "");
	}

	m_looktimer.Start(duration);

	// If given the same position, just update priority
	if ((m_looktarget - pos).IsLengthLessThan(1.0f))
	{
		m_priority = priority;
		return;
	}

	// experimental
	if (m_looktarget.DistTo(pos) > 64.0f)
	{
		m_isOnTarget = false;
	}

	m_priority = priority;
	m_looktarget = pos;
	m_lookentity = nullptr;
	m_didLookAtTarget = false;
}

void IPlayerController::AimAt(CBaseEntity* entity, const LookPriority priority, const float duration, const char* reason)
{
	CBaseBot* me = GetBot<CBaseBot>();

	if (!m_looktimer.IsElapsed() && m_priority > priority)
	{
		if (me->IsDebugging(BOTDEBUG_LOOK))
		{
			me->DebugPrintToConsole(BOTDEBUG_LOOK, 255, 0, 0, "%s: AimAt \"%s\" rejected! Higher priority look is active! Priority: %s Reason: %s \n", me->GetDebugIdentifier(),
				gamehelpers->GetEntityClassname(entity), GetLookPriorityName(priority), reason ? reason : "");
		}

		return;
	}

#ifdef PROBLEMATIC
	if (m_priority == priority)
	{
		// Don't reaim too frequently
		if (IsAimSteady() == false || GetSteadyTime() < smnav_bot_aim_lookat_settle_duration.GetFloat())
		{
			if (me->IsDebugging(BOTDEBUG_LOOK))
			{
				me->DebugPrintToConsole(BOTDEBUG_LOOK, 255, 0, 0, "%s: AimAt \"%s\" rejected! Aim is not steady! Priority: %s Reason: %s \n", me->GetDebugIdentifier(),
					gamehelpers->GetEntityClassname(entity), GetLookPriorityName(priority), reason ? reason : "");
			}

			return;
		}
	}
#endif // PROBLEMATIC


	if (me->IsDebugging(BOTDEBUG_LOOK))
	{
		me->DebugPrintToConsole(BOTDEBUG_LOOK, 255, 100, 0, "%s: AimAt \"%s\" for %3.2f seconds. Priority: %s Reason: %s \n", me->GetDebugIdentifier(),
			gamehelpers->GetEntityClassname(entity), duration, GetLookPriorityName(priority), reason ? reason : "");
	}

	m_looktimer.Start(duration);

	// If given the same target entity again, just update the priority.
	if (m_lookentity.Get() == entity)
	{
		m_priority = priority;
		return;
	}

	// Experimental
	if (m_lookentity.Get() != entity)
	{
		m_isOnTarget = false;
	}

	m_priority = priority;
	m_lookentity = entity;
	m_didLookAtTarget = false;
}

void IPlayerController::AimAt(const QAngle& angles, const LookPriority priority, const float duration, const char* reason)
{
	Vector forward;
	AngleVectors(angles, &forward);
	forward.NormalizeInPlace();
	Vector goal = GetBot()->GetEyeOrigin() + (forward * 512.0f);
	AimAt(goal, priority, duration, reason);
}

void IPlayerController::AimAt(edict_t* entity, const LookPriority priority, const float duration, const char* reason)
{
	AimAt(entity->GetIServerEntity()->GetBaseEntity(), priority, duration, reason);
}

void IPlayerController::AimAt(const int entity, const LookPriority priority, const float duration, const char* reason)
{
	edict_t* edict = gamehelpers->EdictOfIndex(entity);
	AimAt(edict->GetIServerEntity()->GetBaseEntity(), priority, duration, reason);
}

void IPlayerController::SnapAimAt(const QAngle& angles, const LookPriority priority)
{
	if (!m_looktimer.IsElapsed() && m_priority > priority)
	{
		return;
	}

	m_isSnapingViewAngles = true;
	m_snapToAngles = angles; // runlook will send the angle for us
}

void IPlayerController::SnapAimAt(const Vector& pos, const LookPriority priority)
{
	if (!m_looktimer.IsElapsed() && m_priority > priority)
	{
		return;
	}

	QAngle lookAngles;
	Vector origin = GetBot()->GetEyeOrigin();
	Vector to = (pos - origin);
	to.NormalizeInPlace();
	VectorAngles(to, lookAngles);

	lookAngles.x = AngleNormalize(lookAngles.x);
	lookAngles.y = AngleNormalize(lookAngles.y);

	SnapAimAt(lookAngles, priority);
}
