#include <extension.h>
#include <bot/basebot.h>
#include <sdkports/debugoverlay_shared.h>
#include <ifaces_extern.h>
#include <util/helpers.h>
#include "playercontrol.h"

IPlayerController::IPlayerController(CBaseBot* bot) : IBotInterface(bot), IPlayerInput(),
	m_priority(LOOK_NONE),
	m_looktarget(),
	m_lookentity()
{

}

IPlayerController::~IPlayerController()
{
}

void IPlayerController::Reset()
{
	ReleaseAllButtons();
	m_looktimer.Invalidate();
	m_lookentity.Term();
	m_looktarget.Init();
}

void IPlayerController::Update()
{
}

void IPlayerController::Frame()
{
	RunLook();
}

void IPlayerController::ProcessButtons(CBotCmd* cmd) // this is called by the CBaseBot::Frame
{
	m_oldbuttons = m_buttons;
	CompileButtons();
	cmd->buttons = m_buttons;
	m_buttons = 0;
}

void IPlayerController::RunLook()
{
	if (m_looktimer.IsElapsed())
	{
		return; // Not looking
	}

	auto me = GetBot();
	auto currentAngles = me->GetEyeAngles();

	if (m_lookentity.IsValid())
	{
		auto lookatentity = gamehelpers->GetHandleEntity(m_lookentity);

		if (lookatentity != nullptr)
		{
			auto index = gamehelpers->IndexOfEdict(lookatentity);

			// Look at entity is a player entity
			if (index > 0 && index < gpGlobals->maxClients)
			{
				auto player = extmanager->GetPlayerByIndex(index);
				// TO-DO
			}
			else
			{
				// For non player entities, just aim at the center
				m_looktarget = UtilHelpers::getWorldSpaceCenter(lookatentity);
			}
		}
	}

	auto to = m_looktarget - me->GetEyeOrigin();
	QAngle desiredAngles;
	VectorAngles(to, desiredAngles);
	QAngle finalAngles;

	constexpr auto APPROACH_ANGLE_SPEED = 30.0f; // to-do: add config or convar
	finalAngles.x = ApproachAngle(desiredAngles.x, currentAngles.x, APPROACH_ANGLE_SPEED);
	finalAngles.y = ApproachAngle(desiredAngles.y, currentAngles.y, APPROACH_ANGLE_SPEED);

	finalAngles.x = AngleNormalize(finalAngles.x);
	finalAngles.y = AngleNormalize(finalAngles.y);

	// Updates the bot view angle, this is later sent on the User Command
	me->SetViewAngles(finalAngles);

#ifdef SMNAV_DEBUG
	NDebugOverlay::HorzArrow(me->GetEyeOrigin(), m_looktarget, 4.0f, 0, 0, 255, 200, false, NDEBUG_SMNAV_DRAW_TIME);
#endif // SMNAV_DEBUG
}

void IPlayerController::AimAt(const Vector& pos, const LookPriority priority, const float duration)
{
	if (!m_looktimer.IsElapsed() && m_priority > priority)
	{
		return;
	}

	m_priority = priority;
	m_looktimer.Start(duration);
	m_looktarget = pos;
}

void IPlayerController::AimAt(edict_t* entity, const LookPriority priority, const float duration)
{
	if (!m_looktimer.IsElapsed() && m_priority > priority)
	{
		return;
	}

	m_priority = priority;
	m_looktimer.Start(duration);
	gamehelpers->SetHandleEntity(m_lookentity, entity);
}

void IPlayerController::AimAt(const int entity, const LookPriority priority, const float duration)
{
	if (!m_looktimer.IsElapsed() && m_priority > priority)
	{
		return;
	}

	m_priority = priority;
	m_looktimer.Start(duration);
	auto edict = gamehelpers->EdictOfIndex(entity);
	gamehelpers->SetHandleEntity(m_lookentity, edict);
}
