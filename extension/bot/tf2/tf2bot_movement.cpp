#include <extension.h>
#include <sdkports/debugoverlay_shared.h>
#include <bot/tf2/tf2bot.h>
#include <mods/tf2/tf2lib.h>
#include <entities/tf2/tf_entities.h>
#include "tf2bot_movement.h"

#if SOURCE_ENGINE == SE_TF2 // don't register this convar outside of tf2 mods
static ConVar sm_navbot_tf_double_jump_z_boost("sm_navbot_tf_double_jump_z_boost", "300.0", FCVAR_CHEAT, "Amount of Z velocity to add when double jumping.");
#endif

class TF2DoubleJumpAction : public IMovement::Action<CTF2Bot>
{
public:
	TF2DoubleJumpAction(CTF2Bot* bot) : IMovement::Action<CTF2Bot>(bot)
	{
		m_boostTimer = -1;
	}

	void OnStart() override
	{
		GetBot()->GetControlInterface()->PressCrouchButton(0.4f);
		GetBot()->GetControlInterface()->PressJumpButton(0.2f);
		m_secondJumpTimer.Start(0.5f);
	}

	void Execute() override
	{
		if (m_secondJumpTimer.HasStarted() && m_secondJumpTimer.IsElapsed())
		{
			GetBot()->GetControlInterface()->PressJumpButton();
			m_secondJumpTimer.Invalidate();
			m_boostTimer = 9;
		}
#if SOURCE_ENGINE == SE_TF2
		if (m_boostTimer >= 0)
		{
			m_boostTimer--;

			if (m_boostTimer == 0)
			{
				Vector vel = GetBot()->GetAbsVelocity();
				vel.z = sm_navbot_tf_double_jump_z_boost.GetFloat();
				GetBot()->SetAbsVelocity(vel);
				m_doneTimer.Start(0.2f);
			}
		}
#endif
	}

	bool IsClimbingOrJumping() override { return true; }

	bool IsDone() override { return m_doneTimer.HasStarted() && m_doneTimer.IsElapsed(); }

private:
	CountdownTimer m_secondJumpTimer;
	CountdownTimer m_doneTimer;
	int m_boostTimer;
};

class TF2JumpOverGapAction : public IMovement::Action<CTF2Bot>
{
public:
	TF2JumpOverGapAction(CTF2Bot* bot, const Vector& landing, const Vector& forward) : IMovement::Action<CTF2Bot>(bot)
	{
		m_landing = landing;
		m_forward = forward;
		m_wasairborne = false;
		m_onground = false;
		m_boostTimer = -1;

		float jumplength = (GetBot()->GetAbsOrigin() - landing).Length();

		m_requiresDJ = (jumplength >= CTF2BotMovement::scout_gap_jump_do_double_distance());
	}

	void OnStart() override
	{
		GetBot()->GetControlInterface()->AimAt(m_landing + Vector(0.0f, 0.0f, 48.0f), IPlayerController::LOOK_MOVEMENT, 0.5f, "Looking at jump landing spot!");
		GetBot()->GetControlInterface()->PressJumpButton();

		if (m_requiresDJ)
		{
			m_secondJumpTimer.Start(0.5f);
		}

		if (GetBot()->IsDebugging(BOTDEBUG_MOVEMENT))
		{
			Vector top = GetBot()->GetAbsOrigin();
			top.z += GetBot()->GetMovementInterface()->GetMaxJumpHeight();

			NDebugOverlay::VertArrow(GetBot()->GetAbsOrigin(), top, 5.0f, 0, 0, 255, 255, false, 5.0f);
			NDebugOverlay::HorzArrow(top, m_landing, 5.0f, 0, 255, 0, 255, false, 5.0f);
		}
	}

	void Execute() override
	{
		if (m_onground)
			return;

		bool airborne = !GetBot()->GetMovementInterface()->IsOnGround();

		if (!m_wasairborne && airborne)
		{
			m_wasairborne = true;
		}

		Vector toLanding = m_landing - GetBot()->GetAbsOrigin();
		toLanding.z = 0.0f;
		toLanding.NormalizeInPlace();

		if (airborne)
		{
			GetBot()->GetControlInterface()->AimAt(GetBot()->GetEyeOrigin() + 100.0f * toLanding, IPlayerController::LOOK_MOVEMENT, 0.25f);
		}
		else
		{
			if (m_wasairborne)
			{
				m_onground = true; // end jump
			}
		}
#if SOURCE_ENGINE == SE_TF2
		if (m_requiresDJ)
		{
			if (m_secondJumpTimer.HasStarted() && m_secondJumpTimer.IsElapsed())
			{
				m_secondJumpTimer.Invalidate();
				m_boostTimer = 8;
			}


			if (m_boostTimer >= 0)
			{
				m_boostTimer--;

				if (m_boostTimer == 0)
				{
					Vector vel = GetBot()->GetAbsVelocity();
					vel.z = sm_navbot_tf_double_jump_z_boost.GetFloat();
					GetBot()->SetAbsVelocity(vel);
				}
			}
		}
#endif

		GetBot()->GetMovementInterface()->MoveTowards(m_landing);
	}

	bool BlockNavigator() override { return true; }
	bool IsClimbingOrJumping() override { return true; }

	bool IsDone() override { return m_wasairborne && !m_onground; }

private:
	Vector m_landing;
	Vector m_forward;
	bool m_wasairborne;
	bool m_onground;
	bool m_requiresDJ;
	CountdownTimer m_secondJumpTimer;
	int m_boostTimer;
};

CTF2BotMovement::CTF2BotMovement(CBaseBot* bot) : IMovement(bot)
{
}

CTF2BotMovement::~CTF2BotMovement()
{
}

float CTF2BotMovement::GetHullWidth()
{
	float scale = GetBot()->GetModelScale();
	return PLAYER_HULL_WIDTH * scale;
}

float CTF2BotMovement::GetStandingHullHeigh()
{
	float scale = GetBot()->GetModelScale();
	return PLAYER_HULL_STAND * scale;
}

float CTF2BotMovement::GetCrouchedHullHeigh()
{
	float scale = GetBot()->GetModelScale();
	return PLAYER_HULL_CROUCH * scale;
}

float CTF2BotMovement::GetMaxGapJumpDistance() const
{
	auto cls = tf2lib::GetPlayerClassType(GetBot()->GetIndex());

	switch (cls)
	{
	case TeamFortress2::TFClass_Scout:
		return 600.0f; // double jump gap distance
	case TeamFortress2::TFClass_Soldier:
		return 214.0f;
	case TeamFortress2::TFClass_DemoMan:
		return 240.0f;
	case TeamFortress2::TFClass_Medic:
		return 270.0f;
	case TeamFortress2::TFClass_Heavy:
		return 207.0f;
	case TeamFortress2::TFClass_Spy:
		return 270.0f;
	default:
		return 257.0f; // classes that moves at 'default speed' (engineer, sniper, pyro)
	}
}

void CTF2BotMovement::DoubleJump()
{
	ExecuteMovementAction<TF2DoubleJumpAction>(false, GetTF2Bot());
}

bool CTF2BotMovement::IsAbleToDoubleJump()
{
	return GetTF2Bot()->GetMyClassType() == TeamFortress2::TFClassType::TFClass_Scout;
}

bool CTF2BotMovement::IsAbleToBlastJump()
{
	auto cls = tf2lib::GetPlayerClassType(GetBot()->GetIndex());

	// TO-DO: Demoman and maybe engineer
	if (cls == TeamFortress2::TFClass_Soldier)
	{
		if (GetBot()->GetHealth() < min_health_for_rocket_jumps())
		{
			return false;
		}

		return true;
	}

	return false;
}

void CTF2BotMovement::JumpAcrossGap(const Vector& landing, const Vector& forward)
{
	ExecuteMovementAction<TF2JumpOverGapAction>(false, GetTF2Bot(), landing, forward);
}

bool CTF2BotMovement::IsEntityTraversable(edict_t* entity, const bool now)
{
	int index = gamehelpers->IndexOfEdict(entity);
	auto team = tf2lib::GetEntityTFTeam(index);

	/* TO-DO: check solid teammates cvar */
	if (UtilHelpers::IsPlayerIndex(index) && GetTF2Bot()->GetMyTFTeam() == team)
	{
		return true;
	}

	return IMovement::IsEntityTraversable(entity, now);
}

CTF2Bot* CTF2BotMovement::GetTF2Bot() const
{
	return static_cast<CTF2Bot*>(GetBot());
}

