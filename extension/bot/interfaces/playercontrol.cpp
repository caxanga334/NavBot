#include <extension.h>
#include <bot/basebot.h>
#include <sdkports/debugoverlay_shared.h>
#include <ifaces_extern.h>
#include <util/helpers.h>
#include "playercontrol.h"

ConVar smnav_bot_aim_stability_max_rate("sm_navbot_aim_stability_max_rate", "100.0", FCVAR_NONE, "Maximum angle change rate to consider the bot aim to be stable.");
ConVar smnav_bot_aim_lookat_settle_duration("sm_navbot_aim_lookat_settle_duration", "0.3", FCVAR_NONE, "Amount of time the bot will wait for it's aim to stabilize before looking at a target of the same priority again.");

IPlayerController::IPlayerController(CBaseBot* bot) : IBotInterface(bot), IPlayerInput()
{
	Reset();
}

IPlayerController::~IPlayerController()
{
}

void IPlayerController::OnDifficultyProfileChanged()
{
	auto& profile = GetBot()->GetDifficultyProfile();
	m_aimspeed = profile.GetAimSpeed();
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

	auto& profile = GetBot()->GetDifficultyProfile();
	m_aimspeed = profile.GetAimSpeed();
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
	auto me = GetBot();
	auto currentAngles = me->GetEyeAngles();
	float deltaTime = gpGlobals->frametime;

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

	// no need to make changes anymore
	if (m_looktimer.IsElapsed())
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
		}
	}

	constexpr auto tolerance = 0.98f;
	auto to = m_looktarget - me->GetEyeOrigin();
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
#ifdef EXT_DEBUG
			rootconsole->ConsolePrint("BOT#%i Look At ON_TARGET", GetBot()->GetIndex());
#endif // EXT_DEBUG
		}
	}
	else
	{
		m_isOnTarget = false; // aim is off target
	}

	finalAngles.x = ApproachAngle(desiredAngles.x, currentAngles.x, m_aimspeed);
	finalAngles.y = ApproachAngle(desiredAngles.y, currentAngles.y, m_aimspeed);

	finalAngles.x = AngleNormalize(finalAngles.x);
	finalAngles.y = AngleNormalize(finalAngles.y);

	// Updates the bot view angle, this is later sent on the User Command
	me->SetViewAngles(finalAngles);

#ifdef EXT_DEBUG
	float width = isSteady ? 4.0f : 2.0f;
	int red = m_isOnTarget ? 255 : 0;
	NDebugOverlay::HorzArrow(me->GetEyeOrigin(), m_looktarget, width, red, 0, 255, 200, false, EXT_DEBUG_DRAW_TIME);
#endif // EXT_DEBUG
}

void IPlayerController::AimAt(const Vector& pos, const LookPriority priority, const float duration)
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

	m_priority = priority;
	m_looktimer.Start(duration);
	m_looktarget = pos;
	m_lookentity.Set(nullptr);
	m_didLookAtTarget = false;
}

void IPlayerController::AimAt(edict_t* entity, const LookPriority priority, const float duration)
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

	m_priority = priority;
	m_looktimer.Start(duration);
	gamehelpers->SetHandleEntity(m_lookentity, entity);
	m_didLookAtTarget = false;
}

void IPlayerController::AimAt(const int entity, const LookPriority priority, const float duration)
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

	m_priority = priority;
	m_looktimer.Start(duration);
	auto edict = gamehelpers->EdictOfIndex(entity);
	gamehelpers->SetHandleEntity(m_lookentity, edict);
	m_didLookAtTarget = false;
}
