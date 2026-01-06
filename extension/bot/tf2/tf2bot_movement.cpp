#include NAVBOT_PCH_FILE
#include <extension.h>
#include <sdkports/debugoverlay_shared.h>
#include <bot/tf2/tf2bot.h>
#include <mods/tf2/tf2lib.h>
#include <mods/tf2/teamfortress2mod.h>
#include <entities/tf2/tf_entities.h>
#include "tf2bot_movement.h"

#if SOURCE_ENGINE == SE_TF2
static ConVar cvar_rj_fire_delay("sm_navbot_tf_rocket_jump_fire_delay", "0.060", FCVAR_GAMEDLL);
static ConVar cvar_rj_dot("sm_navbot_tf_rocket_fire_dot_product", "0.95", FCVAR_GAMEDLL);
#endif // SOURCE_ENGINE == SE_TF2


CTF2BotMovement::CTF2BotMovement(CBaseBot* bot) : IMovement(bot)
{
	m_blastJumpLandingGoal = vec3_origin;
	m_blastJumpStart = vec3_origin;
	m_grapplingHookGoal = vec3_origin;
	m_bIsBlastJumping = false;
	m_bBlastJumpAlreadyFired = false;
	m_bIsUsingGrapplingHook = false;
}

CTF2BotMovement::~CTF2BotMovement()
{
}

void CTF2BotMovement::Reset()
{
	m_blastJumpLandingGoal = vec3_origin;
	m_blastJumpStart = vec3_origin;
	m_bIsBlastJumping = false;
	m_bBlastJumpAlreadyFired = false;

	IMovement::Reset();
}

void CTF2BotMovement::Update()
{
	IMovement::Update();

	if (m_bIsBlastJumping)
	{
		BlastJumpUpdate();
	}

	if (m_bIsUsingGrapplingHook)
	{
		GrapplingHookUpdate();
	}
}

float CTF2BotMovement::GetMaxDoubleJumpHeight() const
{
	if (CTeamFortress2Mod::GetTF2Mod()->GetCurrentGameMode() == TeamFortress2::GameModeType::GM_VSH)
	{
		if (GetBot<CTF2Bot>()->IsSaxtonHale())
		{
			return 250.0f;
		}
	}

	return 116.0f;
}

float CTF2BotMovement::GetMaxGapJumpDistance() const
{
	if (CTeamFortress2Mod::GetTF2Mod()->GetCurrentGameMode() == TeamFortress2::GameModeType::GM_VSH)
	{
		if (GetBot<CTF2Bot>()->IsSaxtonHale())
		{
			return 600.0f;
		}
	}

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
	if (!m_jumpCooldown.HasStarted() && IsOnGround() && me->GetControlInterface()->IsPressingCrouchButton())
	{
		me->GetControlInterface()->ReleaseCrouchButton();
		me->GetControlInterface()->ReleaseJumpButton();
		// start the jump cooldown timer to give some time for the player to uncrouch
		m_jumpCooldown.Start(0.5f);

		if (me->IsDebugging(BOTDEBUG_MOVEMENT))
		{
			me->DebugPrintToConsole(255, 165, 0, "%s CTF2BotMovement::CrouchJump JUMP REJECTED, BOT IS CROUCHING! \n", me->GetDebugIdentifier());
		}

		return;
	}

	IMovement::CrouchJump();
}

void CTF2BotMovement::BlastJumpTo(const Vector& start, const Vector& landingGoal, const Vector& forward)
{
	CTF2Bot* me = GetBot<CTF2Bot>();

	CBaseEntity* weapon = me->GetWeaponOfSlot(static_cast<int>(TeamFortress2::TFWeaponSlot::TFWeaponSlot_Primary));

	if (!weapon)
		return;

	me->SelectWeapon(weapon);

	m_bIsBlastJumping = true;
	m_blastJumpStart = me->GetAbsOrigin();
	m_blastJumpLandingGoal = landingGoal;
	m_blastJumpFireTimer.Invalidate();
	m_failTimer.Start(6.0f);

	me->GetControlInterface()->ReleaseCrouchButton();
	me->GetControlInterface()->ReleaseJumpButton();

	if (me->IsDebugging(BOTDEBUG_MOVEMENT))
	{
		me->DebugPrintToConsole(255, 255, 0, "%s BLAST JUMPING! LANDING: %3.2f %3.2f %3.2f \n", me->GetDebugIdentifier(), landingGoal.x, landingGoal.y, landingGoal.z);
		NDebugOverlay::Text(landingGoal, "BLAST JUMP TARGET", false, 5.0f);
	}
}

bool CTF2BotMovement::IsAbleToDoubleJump() const
{
	if (CTeamFortress2Mod::GetTF2Mod()->GetCurrentGameMode() == TeamFortress2::GameModeType::GM_VSH)
	{
		if (GetBot<CTF2Bot>()->IsSaxtonHale())
		{
			return true;
		}
	}

	auto cls = tf2lib::GetPlayerClassType(GetBot()->GetIndex());

	if (cls == TeamFortress2::TFClass_Scout)
	{
		return true;
	}

	return false;
}

bool CTF2BotMovement::IsAbleToBlastJump() const
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

bool CTF2BotMovement::IsAbleToUseGrapplingHook() const
{
	return GetBot<CTF2Bot>()->GetInventoryInterface()->GetTheGrapplingHook() != nullptr;
}

bool CTF2BotMovement::IsAbleToUseOffMeshConnection(OffMeshConnectionType type, const NavOffMeshConnection* connection) const
{
	switch (type)
	{
	case OffMeshConnectionType::OFFMESH_JUMP_OVER_GAP:
	{
		Vector forward = UtilHelpers::math::BuildDirectionVector(connection->GetStart(), connection->GetEnd());
		const bool needsdoublejump = GapJumpRequiresDoubleJump(connection->GetEnd(), forward);

		if (needsdoublejump && !IsAbleToDoubleJump())
		{
			return false;
		}

		return true;
	}
	default:
		return IMovement::IsAbleToUseOffMeshConnection(type, connection);
	}
}

bool CTF2BotMovement::GapJumpRequiresDoubleJump(const Vector& landing, const Vector& forward) const
{
	float length = (GetBot<CTF2Bot>()->GetAbsOrigin() - landing).Length();

	if (length >= scout_gap_jump_do_double_distance())
	{
		return true;
	}

	return false;
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

bool CTF2BotMovement::IsControllingMovements() const
{
	if (m_bIsBlastJumping)
	{
		return true;
	}

	if (m_bIsUsingGrapplingHook)
	{
		return true;
	}

	return IMovement::IsControllingMovements();
}

bool CTF2BotMovement::IsPathingAllowed() const
{
	if (m_bIsUsingGrapplingHook)
	{
		return false;
	}

	return IMovement::IsPathingAllowed();
}

bool CTF2BotMovement::NeedsWeaponControl() const
{
	if (m_bIsBlastJumping)
	{
		return true;
	}

	if (m_bIsUsingGrapplingHook)
	{
		return true;
	}

	return IMovement::NeedsWeaponControl();
}

bool CTF2BotMovement::UseGrapplingHook(const Vector& start, const Vector& end)
{
	CTF2Bot* me = GetBot<CTF2Bot>();
	CTF2BotInventory* inv = me->GetInventoryInterface();
	const CTF2BotWeapon* grapple = inv->GetTheGrapplingHook();

	if (!grapple)
	{
		return false;
	}

	m_grapplingHookGoal = end;
	m_bIsUsingGrapplingHook = true;

	const float range = (start - end).Length();
	constexpr float SPEED = 400.0f;
	constexpr float MIN_TIME = 7.0f;
	float time = range / SPEED;
	time = std::max(time, MIN_TIME);

	m_failTimer.Start(time);
	inv->EquipWeapon(grapple);

	if (me->IsDebugging(BOTDEBUG_MOVEMENT))
	{
		me->DebugPrintToConsole(0, 180, 0, "%s USE GRAPPLING HOOK (TRAVEL DISTANCE: %g)\n", me->GetDebugIdentifier(), range);
	}

	return true;
}

bool CTF2BotMovement::IsPathSegmentReached(const CMeshNavigator* nav, const BotPathSegment* goal, bool& resultoverride) const
{
	if (goal->type == AIPath::SegmentType::SEGMENT_GRAPPLING_HOOK && m_bIsUsingGrapplingHook)
	{
		resultoverride = true;
		return true;
	}


	return false;
}

bool CTF2BotMovement::DoRocketJumpAim()
{
	CTF2Bot* me = GetBot<CTF2Bot>();

	Vector origin = me->GetAbsOrigin();
	Vector to = (m_blastJumpLandingGoal - m_blastJumpStart);
	to.NormalizeInPlace();
	Vector goal = origin - (to * (GetHullWidth() * 0.10f));
	me->GetControlInterface()->SnapAimAt(goal, IPlayerController::LOOK_MOVEMENT);
	me->GetControlInterface()->AimAt(goal, IPlayerController::LOOK_MOVEMENT, 0.25f, "Looking for blast jump!");

#if SOURCE_ENGINE == SE_TF2
	return me->IsLookingTowards(goal, cvar_rj_dot.GetFloat());
#else
	return me->IsLookingTowards(goal, 0.060f);
#endif // SOURCE_ENGINE == SE_TF2
}

void CTF2BotMovement::BlastJumpUpdate()
{
#if SOURCE_ENGINE == SE_TF2
	CTF2Bot* me = GetBot<CTF2Bot>();

	if (m_failTimer.IsElapsed())
	{
		if (me->IsDebugging(BOTDEBUG_MOVEMENT))
		{
			me->DebugPrintToConsole(205, 92, 92, "%s BLAST JUMP TIMED OUT! \n", me->GetDebugIdentifier());
		}

		OnEndBlastJump();
	}

	MoveTowards(m_blastJumpLandingGoal, MOVEWEIGHT_PRIORITY);

	if (m_bBlastJumpAlreadyFired && me->GetAbsOrigin().z >= m_blastJumpLandingGoal.z - GetStepHeight())
	{
		float range = (me->GetAbsOrigin() - m_blastJumpLandingGoal).AsVector2D().Length();

		if (IsOnGround() || range <= 64.0f)
		{
			if (me->IsDebugging(BOTDEBUG_MOVEMENT))
			{
				me->DebugPrintToConsole(50, 205, 50, "%s BLAST JUMP COMPLETE! \n", me->GetDebugIdentifier());
			}

			OnEndBlastJump();
		}

		return;
	}

	if (!m_bBlastJumpAlreadyFired && GetGroundSpeed() >= 100.0f && IsOnGround())
	{
		if (!m_blastJumpFireTimer.HasStarted())
		{
			m_blastJumpFireTimer.Start(cvar_rj_fire_delay.GetFloat());
		}
		else if (m_blastJumpFireTimer.IsElapsed())
		{
			if (DoRocketJumpAim())
			{
				me->GetControlInterface()->PressJumpButton(0.5f);
				me->GetControlInterface()->PressCrouchButton(0.5f);
				me->GetControlInterface()->PressAttackButton();
				m_blastJumpFireTimer.Invalidate();
				m_bBlastJumpAlreadyFired = true;

				if (me->IsDebugging(BOTDEBUG_MOVEMENT))
				{
					me->DebugPrintToConsole(230, 230, 250, "%s BLAST JUMP: FIRING WEAPON! \n", me->GetDebugIdentifier());
				}
			}
		}
	}
#endif // SOURCE_ENGINE == SE_TF2
}

void CTF2BotMovement::OnEndBlastJump()
{
	m_bIsBlastJumping = false;
	m_bBlastJumpAlreadyFired = false;
	m_blastJumpFireTimer.Invalidate();
	m_failTimer.Invalidate();
}

void CTF2BotMovement::GrapplingHookUpdate()
{
	Vector lookAt = m_grapplingHookGoal;
	lookAt.z += GetStandingHullHeight() * 0.75f;

	CTF2Bot* me = GetBot<CTF2Bot>();
	CTF2BotPlayerController* input = me->GetControlInterface();
	CTF2BotInventory* inv = me->GetInventoryInterface();
	const CTF2BotWeapon* active = inv->GetActiveTFWeapon();
	const CTF2BotWeapon* grapple = inv->GetTheGrapplingHook();

	if (!grapple)
	{
		m_bIsUsingGrapplingHook = false;
	}

	if (active != grapple)
	{
		inv->EquipWeapon(grapple);
		return;
	}

	if (m_failTimer.IsElapsed())
	{
		if (me->IsDebugging(BOTDEBUG_MOVEMENT))
		{
			me->DebugPrintToConsole(255, 0, 0, "%s USING GRAPPLING HOOK! TIMED OUT!\n", me->GetDebugIdentifier());
		}

		m_bIsUsingGrapplingHook = false;
		m_failTimer.Invalidate();
	}

	input->AimAt(lookAt, IPlayerController::LOOK_MOVEMENT, 0.5f, "Looking at grappling hook goal!");

	const float tolerance = GetHullWidth() * 1.5f;

	if (input->IsAimOnTarget())
	{
		input->PressAttackButton(0.2f);
	}

	Vector origin = me->GetAbsOrigin();
	const float range = (origin - m_grapplingHookGoal).Length2D();

	if (me->IsDebugging(BOTDEBUG_MOVEMENT))
	{
		me->DebugPrintToConsole(220, 208, 255, "%s USING GRAPPLING HOOK! DISTANCE TO GOAL: %g\n", me->GetDebugIdentifier(), range);
	}

	if (range <= tolerance)
	{
		m_bIsUsingGrapplingHook = false;
		m_failTimer.Invalidate();
		input->ReleaseAllAttackButtons();
		inv->SelectBestWeapon();
	}
}
