#include <extension.h>
#include <bot/basebot.h>
#include <sdkports/debugoverlay_shared.h>
#include <util/helpers.h>
#include <util/librandom.h>
#include <util/sdkcalls.h>
#include <entities/baseentity.h>
#include "playercontrol.h"

ConVar smnav_bot_aim_stability_max_rate("sm_navbot_aim_stability_max_rate", "100.0", FCVAR_NONE, "Maximum angle change rate to consider the bot aim to be stable.");
ConVar smnav_bot_aim_lookat_settle_duration("sm_navbot_aim_lookat_settle_duration", "0.3", FCVAR_NONE, "Amount of time the bot will wait for it's aim to stabilize before looking at a target of the same priority again.");

IPlayerController::IPlayerController(CBaseBot* bot) : IBotInterface(bot), IPlayerInput()
{
	ReleaseAllButtons();
	ResetInputData();
	m_looktimer.Invalidate();
	m_lookentity.Term();
	m_looktarget.Init();
	m_lastangles.Init();
	m_priority = LOOK_NONE;
	m_isSteady = false;
	m_isOnTarget = false;
	m_didLookAtTarget = false;
	m_steadyTimer.Invalidate();
	m_aimSpeed = GetBot()->GetDifficultyProfile()->GetAimSpeed();
	m_lastlookent = nullptr;
	m_aimerrorintervaltime.Invalidate();
	m_aimlockintimer.Invalidate();
	m_currentAimError = 0.0f;
}

IPlayerController::~IPlayerController()
{
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
	m_priority = LOOK_NONE;
	m_isSteady = false;
	m_isOnTarget = false;
	m_didLookAtTarget = false;
	m_steadyTimer.Invalidate();
	m_aimSpeed = GetBot()->GetDifficultyProfile()->GetAimSpeed();
	m_lastlookent = nullptr;
	m_aimerrorintervaltime.Invalidate();
	m_aimlockintimer.Invalidate();
	m_currentAimError = 0.0f;
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
	m_oldbuttons = m_buttons;
	CompileButtons();
	buttons = m_buttons;
	m_buttons = 0;
}

// This function handles the bot aiming/looking process
void IPlayerController::RunLook()
{
	float deltaTime = gpGlobals->frametime;

	if (deltaTime < 0.00001f)
	{
		return;
	}

	auto me = GetBot();
	auto currentAngles = me->GetEyeAngles();

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

	if (isSteady == true)
	{
		if (m_steadyTimer.HasStarted() == false)
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
		auto lookatentity = gamehelpers->GetHandleEntity(m_lookentity);

		if (lookatentity != nullptr)
		{
			auto index = gamehelpers->IndexOfEdict(lookatentity);

			// Look at entity is a player entity
			if (UtilHelpers::IsPlayerIndex(index))
			{
				// For players, we want to ask the bot behavior interface for a desired aim position
				// It will handle stuff like proper aiming for the current weapon, etc
				CBaseExtPlayer player(lookatentity);
				m_looktarget = GetBot()->GetBehaviorInterface()->GetTargetAimPos(GetBot(), lookatentity, &player);
			}
			else
			{
				// For non player entities, just aim at the center
				m_looktarget = UtilHelpers::getWorldSpaceCenter(lookatentity);
			}

			UpdateAimError();

			// if it's elapsed, the bot has locked in (no aim error)
			if (!m_aimlockintimer.IsElapsed())
			{
				entities::HBaseEntity be(m_lookentity.Get());
				Vector targetvel = be.GetAbsVelocity();

				if (targetvel.Length() > me->GetDifficultyProfile()->GetAimMinSpeedForError())
				{
					// apply error
					m_looktarget = (m_looktarget + targetvel * m_currentAimError);
				}
			}
		}
	}

	constexpr auto tolerance = 0.98f;
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

		NDebugOverlay::HorzArrow(eyepos, m_looktarget, width, red, green, 255, 255, false, DRAW_TIME);
	}

	// Updates the bot view angle, this is later sent on the User Command
	me->SetViewAngles(finalAngles);
}

void IPlayerController::UpdateAimError()
{
	CBaseBot* me = GetBot();

	if (me->GetDifficultyProfile()->GetAimLockInTime() < 0.02f)
	{
		m_currentAimError = 0.0f;
		m_aimlockintimer.Invalidate();
		return;
	}

	CBaseEntity* last = m_lastlookent.Get();
	CBaseEntity* current = m_lookentity.Get();

	if (last != current || m_aimerrorintervaltime.IsGreaterThen(me->GetDifficultyProfile()->GetAimLockInTime()))
	{
		// new target, recalculate error
		float errorvar = me->GetDifficultyProfile()->GetAimMovingError();
		m_currentAimError = randomgen->GetRandomReal<float>((errorvar * -1.0f), errorvar);
		m_aimlockintimer.Start(me->GetDifficultyProfile()->GetAimLockInTime());
		m_lastlookent = current;

		if (me->IsDebugging(BOTDEBUG_LOOK))
		{
			me->DebugPrintToConsole(230, 230, 250, "%s Aim Error: New Target! Current Error: %3.4f (%3.4f - %3.4f)\n", me->GetDebugIdentifier(), 
				m_currentAimError, (errorvar * -1.0f), errorvar);
		}
	}
	else
	{
		if (m_aimlockintimer.IsElapsed())
		{
			m_currentAimError = 0.0f;
		}
	}

	m_aimerrorintervaltime.Start();
}

void IPlayerController::AimAt(const Vector& pos, const LookPriority priority, const float duration, const char* reason)
{
	if (!m_looktimer.IsElapsed() && m_priority > priority)
	{
		return;
	}

	if (m_priority == priority)
	{
		// Don't reaim too frequently
		if (IsAimSteady() == false || GetSteadyTime() < smnav_bot_aim_lookat_settle_duration.GetFloat())
		{
			return;
		}
	}

	auto me = GetBot();
	if (me->IsDebugging(BOTDEBUG_LOOK))
	{
		me->DebugPrintToConsole(BOTDEBUG_LOOK, 255, 100, 0, "%s: AimAt (%3.2f, %3.2f, %3.2f) for %3.2f seconds. Priority: %s Reason: %s \n", me->GetDebugIdentifier(),
			pos.x, pos.y, pos.z, duration, GetLookPriorityName(priority), reason ? reason : "");
	}

	// experimental
	if (m_looktarget.DistTo(pos) > 64.0f)
	{
		m_isOnTarget = false;
	}

	m_priority = priority;
	m_looktimer.Start(duration);
	m_looktarget = pos;
	m_lookentity = nullptr;
	m_didLookAtTarget = false;

}

void IPlayerController::AimAt(CBaseEntity* entity, const LookPriority priority, const float duration, const char* reason)
{
	if (!m_looktimer.IsElapsed() && m_priority > priority)
	{
		return;
	}

	if (m_priority == priority)
	{
		// Don't reaim too frequently
		if (IsAimSteady() == false || GetSteadyTime() < smnav_bot_aim_lookat_settle_duration.GetFloat())
		{
			return;
		}
	}

	auto me = GetBot();
	if (me->IsDebugging(BOTDEBUG_LOOK))
	{
		me->DebugPrintToConsole(BOTDEBUG_LOOK, 255, 100, 0, "%s: AimAt \"%s\" for %3.2f seconds. Priority: %s Reason: %s \n", me->GetDebugIdentifier(),
			gamehelpers->GetEntityClassname(entity), duration, GetLookPriorityName(priority), reason ? reason : "");
	}

	// Experimental
	if (m_lookentity.Get() != entity)
	{
		m_isOnTarget = false;
	}

	m_priority = priority;
	m_looktimer.Start(duration);
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

	GetBot()->SnapEyeAngles(angles);
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
