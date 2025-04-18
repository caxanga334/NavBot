#include <extension.h>
#include <sdkports/debugoverlay_shared.h>
#include <bot/tf2/tf2bot.h>
#include <mods/tf2/tf2lib.h>
#include <entities/tf2/tf_entities.h>
#include "tf2bot_movement.h"

static ConVar sm_navbot_tf_double_jump_z_boost("sm_navbot_tf_double_jump_z_boost", "300.0", FCVAR_CHEAT, "Amount of Z velocity to add when double jumping.");

CTF2BotMovement::CTF2BotMovement(CBaseBot* bot) : IMovement(bot)
{
}

CTF2BotMovement::~CTF2BotMovement()
{
}

void CTF2BotMovement::Reset()
{
	IMovement::Reset();
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
		return 216.0f;
	case TeamFortress2::TFClass_DemoMan:
		return 240.0f;
	case TeamFortress2::TFClass_Medic:
		return 272.0f;
	case TeamFortress2::TFClass_Heavy:
		return 209.0f;
	case TeamFortress2::TFClass_Spy:
		return 272.0f;
	default:
		return 258.0f; // classes that moves at 'default speed' (engineer, sniper, pyro)
	}
}

void CTF2BotMovement::CrouchJump()
{
	CTF2Bot* me = GetBot<CTF2Bot>();

	// TF2: Cannot jump while crouched, if not jumping already, release the crouch button
	if (!m_jumpCooldown.HasStarted() && !m_isJumping && me->GetControlInterface()->IsPressingCrouchButton())
	{
		me->GetControlInterface()->ReleaseCrouchButton();
		me->GetControlInterface()->ReleaseJumpButton();
		// start the jump cooldown timer to give some time for the player to uncrouch
		m_jumpCooldown.Start(0.3f);
		return;
	}

	IMovement::CrouchJump();
}

bool CTF2BotMovement::IsAbleToDoubleJump()
{
	auto cls = tf2lib::GetPlayerClassType(GetBot()->GetIndex());

	if (cls == TeamFortress2::TFClass_Scout)
	{
		return true;
	}

	return false;
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
	CTF2Bot* me = GetBot<CTF2Bot>();

	CrouchJump();

	// look towards the jump target
	me->GetControlInterface()->AimAt(landing, IPlayerController::LOOK_MOVEMENT, 1.0f);

	m_isJumpingAcrossGap = true;
	m_landingGoal = landing;
	m_isAirborne = false;

	if (me->GetMyClassType() == TeamFortress2::TFClass_Scout)
	{
		float jumplength = (me->GetAbsOrigin() - landing).Length();

		if (jumplength >= scout_gap_jump_do_double_distance())
		{
			// FIX ME!
			// m_doublejumptimer.Start(0.5f);
		}
	}

	if (me->IsDebugging(BOTDEBUG_MOVEMENT))
	{
		Vector top = me->GetAbsOrigin();
		top.z += GetMaxJumpHeight();

		NDebugOverlay::VertArrow(me->GetAbsOrigin(), top, 5.0f, 0, 0, 255, 255, false, 5.0f);
		NDebugOverlay::HorzArrow(top, landing, 5.0f, 0, 255, 0, 255, false, 5.0f);
	}
}

bool CTF2BotMovement::IsEntityTraversable(int index, edict_t* edict, CBaseEntity* entity, const bool now)
{
	auto theirteam = tf2lib::GetEntityTFTeam(index);
	auto myteam = GetBot<CTF2Bot>()->GetMyTFTeam();

	if (myteam == theirteam)
	{
		/* TO-DO: check solid teammates cvar */
		if (UtilHelpers::IsPlayerIndex(index))
		{
			return true;
		}

		if (UtilHelpers::FClassnameIs(entity, "obj_*"))
		{
			if (GetBot<CTF2Bot>()->GetMyClassType() == TeamFortress2::TFClassType::TFClass_Engineer)
			{
				CBaseEntity* builder = tf2lib::GetBuildingBuilder(entity);

				if (builder == GetBot<CTF2Bot>()->GetEntity())
				{
					return false; // my own buildings are solid to me
				}

				return true; // not my own buildings, not solid
			}

			return true; // not an engineer, friendly buildings are not solid (the telepoter is solid but we can generally walk over it)
		}
	}

	return IMovement::IsEntityTraversable(index, edict, entity, now);
}

