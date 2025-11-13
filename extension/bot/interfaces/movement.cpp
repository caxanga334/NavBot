#include NAVBOT_PCH_FILE
#include <cstring>
#include <extension.h>
#include <sdkports/debugoverlay_shared.h>
#include <sdkports/sdk_entityoutput.h>
#include <bot/interfaces/playercontrol.h>
#include <bot/basebot.h>
#include <navmesh/nav_mesh.h>
#include <navmesh/nav_area.h>
#include <navmesh/nav_ladder.h>
#include <manager.h>
#include <mods/basemod.h>
#include <entities/baseentity.h>
#include <bot/interfaces/path/meshnavigator.h>
#include "movement.h"

#ifdef EXT_VPROF_ENABLED
#include <tier0/vprof.h>
#endif // EXT_VPROF_ENABLED

#ifdef EXT_DEBUG
static ConVar cvar_dev_jump_hold_time("sm_navbot_dev_jump_hold_time", "1.0", FCVAR_GAMEDLL);
#endif // EXT_DEBUG

static ConVar sm_navbot_movement_climb_dj_time("sm_navbot_movement_climb_dj_time", "0.3", FCVAR_GAMEDLL, "Delay to perform the double jump whem climbing.");
static ConVar sm_navbot_movement_gap_dj_time("sm_navbot_movement_gap_dj_time", "0.6", FCVAR_GAMEDLL, "Delay to perform the double jump whem jumping over a gap.");
static ConVar sm_navbot_movement_jump_cooldown("sm_navbot_movement_jump_cooldown", "0.1", FCVAR_GAMEDLL, "Cooldown between jumps.");
static ConVar sm_navbot_movement_catapult_speed("sm_navbot_movement_catapult_speed", "550.0", FCVAR_GAMEDLL, "Speed to use in catapult velocity correction.");
static ConVar sm_navbot_movement_strafejump_max_angle("sm_navbot_movement_strafe_jump_max_angle", "75.0", FCVAR_GAMEDLL, "Maximum strafe angle for strafe jumps.");
static ConVar sm_navbot_movement_strafejump_look_angle_offset("sm_navbot_movement_strafe_look_angle_offset", "15.0", FCVAR_GAMEDLL, "Angle offset for strafe jump look.");

CMovementTraverseFilter::CMovementTraverseFilter(CBaseBot* bot, IMovement* mover, const bool now) :
	trace::CTraceFilterSimple(bot->GetEntity(), mover->GetMovementCollisionGroup())
{
	m_me = bot;
	m_mover = mover;
	m_now = now;
}

bool CMovementTraverseFilter::ShouldHitEntity(IHandleEntity* pHandleEntity, int contentsMask)
{
	edict_t* pEdict = nullptr;
	CBaseEntity* pEntity = nullptr;
	int index = INVALID_EHANDLE_INDEX;

	trace::ExtractHandleEntity(pHandleEntity, &pEntity, &pEdict, index);

	if (pEntity == m_me->GetEntity())
	{
		return false; // Don't hit the bot itself
	}

	if (CTraceFilterSimple::ShouldHitEntity(pHandleEntity, contentsMask))
	{
		return !(m_mover->IsEntityTraversable(index, pEdict, pEntity, m_now));
	}

	return false;
}

CTraceFilterOnlyActors::CTraceFilterOnlyActors(CBaseEntity* pPassEnt, int collisionGroup) :
	trace::CTraceFilterSimple(pPassEnt, collisionGroup)
{
}

bool CTraceFilterOnlyActors::ShouldHitEntity(IHandleEntity* pHandleEntity, int contentsMask)
{
	if (trace::CTraceFilterSimple::ShouldHitEntity(pHandleEntity, contentsMask))
	{
		CBaseEntity* pEntity = trace::EntityFromEntityHandle(pHandleEntity);

		if (pEntity && UtilHelpers::IsPlayer(pEntity))
		{
			return true;
		}
	}

	return false;
}

IMovement::IMovement(CBaseBot* bot) : IBotInterface(bot)
{
	_Reset();
}

IMovement::~IMovement()
{
}

const char* IMovement::MovementRequestPriorityTypeToString(MovementRequestPriority type)
{
	using namespace std::literals::string_view_literals;

	constexpr std::array names = {
		"MOVEREQUEST_PRIO_LOW"sv,
		"MOVEREQUEST_PRIO_MEDIUM"sv,
		"MOVEREQUEST_PRIO_HIGH"sv,
		"MOVEREQUEST_PRIO_VERY_HIGH"sv,
		"MOVEREQUEST_PRIO_CRITICAL"sv,
		"MOVEREQUEST_PRIO_MANDATORY"sv,
	};

	static_assert(names.size() == static_cast<std::size_t>(MovementRequestPriority::MAX_MOVEREQUEST_PRIO_TYPES), "Movement Request Priority name array and enum count mismatch!");

	return names[static_cast<std::size_t>(type)].data();
}

const char* IMovement::MovementTypeToString(MovementType type)
{
	using namespace std::literals::string_view_literals;

	constexpr std::array names = {
		"WALKING"sv,
		"RUNNING"sv,
		"SPRINTING"sv,
	};

	static_assert(names.size() == static_cast<std::size_t>(MovementType::MAX_MOVEMENT_TYPES), "Movement Type name array and enum count mismatch!");

	return names[static_cast<std::size_t>(type)].data();
}

bool IMovement::InitializeGameData(SourceMod::IGameConfig* cfgnavbot)
{
	const char* value = cfgnavbot->GetKeyValue("PlayerHull_StandHeight");
	if (value) { IMovement::s_playerhull.stand_height = atof(value); }

	value = cfgnavbot->GetKeyValue("PlayerHull_CrouchHeight");
	if (value) { IMovement::s_playerhull.crouch_height = atof(value); }

	value = cfgnavbot->GetKeyValue("PlayerHull_ProneHeight");
	if (value) { IMovement::s_playerhull.prone_height = atof(value); }

	value = cfgnavbot->GetKeyValue("PlayerHull_Width");
	if (value) { IMovement::s_playerhull.width = atof(value); }

	return true;
}

void IMovement::Reset()
{
	_Reset();
}

void IMovement::Update()
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("IMovement::Update", "NavBot");
#endif // EXT_VPROF_ENABLED

	CBaseBot* me = GetBot<CBaseBot>();
	m_maxspeed = me->GetMaxSpeed();

	StuckMonitor();
	ObstacleBreakUpdate();
	TraverseLadder();
	ElevatorUpdate();
	StrafeJumpUpdate();

	Vector velocity = me->GetAbsVelocity();
	m_speed = velocity.Length();
	m_groundspeed = velocity.AsVector2D().Length();

	constexpr auto MIN_SPEED = 10.0f;

	if (m_speed > MIN_SPEED)
	{
		m_motionVector = velocity / m_speed;
	}

	if (m_groundspeed > MIN_SPEED)
	{
		m_groundMotionVector.x = velocity.x / m_speed;
		m_groundMotionVector.y = velocity.y / m_speed;
	}

	// do this after the speeds calculations
	UpdateMovementButtons();

	if (IsUsingLadder() || IsUsingElevator() || IsStrafeJumping())
	{
		return;
	}

	if (m_doMidAirCJ.HasStarted() && m_doMidAirCJ.IsElapsed())
	{
		CrouchJump();
		m_doMidAirCJ.Invalidate();

		if (me->IsDebugging(BOTDEBUG_MOVEMENT))
		{
			me->DebugPrintToConsole(255, 255, 0, "%s MID AIR CROUCH JUMP! \n", me->GetDebugIdentifier());
		}
	}

	if (m_isUsingCatapult)
	{
		if (!m_isAirborne)
		{
			// These require a precise aligment and movement (depends on the map).
			// To make things easier for bots and for nav mesh editing, we cheat a bit by correcting the bot's velocity mid flight.
			m_catapultCorrectVelocityTimer.Start(0.75f);

			if (!IsOnGround())
			{
				m_isAirborne = true;

				if (me->IsDebugging(BOTDEBUG_MOVEMENT))
				{
					me->DebugPrintToConsole(135, 206, 250, "%s CATAPULT AIRBORNE! \n", me->GetDebugIdentifier());
					const ICollideable* collider = me->GetCollideable();
					debugoverlay->AddBoxOverlay(collider->GetCollisionOrigin(), collider->OBBMins(), collider->OBBMaxs(), collider->GetCollisionAngles(), 135, 206, 250, 180, 15.0f);
					debugoverlay->AddLineOverlay(collider->GetCollisionOrigin(), m_landingGoal, 135, 206, 250, true, 15.0f);
				}

				return;
			}

			// move towards the start position
			MoveTowards(m_catapultStartPosition, MOVEWEIGHT_CRITICAL);
		}
		else
		{
			if (IsOnGround())
			{
				// Landed, assume the destination was reached.
				m_isUsingCatapult = false;
				m_isAirborne = false;

				if (me->IsDebugging(BOTDEBUG_MOVEMENT))
				{
					me->DebugPrintToConsole(135, 206, 250, "%s CATAPULT COMPLETE! DISTANCE FROM LANDING GOAL: %g \n", me->GetDebugIdentifier(), me->GetRangeTo(m_landingGoal));
				}

				// force an update
				me->UpdateLastKnownNavArea(true);
				CMeshNavigator* nav = me->GetActiveNavigator();

				if (nav)
				{
					// Refresh the goal position so the bot doesn't try to walk back to the jump start
					nav->AdvanceGoalToNearest();
				}
			}
			else
			{
				if (m_catapultCorrectVelocityTimer.HasStarted() && m_catapultCorrectVelocityTimer.IsElapsed())
				{
					m_catapultCorrectVelocityTimer.Invalidate();
					Vector velocity = me->CalculateLaunchVector(m_landingGoal, sm_navbot_movement_catapult_speed.GetFloat());
					me->SetAbsVelocity(velocity);
				}

				// air strafe towards it
				MoveTowards(m_landingGoal, MOVEWEIGHT_CRITICAL);
				me->GetControlInterface()->AimAt(m_landingGoal, IPlayerController::LOOK_MOVEMENT, 0.2f, "Looking at catapult landing position!");
			}
		}

		return; // block everything below
	}

	// the code below should not run when the bot is using ladders

	if (m_isJumpingAcrossGap || m_isClimbingObstacle)
	{
		Vector toLanding = m_landingGoal - me->GetAbsOrigin();
		toLanding.z = 0.0f;
		toLanding.NormalizeInPlace();

		if (m_isAirborne)
		{
			me->GetControlInterface()->AimAt(me->GetEyeOrigin() + 100.0f * toLanding, IPlayerController::LOOK_MOVEMENT, 0.1f);

			if (IsOnGround())
			{
				OnJumpComplete();
			}
		}
		else // not airborne, jump!
		{
			if (!IsClimbingOrJumping())
			{
				CrouchJump();
			}

			if (m_isJumpingAcrossGap && GetRunSpeed() > 1.0f)
			{
				Vector velocity = me->GetAbsVelocity();

				// Cheat: helps with aligment
				velocity.x = GetRunSpeed() * toLanding.x;
				velocity.y = GetRunSpeed() * toLanding.y;

				me->SetAbsVelocity(velocity);
			}

			if (!IsOnGround())
			{
				m_isAirborne = true;
			}
		}

		MoveTowards(m_landingGoal, MOVEWEIGHT_CRITICAL);
	}
	else if (m_isJumping) // for simple jumps
	{
		if (m_isAirborne || m_jumpTimer.IsElapsed())
		{
			if (IsOnGround())
			{
				OnJumpComplete();
			}
		}
		else
		{
			if (!IsOnGround())
			{
				m_isAirborne = true;
			}
		}
	}

	if (m_counterStrafeTimer.HasStarted() && !m_counterStrafeTimer.IsElapsed())
	{
		constexpr auto DISABLE_COUNTERSTRAFE_SPEED = 32.0f;

		if (m_groundspeed <= DISABLE_COUNTERSTRAFE_SPEED)
		{
			m_counterStrafeTimer.Invalidate();
		}
		else
		{
			Vector forward = UtilHelpers::GetNormalizedVector(velocity);
			Vector strafeTo = me->GetAbsOrigin();
			strafeTo += (forward * -256.0f);
			MoveTowards(strafeTo, MOVEWEIGHT_COUNTERSTRAFE);
		}
	}

	if (m_isStopAndWait)
	{
		DoCounterStrafe(); // counter strafe to cancel any movement speed

		if (m_stopAndWaitTimer.IsElapsed())
		{
			m_stopAndWaitTimer.Invalidate();
			m_isStopAndWait = false;
		}
	}
}

void IMovement::Frame()
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("IMovement::Frame", "NavBot");
#endif // EXT_VPROF_ENABLED
}

float IMovement::GetHullWidth() const
{
	float scale = GetBot()->GetModelScale();
	return IMovement::s_playerhull.width * scale;
}

float IMovement::GetStandingHullHeight() const
{
	float scale = GetBot()->GetModelScale();
	return IMovement::s_playerhull.stand_height * scale;
}

float IMovement::GetCrouchedHullHeight() const
{
	float scale = GetBot()->GetModelScale();
	return IMovement::s_playerhull.crouch_height * scale;
}

float IMovement::GetProneHullHeight() const
{
	float scale = GetBot()->GetModelScale();
	return IMovement::s_playerhull.prone_height * scale;
}

unsigned int IMovement::GetMovementTraceMask() const
{
	return MASK_PLAYERSOLID;
}

int IMovement::GetMovementCollisionGroup() const
{
	return static_cast<int>(Collision_Group_t::COLLISION_GROUP_NONE);
}

float IMovement::GetDesiredSpeed() const
{
	return m_desiredspeed;
}

void IMovement::SetDesiredSpeed(float speed)
{
	m_desiredspeed = std::clamp(speed, 0.0f, m_maxspeed);
}

void IMovement::MoveTowards(const Vector& pos, const int weight)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("IMovement::MoveTowards", "NavBot");
#endif // EXT_VPROF_ENABLED

	if (m_lastMoveWeight > weight)
	{
		return;
	}

	m_lastMoveWeight = weight;

	CBaseBot* me = GetBot<CBaseBot>();
	IPlayerController* input = me->GetControlInterface();
	Vector origin = me->GetAbsOrigin();
	Vector eyeforward;
	me->EyeVectors(&eyeforward);
	m_stuck.OnMovementRequested();

	Vector2D forward(eyeforward.x, eyeforward.y);
	forward.NormalizeInPlace();
	Vector2D right(forward.y, -forward.x);

	// Build direction vector towards goal
	Vector2D to = (pos - me->GetAbsOrigin()).AsVector2D();
	float distancetogoal = to.NormalizeInPlace();

	float ahead = to.Dot(forward);
	float side = to.Dot(right);

	constexpr float epsilon = 0.25f;

	if (me->IsDebugging(BOTDEBUG_MOVEMENT))
	{
		NDebugOverlay::Line(me->WorldSpaceCenter(), pos, 0, 255, 0, false, 0.2f);
	}

	if (me->IsUnderWater())
	{
		// While underwater, use move up/down to adjust the bot's height

		/* const float distance2d = (origin - pos).AsVector2D().Length(); */
		const float heightdiff = std::abs(origin.z - pos.z);
		
		if (/* distance2d <= GetHullWidth() * 1.2f && */ heightdiff > GetStepHeight())
		{
			if (pos.z > origin.z)
			{
				input->PressMoveDownButton(0.2f);
			}
			else
			{
				input->PressMoveUpButton(0.2f);
			}
		}
	}

	// handle ladder movement
	if (IsOnLadder() && IsUsingLadder() && (m_ladderState == USING_LADDER_UP || m_ladderState == USING_LADDER_DOWN))
	{
		// bot is on a ladder and wants to use it
		input->PressForwardButton();

		if (m_ladder != nullptr)
		{
			Vector posOnLadder;
			CalcClosestPointOnLine(origin, m_ladder->m_bottom, m_ladder->m_top, posOnLadder);

			Vector alongLadder = m_ladder->m_top - m_ladder->m_bottom;
			alongLadder.NormalizeInPlace();

			Vector rightLadder = CrossProduct(alongLadder, m_ladder->GetNormal());
			Vector away = origin - posOnLadder;

			const float error = DotProduct(away, rightLadder);
			away.NormalizeInPlace();

			const float tolerance = 5.0f + 0.25f * GetHullWidth();

			if (error > tolerance)
			{
				if (DotProduct(away, rightLadder) > 0.0f)
				{
					input->PressMoveLeftButton();
				}
				else
				{
					input->PressMoveRightButton();
				}
			}
		}
	}
	else
	{
		if (ahead > epsilon)
		{
			input->PressForwardButton();
		}
		else if (ahead < -epsilon)
		{
			input->PressBackwardsButton();
		}

		if (side <= -epsilon)
		{
			input->PressMoveLeftButton();
		}
		else if (side >= epsilon)
		{
			input->PressMoveRightButton();
		}
	}
}

void IMovement::AccelerateTowards(const Vector& pos, const float* speed, const float* delta, const int weight)
{
	if (m_lastMoveWeight > weight)
	{
		return;
	}

	// call move towards to setup button presses
	MoveTowards(pos, weight);

	CBaseBot* me = GetBot<CBaseBot>();

	const float accelspeed = speed != nullptr ? *speed : GetRunSpeed();
	const float deltaT = delta != nullptr ? *delta : me->GetLastUpdateTimeDelta();

	Vector velocity = me->GetAbsVelocity();
	Vector dir = UtilHelpers::math::BuildDirectionVector(me->GetAbsOrigin(), pos);
	dir.z = 0.0f;
	velocity += (dir * deltaT);
	me->SetAbsVelocity(velocity);
}

void IMovement::FaceTowards(const Vector& pos, const bool important)
{
	auto input = GetBot()->GetControlInterface();
	auto eyes = GetBot()->GetEyeOrigin();
	IPlayerController::LookPriority priority = important ? IPlayerController::LookPriority::LOOK_MOVEMENT : IPlayerController::LookPriority::LOOK_IDLE;

	Vector lookAt(pos.x, pos.y, eyes.z);
	input->AimAt(lookAt, priority, 0.1f, "IMovement::FaceTowards");
}

void IMovement::AdjustSpeedForPath(CMeshNavigator* path)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("IMovement::AdjustSpeedForPath", "NavBot");
#endif // EXT_VPROF_ENABLED

	auto& data = path->GetCursorData();
	auto goal = path->GetGoalSegment();

	if (GetBot()->IsUnderWater())
	{
		SetDesiredSpeed(GetRunSpeed());
		return;
	}

	if (!goal)
	{
		SetDesiredSpeed(GetRunSpeed());
		return;
	}

	auto next = path->GetNextSegment(goal);

	if (goal->type == AIPath::SEGMENT_LADDER_UP || goal->type == AIPath::SEGMENT_LADDER_DOWN)
	{
		SetDesiredSpeed(GetWalkSpeed());
		return;
	}

	if (goal->type == AIPath::SegmentType::SEGMENT_JUMP_OVER_GAP || goal->type == AIPath::SegmentType::SEGMENT_BLAST_JUMP ||
		goal->type == AIPath::SegmentType::SEGMENT_CLIMB_UP || 
		goal->type == AIPath::SegmentType::SEGMENT_CLIMB_DOUBLE_JUMP || (next && next->type == AIPath::SegmentType::SEGMENT_JUMP_OVER_GAP))
	{
		// on these conditions, always move at max speed
		SetDesiredSpeed(GetRunSpeed());
		return;
	}

	// adjust speed based on the path's curvature
	SetDesiredSpeed(GetRunSpeed() + fabs(data.curvature) * (GetWalkSpeed() - GetRunSpeed()));
}

void IMovement::DetermineIdealPostureForPath(const CMeshNavigator* path)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("IMovement::DetermineIdealPostureForPath", "NavBot");
#endif // EXT_VPROF_ENABLED

	CBaseBot* bot = GetBot<CBaseBot>();

	CNavArea* lastarea = bot->GetLastKnownNavArea();

	if (lastarea && lastarea->HasAttributes(static_cast<int>(NavAttributeType::NAV_MESH_CROUCH)))
	{
		bot->GetControlInterface()->PressCrouchButton(0.25f);
		return;
	}

	constexpr auto GOAL_CROUCH_RANGE = 50.0f * 50.0f;
	const CBasePathSegment* goal = path->GetGoalSegment();

	if (goal->area->HasAttributes(static_cast<int>(NavAttributeType::NAV_MESH_CROUCH)) /* && GetBot<CBaseBot>()->GetRangeToSqr(goal->goal) <= GOAL_CROUCH_RANGE */)
	{
		bot->GetControlInterface()->PressCrouchButton(0.25f);
		return;
	}
}

void IMovement::Stop()
{
	auto input = GetBot()->GetControlInterface();
	input->ReleaseForwardButton();
	input->ReleaseBackwardsButton();
	input->ReleaseMoveLeftButton();
	input->ReleaseMoveRightButton();
}

void IMovement::ClimbLadder(const CNavLadder* ladder, CNavArea* dismount)
{
#ifdef EXT_DEBUG
	if (m_ladderState != NOT_USING_LADDER)
	{
		Warning("IMovement::ClimbLadder called while climbing a ladder!\n");
	}
#endif // EXT_DEBUG

	m_ladder = ladder;
	m_ladderExit = dismount;
	m_wasLaunched = false;
	ChangeLadderState(LadderState::APPROACHING_LADDER_UP);

	CBaseBot* bot = GetBot<CBaseBot>();
	
	if (bot->IsDebugging(BOTDEBUG_MOVEMENT))
	{
		bot->DebugPrintToConsole(BOTDEBUG_MOVEMENT, 200, 0, 200, "%s CLIMBING LADDER! \n", bot->GetDebugIdentifier());

		m_ladderExit->DrawFilled(255, 0, 255, 255, 5.0f, true);
		NDebugOverlay::Text(m_ladderExit->GetCenter() + Vector(0.0f, 0.0f, navgenparams->step_height), "DISMOUNT AREA!", false, 5.0f);
	}
}

void IMovement::DescendLadder(const CNavLadder* ladder, CNavArea* dismount)
{
	m_ladder = ladder;
	m_ladderExit = dismount;
	m_wasLaunched = false;
	ChangeLadderState(LadderState::APPROACHING_LADDER_DOWN);

	auto bot = GetBot();

	if (bot->IsDebugging(BOTDEBUG_MOVEMENT))
	{
		bot->DebugPrintToConsole(BOTDEBUG_MOVEMENT, 200, 0, 200, "%s DESCENDING LADDER! \n", bot->GetDebugIdentifier());

		m_ladderExit->DrawFilled(255, 0, 255, 255, 5.0f, true);
		NDebugOverlay::Text(m_ladderExit->GetCenter() + Vector(0.0f, 0.0f, navgenparams->step_height), "DISMOUNT AREA!", false, 5.0f);
	}
}

void IMovement::Jump()
{
	if (!m_jumpCooldown.IsElapsed())
		return;

	CBaseBot* me = GetBot<CBaseBot>();

	// Cannot start a jump while holding the jump button
	if (IsOnGround() && me->GetControlInterface()->IsPressingJumpButton())
	{
		m_jumpCooldown.Start(sm_navbot_movement_jump_cooldown.GetFloat());
		me->GetControlInterface()->ReleaseJumpButton();

		if (me->IsDebugging(BOTDEBUG_MOVEMENT))
		{
			me->DebugPrintToConsole(255, 0, 0, "%s REJECTING JUMP: JUMP KEY IS CURRENTLY BEING PRESSED! \n", me->GetDebugIdentifier());
		}

		return;
	}

	m_isJumping = true;
	m_jumpTimer.Start(0.5f);

	me->GetControlInterface()->PressJumpButton();
}

void IMovement::CrouchJump()
{
	if (!m_jumpCooldown.IsElapsed())
	{
		if (GetBot<CBaseBot>()->IsDebugging(BOTDEBUG_MOVEMENT))
		{
			GetBot<CBaseBot>()->DebugPrintToConsole(255, 0, 0, "%s CROUCH JUMP REJECTED: JUMP COOLDOWN \n", GetBot<CBaseBot>()->GetDebugIdentifier());
		}

		return;
	}

	CBaseBot* me = GetBot<CBaseBot>();

	// Cannot jump while holding the jump button
	if (IsOnGround() && me->GetControlInterface()->IsPressingJumpButton())
	{
		m_jumpCooldown.Start(0.5f);
		me->GetControlInterface()->ReleaseJumpButton();
		me->GetControlInterface()->ReleaseCrouchButton();

		if (me->IsDebugging(BOTDEBUG_MOVEMENT))
		{
			me->DebugPrintToConsole(255, 0, 0, "%s REJECTING CROUCH JUMP: JUMP KEY IS CURRENTLY BEING PRESSED! \n", me->GetDebugIdentifier());
		}

		return;
	}

	m_isJumping = true;
	m_jumpTimer.Start(0.5f);

#ifdef EXT_DEBUG
	me->GetControlInterface()->PressCrouchButton(cvar_dev_jump_hold_time.GetFloat());
	me->GetControlInterface()->PressJumpButton(cvar_dev_jump_hold_time.GetFloat());
#else
	me->GetControlInterface()->PressCrouchButton(1.0f);
	me->GetControlInterface()->PressJumpButton(1.0f);
#endif // EXT_DEBUG
}

void IMovement::DoubleJump()
{
	if (!m_jumpCooldown.IsElapsed())
		return;

	CBaseBot* me = GetBot<CBaseBot>();

	// Cannot jump while holding the jump button
	if (IsOnGround() && me->GetControlInterface()->IsPressingJumpButton())
	{
		m_jumpCooldown.Start(sm_navbot_movement_jump_cooldown.GetFloat());
		me->GetControlInterface()->ReleaseJumpButton();

		if (me->IsDebugging(BOTDEBUG_MOVEMENT))
		{
			me->DebugPrintToConsole(255, 0, 0, "%s REJECTING DOUBLE JUMP: JUMP KEY IS CURRENTLY BEING PRESSED! \n", me->GetDebugIdentifier());
		}

		return;
	}

	m_isJumping = true;
	m_jumpTimer.Start(1.0f);

	GetBot()->GetControlInterface()->PressJumpButton(0.1f); // simple jump first

	m_doMidAirCJ.Start(sm_navbot_movement_climb_dj_time.GetFloat());
}

/**
 * @brief Makes the bot jump over a hole on the ground
 * @param landing jump landing target
 * @param forward jump unit vector
*/
void IMovement::JumpAcrossGap(const Vector& landing, const Vector& forward)
{
	// look towards the jump target
	GetBot()->GetControlInterface()->SnapAimAt(landing, IPlayerController::LOOK_MOVEMENT);
	GetBot()->GetControlInterface()->AimAt(landing, IPlayerController::LOOK_MOVEMENT, 1.0f, "Looking at jump over gap landing!");

	if (IsAbleToDoubleJump() && GapJumpRequiresDoubleJump(landing, forward))
	{
		Jump();
		
		m_doMidAirCJ.Start(sm_navbot_movement_gap_dj_time.GetFloat());

		if (GetBot()->IsDebugging(BOTDEBUG_MOVEMENT))
		{
			GetBot<CBaseBot>()->DebugPrintToConsole(70, 130, 180, "%s JUMP ACROSS GAP REQUIRES DOUBLE JUMP \n", GetBot<CBaseBot>()->GetDebugIdentifier());
		}
	}
	else
	{
		CrouchJump();
	}

	m_isJumpingAcrossGap = true;
	m_isClimbingObstacle = false;
	m_landingGoal = landing;
	m_isAirborne = false;

	if (GetBot()->IsDebugging(BOTDEBUG_MOVEMENT))
	{
		Vector top = GetBot()->GetAbsOrigin();
		top.z += GetMaxJumpHeight();

		NDebugOverlay::VertArrow(GetBot()->GetAbsOrigin(), top, 5.0f, 0, 0, 255, 255, false, 5.0f);
		NDebugOverlay::HorzArrow(top, landing, 5.0f, 0, 255, 0, 255, false, 5.0f);

		GetBot<CBaseBot>()->DebugPrintToConsole(70, 130, 180, "%s JUMP ACROSS GAP AT %3.2f %3.2f %3.2f \n", GetBot<CBaseBot>()->GetDebugIdentifier(), landing.x, landing.y, landing.z);
	}
}

bool IMovement::ClimbUpToLedge(const Vector& landingGoal, const Vector& landingForward, edict_t* obstacle)
{
	CBaseBot* me = GetBot<CBaseBot>();

	// 60 degress tolerance ( t = cos(deg2rad(60)) )
	if (!me->IsMovingTowards2D(landingGoal, 0.5f))
	{
		// Cheat: bot is not aliged, manually correct their velocity.
		Vector velocity = me->GetAbsVelocity();
		velocity.x = GetRunSpeed() * landingForward.x;
		velocity.y = GetRunSpeed() * landingForward.y;
		me->SetAbsVelocity(velocity);

		if (me->IsDebugging(BOTDEBUG_MOVEMENT))
		{
			me->DebugPrintToConsole(250, 128, 114, "%s CLIMB UP TO LEDGE: BOT MOTION VECTOR WAS NOT ALIGNED WITH JUMP GOAL. MANUALLY ALIGNED! \n", me->GetDebugIdentifier());
		}
	}

	CrouchJump(); // Always do a crouch jump
	MoveTowards(landingGoal, MOVEWEIGHT_STANDARD_JUMPS);

	m_isClimbingObstacle = true;
	m_isJumpingAcrossGap = false;
	m_landingGoal = landingGoal;
	m_isAirborne = false;

	// low priority look
	me->GetControlInterface()->AimAt(landingGoal, IPlayerController::LOOK_INTERESTING, 0.5f, "Looking at ledge to climb!");

	if (me->IsDebugging(BOTDEBUG_MOVEMENT))
	{
		me->DebugPrintToConsole(70, 130, 180, "%s CLIMB UP TO LEDGE AT %3.2f %3.2f %3.2f \n", me->GetDebugIdentifier(), landingGoal.x, landingGoal.y, landingGoal.z);
	}

	return true;
}

bool IMovement::DoubleJumpToLedge(const Vector& landingGoal, const Vector& landingForward, edict_t* obstacle)
{
	if (!IsAbleToDoubleJump())
		return false;

	DoubleJump();

	m_isClimbingObstacle = true;
	m_isJumpingAcrossGap = false;
	m_landingGoal = landingGoal;
	m_isAirborne = false;

	// The second jump direction generally requires looking at the direction you want to move
	GetBot<CBaseBot>()->GetControlInterface()->AimAt(landingGoal, IPlayerController::LOOK_MOVEMENT, 0.2f, "Looking at ledge to climb!");

	if (GetBot<CBaseBot>()->IsDebugging(BOTDEBUG_MOVEMENT))
	{
		GetBot<CBaseBot>()->DebugPrintToConsole(70, 130, 180, "%s DOUBLE JUMP TO LEDGE AT %3.2f %3.2f %3.2f \n", GetBot<CBaseBot>()->GetDebugIdentifier(), landingGoal.x, landingGoal.y, landingGoal.z);
	}

	return true;
}

bool IMovement::IsAbleToClimbOntoEntity(edict_t* entity) const
{
	if (entity == nullptr)
	{
		return false;
	}

	if (UtilHelpers::FClassnameIs(entity, "func_door*") == true || UtilHelpers::FClassnameIs(entity, "prop_door*") == true)
	{
		return false; // never climb doors
	}

	return true;
}

bool IMovement::IsAbleToUseOffMeshConnection(OffMeshConnectionType type, const NavOffMeshConnection* connection) const
{
	switch (type)
	{
	case OffMeshConnectionType::OFFMESH_DOUBLE_JUMP:
		return IsAbleToDoubleJump();
	case OffMeshConnectionType::OFFMESH_BLAST_JUMP:
		return IsAbleToBlastJump();
	case OffMeshConnectionType::OFFMESH_GRAPPLING_HOOK:
		return IsAbleToUseGrapplingHook();
	case OffMeshConnectionType::OFFMESH_STRAFE_JUMP:
		return IsAbleToStrafeJump();
	default:
		return true;
	}
}

bool IMovement::IsAscendingOrDescendingLadder()
{
	switch (m_ladderState)
	{
	case IMovement::EXITING_LADDER_UP:
		[[fallthrough]];
	case IMovement::USING_LADDER_UP:
		[[fallthrough]];
	case IMovement::EXITING_LADDER_DOWN:
		[[fallthrough]];
	case IMovement::USING_LADDER_DOWN:
		return true;
	default:
		return false;
	}
}

bool IMovement::IsOnLadder()
{
	return GetBot()->GetMoveType() == MOVETYPE_LADDER;
}

bool IMovement::IsGap(const Vector& pos, const Vector& forward)
{
	trace_t result;
	trace::CTraceFilterNoNPCsOrPlayers filter(GetBot()->GetEntity(), COLLISION_GROUP_NONE);
	Vector start = pos + Vector(0.0f, 0.0f, GetStepHeight());
	Vector end = pos + Vector(0.0f, 0.0f, -GetMaxJumpHeight());
	trace::line(start, end, GetMovementTraceMask(), &filter, result);
	return result.fraction >= 1.0f && !result.startsolid;
}

/**
 * @brief Checks if the bot is able to move between 'from' and 'to'
 * @param from Start position
 * @param to End position
 * @param fraction trace result fraction
 * @param now When true, check if the bot is able to move right now. Otherwise check if the bot is able to move in the future 
 * (ie: blocked by an entity that can be destroyed)
 * @param obstacle The obstacle entity will be stored here.
 * @return true if the bot can walk, false otherwise
*/
bool IMovement::IsPotentiallyTraversable(const Vector& from, const Vector& to, float* fraction, const bool now, CBaseEntity** obstacle)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("IMovement::IsPotentiallyTraversable", "NavBot");
#endif // EXT_VPROF_ENABLED

	float heightdiff = to.z - from.z;
	float jumplimit = GetMaxJumpHeight();

	if (GetMaxDoubleJumpHeight() > jumplimit && IsAbleToDoubleJump())
	{
		jumplimit = GetMaxDoubleJumpHeight();
	}

	if (heightdiff >= jumplimit)
	{
		Vector along = to - from;
		along.NormalizeInPlace();
		if (along.z > GetTraversableSlopeLimit()) // too high, can't climb this ramp
		{
			if (fraction != nullptr)
			{
				*fraction = 0.0f;
			}

			return false;
		}
	}

	trace_t result;
	CMovementTraverseFilter filter(GetBot(), this, now);
	const float hullsize = GetHullWidth() * 0.25f; // use a smaller hull since we won't resolve collisions now
	const float height = GetStepHeight();
	Vector mins(-hullsize, -hullsize, height);
	Vector maxs(hullsize, hullsize, GetCrouchedHullHeight());
	trace::hull(from, to, mins, maxs, GetMovementTraceMask(), &filter, result);

	if (GetBot()->IsDebugging(BOTDEBUG_MOVEMENT))
	{
		if (result.DidHit())
		{
			NDebugOverlay::SweptBox(from, to, mins, maxs, vec3_angle, 255, 0, 0, 255, 0.5f);
			NDebugOverlay::Sphere(result.endpos, 24.0f, 255, 0, 255, true, 0.5f);

			auto index = result.GetEntityIndex();

			if (index != -1)
			{
				auto classname = gamehelpers->GetEntityClassname(result.m_pEnt);
				char message[256];
				ke::SafeSprintf(message, sizeof(message), "Hit Entity %s#%i", classname, index);

				NDebugOverlay::Text(result.endpos, message, false, 0.5f);
				DevMsg("%s IMovement::IsPotentiallyTraversable \n  %s \n", GetBot()->GetDebugIdentifier(), message);
			}
		}
	}

	if (fraction != nullptr)
	{
		*fraction = result.fraction;
	}

	if (obstacle != nullptr)
	{
		*obstacle = result.m_pEnt;
	}

	return result.fraction >= 1.0f && !result.startsolid;
}

bool IMovement::HasPotentialGap(const Vector& from, const Vector& to, float* fraction)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("IMovement::HasPotentialGap", "NavBot");
#endif // EXT_VPROF_ENABLED

	// get movement fraction
	float traversableFraction = 0.0f;
	IsPotentiallyTraversable(from, to, &traversableFraction, true);

	Vector end = from + (to - from) * traversableFraction;
	Vector forward = to - from;
	const float length = forward.NormalizeInPlace();
	const float step = GetHullWidth() * 0.5f;
	Vector start = from;
	Vector delta = step * forward;

	for (float f = 0.0f; f < (length + step); f += step)
	{
		if (IsGap(start, forward))
		{
			if (fraction)
			{
				*fraction = (f - step) / (length + step);
			}

			return true;
		}

		start += delta;
	}

	if (fraction)
	{
		*fraction = 1.0f;
	}

	return false;
}

bool IMovement::IsEntityTraversable(int index, edict_t* edict, CBaseEntity* entity, const bool now)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("IMovement::IsEntityTraversable", "NavBot");
#endif // EXT_VPROF_ENABLED

	if (index == 0) // index 0 is the world
	{
		return false;
	}

	if (UtilHelpers::IsPlayerIndex(index))
	{
		return true; // assume players are walkable since they will either move out of the way or get killed
	}

	if (UtilHelpers::FClassnameIs(entity, "func_door*") == true)
	{
		return true;
	}

	if (UtilHelpers::FClassnameIs(entity, "prop_door*") == true)
	{
		int doorstate = 0;
		if (!entprops->GetEntProp(index, Prop_Data, "m_eDoorState", doorstate))
		{
			return true; // lookup failed, assume it's walkable
		}

		if (doorstate == static_cast<int>(sdkdefs::DoorState_t::DOOR_STATE_OPEN))
		{
			return false; // open doors are obstacles
		}
	}

	if (UtilHelpers::FClassnameIs(entity, "func_brush") == true)
	{
		entities::HFuncBrush brush(gamehelpers->EdictOfIndex(index));
		auto solidity = brush.GetSolidity();
		
		switch (solidity)
		{
		case entities::HFuncBrush::BRUSHSOLID_TOGGLE:
			[[fallthrough]];
		case entities::HFuncBrush::BRUSHSOLID_NEVER:
			return true;
		case entities::HFuncBrush::BRUSHSOLID_ALWAYS:
		default:
			return false;
		}
	}

	if (now == true)
	{
		return false;
	}

	return GetBot()->IsAbleToBreak(entity);
}

bool IMovement::IsOnGround()
{
	// ground ent was not being realiable for some reason.
	// return GetBot()->GetGroundEntity() != nullptr;
	// TO-DO: See if it's worth to cache the m_fFlags pointer
	int flags = 0;
	entprops->GetEntProp(GetBot<CBaseBot>()->GetEntity(), Prop_Send, "m_fFlags", flags);
	return (flags & FL_ONGROUND) != 0;
}

bool IMovement::IsCompletelyCrouched() const
{
	// m_bDucked seems reliable, at least on the modern SDK 2013
	// Could try changing to checking FL_DUCKING flag

	bool result = false;
	entprops->GetEntPropBool(GetBot<CBaseBot>()->GetEntity(), Prop_Send, "m_bDucked", result);
	return false;
}

bool IMovement::IsInCrouchTransition() const
{
	bool result = false;
	entprops->GetEntPropBool(GetBot<CBaseBot>()->GetEntity(), Prop_Send, "m_bDucking", result);
	return false;
}

/**
 * @brief Checks if a request for movement has been made recently
 * @param time Max time in seconds to consider a movement request
 * @return true if a request was made, false otherwise
*/
bool IMovement::IsAttemptingToMove(const float time) const
{
	return m_stuck.movementrequesttimer.HasStarted() && m_stuck.movementrequesttimer.IsLessThen(time);
}

float IMovement::GetStuckDuration()
{
	if (m_stuck.isstuck)
	{
		return m_stuck.stucktimer.GetElapsedTime();
	}

	return 0.0f;
}

void IMovement::ClearStuckStatus(const char* reason)
{
	m_stuck.ClearStuck(GetBot()->GetAbsOrigin());

	if (GetBot()->IsDebugging(BOTDEBUG_MOVEMENT))
	{
		GetBot()->DebugPrintToConsole(BOTDEBUG_MOVEMENT, 255, 200, 200, "ClearStuckStatus: %s \n", reason ? reason : "");
	}
}

bool IMovement::IsAreaTraversable(const CNavArea* area) const
{
	auto bot = GetBot();

	if (area->IsBlocked(bot->GetCurrentTeamIndex()))
	{
		return false;
	}

	return true;
}

void IMovement::AdjustPathCrossingPoint(const CNavArea* fromArea, const CNavArea* toArea, const Vector& fromPos, Vector* crosspoint)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("IMovement::AdjustPathCrossingPoint", "NavBot");
#endif // EXT_VPROF_ENABLED

	// Compute a direction vector point towards the crossing point
	Vector forward = (*crosspoint - fromPos);
	forward.z = 0.0f;
	forward.NormalizeInPlace();
	Vector left(-forward.y, forward.x, 0.0f);

	if (left.IsZero())
	{
		return;
	}

	trace::CTraceFilterNoNPCsOrPlayers filter(GetBot()->GetEntity(), COLLISION_GROUP_NONE);
	trace_t result;
	float hullwidth = GetHullWidth();
	Vector mins(-hullwidth, -hullwidth, GetStepHeight());
	Vector maxs(hullwidth, hullwidth, GetStandingHullHeight());
	const Vector& endPos = *crosspoint;
	
	trace::hull(fromPos, endPos, mins, maxs, GetMovementTraceMask(), &filter, result);

	if (!result.DidHit())
	{
		return; // no collision
	}

	if (GetBot()->IsDebugging(BOTDEBUG_PATH))
	{
		NDebugOverlay::SweptBox(fromPos, endPos, mins, maxs, vec3_angle, 255, 0, 0, 255, 0.2f);
	}

	// direct path is blocked, try left first

	trace::hull(fromPos, endPos + (left * hullwidth), mins, maxs, GetMovementTraceMask(), &filter, result);

	if (!result.DidHit())
	{
		*crosspoint = endPos + (left * hullwidth);

		if (GetBot()->IsDebugging(BOTDEBUG_PATH))
		{
			NDebugOverlay::SweptBox(fromPos, *crosspoint, mins, maxs, vec3_angle, 0, 255, 0, 255, 0.2f);
		}

		return;
	}

	// try right
	trace::hull(fromPos, endPos + (left * -hullwidth), mins, maxs, GetMovementTraceMask(), &filter, result);

	if (!result.DidHit())
	{
		*crosspoint = endPos + (left * -hullwidth);

		if (GetBot()->IsDebugging(BOTDEBUG_PATH))
		{
			NDebugOverlay::SweptBox(fromPos, *crosspoint, mins, maxs, vec3_angle, 0, 255, 0, 255, 0.2f);
		}

		return;
	}
}

class MovementFunctorMegaBreak
{
public:
	MovementFunctorMegaBreak(CBaseBot* bot)
	{
		m_bot = bot;
	}

	bool operator()(CBaseEntity* entity)
	{
		if (!UtilHelpers::IsPlayer(entity))
		{
			if (m_bot->IsAbleToBreak(entity))
			{
				if (entprops->HasEntProp(entity, Prop_Data, "InputBreak"))
				{
					UtilHelpers::io::FireInput(entity, "InputBreak", m_bot->GetEntity(), entity, m_emptyVariant);
				}
			}
		}

		return true;
	}

	variant_t m_emptyVariant;
	CBaseBot* m_bot;
};

void IMovement::TryToUnstuck()
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("IMovement::TryToUnstuck", "NavBot");
#endif // EXT_VPROF_ENABLED

	auto bot = GetBot();

	if (bot->IsOnLadder())
	{
		if (m_ladderState == NOT_USING_LADDER)
		{
			// bot got stuck on a ladder
			CNavArea* area = bot->GetLastKnownNavArea();

			if (area != nullptr)
			{
				bot->GetControlInterface()->AimAt(area->GetCenter(), IPlayerController::LOOK_CRITICAL, 0.2f, "Stuck on Ladder");
				Jump();
				bot->GetControlInterface()->PressUseButton();
			}
		}
		else if (m_ladderState == IMovement::LadderState::USING_LADDER_UP)
		{
			/*
				* If going up a ladder, the bot may get stuck near the top if the ladder is inside a vent (IE: in cs_assault).
				* Do a trace to see if there is anything blocking player movement above, if yes, crouch.
			*/

			Vector start = bot->GetAbsOrigin();
			Vector end = start;
			end.z += (GetStandingHullHeight() * 2.0f);
			trace_t tr;
			trace::line(start, end, MASK_PLAYERSOLID, bot->GetEntity(), COLLISION_GROUP_PLAYER, tr);

			if (tr.fraction < 1.0f)
			{
				bot->GetControlInterface()->PressCrouchButton(0.5f);
			}
		}

		return;
	}

	// Jumping in these states will probably cause more issues than fix then, don't try to do anything.
	if (m_isJumping || m_isJumpingAcrossGap || m_isClimbingObstacle || m_isUsingCatapult || IsUsingElevator())
	{
		return;
	}

	if (!IsOnGround())
	{
		return;
	}

	const CModSettings* modsettings = extmanager->GetMod()->GetModSettings();

	if (!IsBreakingObstacle())
	{
		CBaseEntity* pGroundEntity = bot->GetGroundEntity();

		if (pGroundEntity)
		{
			entities::HBaseEntity groundent{ pGroundEntity };

			// not world ent
			if (groundent.GetIndex() != 0)
			{
				if (bot->IsAbleToBreak(pGroundEntity))
				{
					BreakObstacle(pGroundEntity);
					return;
				}
			}
		}

		if (modsettings->AllowUnstuckCheats())
		{
			if (modsettings->AllowUnstuckTeleport() && sdkcalls->IsTeleportAvailable() && m_stuck.counter >= modsettings->GetUnstuckTeleportThreshold())
			{
				CMeshNavigator* path = bot->GetActiveNavigator();

				if (path && path->GetGoalSegment())
				{
					UnstuckTeleport(bot, path, path->GetGoalSegment());
					return;
				}
			}

			const Vector& origin = m_stuck.GetStuckPos();
			Vector size = { GetHullWidth() * 2.0f, GetHullWidth() * 2.0f, GetStandingHullHeight() * 1.4f };
			UtilHelpers::CEntityEnumerator enumerator;
			UtilHelpers::EntitiesInBox(origin - size, origin + size, enumerator);
			MovementFunctorMegaBreak functor{ bot };
			enumerator.ForEach(functor);
		}
	}

	Vector start = bot->GetEyeOrigin();
	Vector end(start.x, start.y, start.z + 64.0f);
	trace::CTraceFilterNoNPCsOrPlayers filter(bot->GetEntity(), COLLISION_GROUP_PLAYER_MOVEMENT);
	trace_t tr;
	trace::line(start, end, MASK_PLAYERSOLID, &filter, tr);

	if (!tr.DidHit())
	{
		if (bot->IsDebugging(BOTDEBUG_MOVEMENT))
		{
			debugoverlay->AddLineOverlay(start, end, 0, 128, 0, true, 3.0f);
			bot->DebugPrintToConsole(255, 69, 0, "%s IMovement::TryToUnstuck No obstacle on top, crouch jumping! \n", bot->GetDebugIdentifier());
		}

		CrouchJump();

		if (modsettings->AllowUnstuckCheats())
		{
			CMeshNavigator* path = bot->GetActiveNavigator();

			if (path && path->GetGoalSegment())
			{
				Vector launch = bot->CalculateLaunchVector(path->GetGoalSegment()->goal, GetMaxSpeed());
				bot->SetAbsVelocity(launch);
			}
		}
	}
}

void IMovement::ObstacleOnPath(CBaseEntity* obstacle, const Vector& goalPos, const Vector& forward, const Vector& left)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("IMovement::ObstacleOnPath", "NavBot");
#endif // EXT_VPROF_ENABLED

	// Wait for the bot to be actually stuck by the obstacle, prevents excessive jumping on slope/ramps
	if (GetGroundSpeed() > 16.0f)
	{
		return;
	}

	Vector mins(-8, -8, -8);
	Vector maxs(8, 8, 4);
	Vector start = GetBot()->GetEyeOrigin();
	Vector end = goalPos;
	end.z = start.z;

	trace::CTraceFilterNoNPCsOrPlayers filter(GetBot()->GetEntity(), COLLISION_GROUP_PLAYER_MOVEMENT);
	trace_t tr;
	trace::hull(start, end, mins, maxs, MASK_PLAYERSOLID, &filter, tr);

	if (!tr.startsolid && tr.fraction >= 0.9999f)
	{
		// Nothing ahead around eye level, JUMP!
		CrouchJump();

		if (GetBot()->IsDebugging(BOTDEBUG_MOVEMENT))
		{
			NDebugOverlay::SweptBox(start, end, mins, maxs, vec3_angle, 0, 156, 0, 255, 0.3f);
			GetBot()->DebugPrintToConsole(BOTDEBUG_MOVEMENT, 200, 255, 200, "%s IMovement::ObstacleOnPath CLEAR \nGoal: <%3.2f, %3.2f, %3.2f> \n", GetBot()->GetDebugIdentifier(), goalPos.x, goalPos.y, goalPos.z);
		}
	}
	else
	{
		if (GetBot()->IsDebugging(BOTDEBUG_MOVEMENT))
		{
			NDebugOverlay::SweptBox(start, end, mins, maxs, vec3_angle, 255, 0, 0, 255, 0.3f);
			GetBot()->DebugPrintToConsole(BOTDEBUG_MOVEMENT, 200, 255, 200, "%s IMovement::ObstacleOnPath NOT CLEAR \nGoal: <%3.2f, %3.2f, %3.2f> \n", GetBot()->GetDebugIdentifier(), goalPos.x, goalPos.y, goalPos.z);
		}
	}
}

bool IMovement::IsUsingElevator() const
{
	return m_elevator != nullptr;
}

void IMovement::UseElevator(const CNavElevator* elevator, const CNavArea* from, const CNavArea* to)
{
	if (from->GetMyElevatorFloor() == nullptr || to->GetMyElevatorFloor() == nullptr)
	{
		return;
	}

	m_elevator = elevator;
	m_fromFloor = from->GetMyElevatorFloor();
	m_toFloor = to->GetMyElevatorFloor();
	m_elevatorTimeout.Invalidate();
	m_elevatorState = ElevatorState::MOVE_TO_WAIT_POS;
	SetDesiredSpeed(GetRunSpeed() * IMovement::ELEV_MOVESPEED_SCALE);

	if (GetBot()->IsDebugging(BOTDEBUG_MOVEMENT))
	{
		GetBot()->DebugPrintToConsole(255, 105, 180, "%s Use Elevator #%i to go from area #%i to area #%i!\n", GetBot()->GetDebugIdentifier(), 
			m_elevator->GetID(), from->GetID(), to->GetID());
	}
}

bool IMovement::BreakObstacle(CBaseEntity* obstacle)
{
	if (obstacle == nullptr)
	{
		return false;
	}

	int index = gamehelpers->EntityToBCompatRef(obstacle);

	// Don't break worldspawn
	if (index == 0) { return false; }

	constexpr auto MIN_DAMAGE = 45.0f;
	int health = UtilHelpers::GetEntityHealth(index);
	float timeout = (static_cast<float>(health) / MIN_DAMAGE) + 2.0f;

	m_obstacleEntity = obstacle;
	m_isBreakingObstacle = true;
	m_obstacleBreakTimeout.Start(timeout);

	if (GetBot<CBaseBot>()->IsDebugging(BOTDEBUG_MOVEMENT))
	{
		GetBot<CBaseBot>()->DebugPrintToConsole(0, 255, 255, "%s BREAKING OBSTACLE %s HEALTH %i TIMEOUT %g\n", 
			GetBot<CBaseBot>()->GetDebugIdentifier(), UtilHelpers::textformat::FormatEntity(obstacle), health, timeout);
	}

	return true;
}

bool IMovement::IsUseableObstacle(CBaseEntity* entity)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("IMovement::IsUseableObstacle", "NavBot");
#endif // EXT_VPROF_ENABLED

	const char* classname = gamehelpers->GetEntityClassname(entity);

	if (std::strcmp(classname, "func_door") == 0 || std::strcmp(classname, "func_door_rotating") == 0)
	{
		constexpr int SF_USEOPENS = 256; // spawn flag to allow +USE on brush doors. See: https://developer.valvesoftware.com/wiki/Func_door

		int spawnflags = 0;
		entprops->GetEntProp(gamehelpers->EntityToBCompatRef(entity), Prop_Data, "m_spawnflags", spawnflags);

		if ((spawnflags & SF_USEOPENS) != 0)
		{
			return true;
		}

		return false;
	}
	if (std::strcmp(classname, "prop_door_rotating") == 0)
	{
		int state = 0;
		entprops->GetEntProp(entity, Prop_Data, "m_eDoorState", state);

		if (state != static_cast<int>(sdkdefs::DoorState_t::DOOR_STATE_CLOSED))
		{
			return false; // only closed doors can be used
		}

		return true;
	}

	return false;
}

bool IMovement::UseCatapult(const Vector& start, const Vector& landing)
{
	m_catapultStartPosition = start;
	m_landingGoal = landing;
	m_isUsingCatapult = true;
	m_catapultCorrectVelocityTimer.Invalidate();
	Vector lookat = start;
	lookat.z += navgenparams->human_eye_height;
	GetBot<CBaseBot>()->GetControlInterface()->AimAt(lookat, IPlayerController::LOOK_MOVEMENT, 0.25f, "Looking at catapult!");

	if (GetBot()->IsDebugging(BOTDEBUG_MOVEMENT))
	{
		GetBot()->DebugPrintToConsole(255, 105, 180, "%s USE CATAPULT FROM %g %g %g TO %g %g %g!\n", GetBot()->GetDebugIdentifier(),
			start.x, start.y, start.z, landing.x, landing.y, landing.z);
		debugoverlay->AddLineOverlay(start, landing, 255, 165, 0, true, 15.0f);
	}

	return true;
}

bool IMovement::DoStrafeJump(const Vector& start, const Vector& end)
{
	constexpr auto DEBUG_ARROW_WIDTH = 4.0f;
	m_sjEndPoint = end;
	bool result = CalculateStrafeJumpMidPoint(start, end);

	if (result)
	{
		SetDesiredSpeed(GetRunSpeed());
		MoveTowards(start, MOVEWEIGHT_PRIORITY);
		m_strafeJumpState = StrafeJumpState::STRAFEJUMP_INIT;

		if (GetBot<CBaseBot>()->IsDebugging(BOTDEBUG_MOVEMENT))
		{
			NDebugOverlay::HorzArrow(start, m_sjMidPoint, DEBUG_ARROW_WIDTH, 0, 180, 0, 255, true, 5.0f);
			NDebugOverlay::HorzArrow(m_sjMidPoint, end, DEBUG_ARROW_WIDTH, 0, 180, 0, 255, true, 5.0f);
		}
	}

	return result;
}

void IMovement::OnDoneBreakingObstacle(CBaseEntity* obstacle, const bool istimedout)
{
	CBaseBot* me = GetBot<CBaseBot>();

	if (extmanager->GetMod()->GetModSettings()->ShouldUseMovementBreakAssist())
	{
		if (istimedout && obstacle)
		{
			CBaseEntity* myentity = me->GetEntity();
			variant_t emptyvariant;

			UtilHelpers::io::FireInput(obstacle, "InputBreak", myentity, myentity, emptyvariant);
		}
	}

	if (me->IsDebugging(BOTDEBUG_MOVEMENT))
	{
		me->DebugPrintToConsole(255, 255, 0, "%s DONE BREAKING OBSTACLE %p. IS TIME OUT: %s \n", me->GetDebugIdentifier(), obstacle, istimedout ? "YES" : "NO");
	}
}

bool IMovement::RequestMovementTypeChange(MovementType type, MovementRequestPriority priority, float duration, const char* reason)
{
	CBaseBot* me = GetBot<CBaseBot>();

	if (!m_MTRequestTimer.IsElapsed() && priority < m_lastMTRequestPriority)
	{
		if (me->IsDebugging(BOTDEBUG_MOVEMENT))
		{
			me->DebugPrintToConsole(255, 0, 0, "%s MOVEMENT TYPE CHANGE REQUEST DENIED! TYPE: \"%s\" PRIORITY: \"%s\" DURATION: %3.2f REASON: %s \n",
				me->GetDebugIdentifier(), 
				IMovement::MovementTypeToString(type),
				IMovement::MovementRequestPriorityTypeToString(priority),
				duration,
				reason != nullptr ? reason : "");
		}

		return false;
	}

	// if the current type is the request type, just update the timer and the priority
	if (GetMovementType() == type)
	{
		m_lastMTRequestPriority = priority;
		m_MTRequestTimer.Start(duration);
		return true;
	}

	if (me->IsDebugging(BOTDEBUG_MOVEMENT))
	{
		me->DebugPrintToConsole(255, 0, 0, "%s MOVEMENT TYPE CHANGED! OLD TYPE: \"%s\" NEW TYPE: \"%s\" PRIORITY: \"%s\" DURATION: %3.2f REASON: %s \n",
			me->GetDebugIdentifier(),
			IMovement::MovementTypeToString(GetMovementType()),
			IMovement::MovementTypeToString(type),
			IMovement::MovementRequestPriorityTypeToString(priority),
			duration,
			reason != nullptr ? reason : "");
	}

	m_lastMTRequestPriority = priority;
	m_MTRequestTimer.Start(duration);
	ChangeMovementType(type);

	return true;
}

void IMovement::StuckMonitor()
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("IMovement::StuckMonitor", "NavBot");
#endif // EXT_VPROF_ENABLED

	CBaseBot* bot = GetBot<CBaseBot>();
	Vector origin = bot->GetAbsOrigin();
	constexpr auto STUCK_RADIUS = 100.0f; // distance the bot has to move to consider not stuck

	if (!bot->IsAlive())
	{
		return;
	}

	if (m_ladderWait.HasStarted() && !m_ladderWait.IsElapsed())
	{
		// not stuck, waiting on a ladder
		m_stuck.UpdateNotStuck(origin);
		return;
	}

	// These have timeout timers, don't send stuck events.
	if (IsUsingElevator() || IsBreakingObstacle())
	{
		m_stuck.UpdateNotStuck(origin);
		return;
	}

	// bot hasn't moved in a while. A bot can't be stuck if it doesn't want to move
	if (m_stuck.IsIdle())
	{
		m_stuck.UpdateNotStuck(origin);
		return;
	}

	if (m_stuck.isstuck)
	{
		// bot is stuck
		if (bot->IsRangeGreaterThan(m_stuck.GetStuckPos(), STUCK_RADIUS))
		{
			ClearStuckStatus("Stuck Monitor: No longer stuck.");
			bot->OnUnstuck();
			m_stuck.UpdateNotStuck(origin);

			if (bot->IsDebugging(BOTDEBUG_MOVEMENT))
			{
				bot->DebugPrintToConsole(BOTDEBUG_MOVEMENT, 200, 255, 200, "%s UNSTUCK! \n", bot->GetDebugIdentifier());
				NDebugOverlay::Circle(m_stuck.GetStuckPos() + Vector(0.0f, 0.0f, 5.0f), QAngle(-90.0f, 0.0f, 0.0f), 5.0f, 0, 255, 0, 255, true, 1.0f);
			}
		}
		else
		{
			if (m_stuck.stuckrechecktimer.IsElapsed())
			{
				m_stuck.stuckrechecktimer.Start(1.0f);

				// resend stuck event
				bot->OnStuck();
				TryToUnstuck();
				m_stuck.counter++; // still stuck, increase counter

				if (m_stuck.counter > extmanager->GetMod()->GetModSettings()->GetStuckSuicideThreshold())
				{
					smutils->LogMessage(myself, "Bot \"%s\" suicided due to being stuck for too long. %g %g %g", bot->GetClientName(), origin.x, origin.y, origin.z);
					bot->DelayedFakeClientCommand("kill");
					ClearStuckStatus("Kill command sent."); // don't spam the log files
				}

				if (bot->IsDebugging(BOTDEBUG_MOVEMENT))
				{
					bot->DebugPrintToConsole(BOTDEBUG_MOVEMENT, 200, 255, 200, "%s STILL STUCK AT %g %g %g! \n", bot->GetDebugIdentifier(), origin.x, origin.y, origin.z);
					NDebugOverlay::Circle(m_stuck.GetStuckPos() + Vector(0.0f, 0.0f, 5.0f), QAngle(-90.0f, 0.0f, 0.0f), 5.0f, 255, 0, 0, 255, true, 1.0f);
				}
			}
		}
	}
	else // bot is NOT stuck
	{
		if (bot->IsRangeGreaterThan(m_stuck.GetStuckPos(), STUCK_RADIUS))
		{
			// bot moved, update position
			m_stuck.UpdateNotStuck(origin);
		}
		else
		{
			float minspeed = GetMinimumMovementSpeed() / 4.0f;
			float mintimeforstuck = STUCK_RADIUS / minspeed;

			if (m_stuck.stucktimer.IsGreaterThen(mintimeforstuck))
			{
				// bot is taking too long to move, consider it stuck
				m_stuck.Stuck(bot);
				bot->OnStuck();
				TryToUnstuck();

				if (bot->IsDebugging(BOTDEBUG_MOVEMENT))
				{
					bot->DebugPrintToConsole(BOTDEBUG_MOVEMENT, 200, 255, 200, "%s GOT STUCK AT %g %g %g! \n", bot->GetDebugIdentifier(), origin.x, origin.y, origin.z);
					NDebugOverlay::Circle(origin + Vector(0.0f, 0.0f, 5.0f), QAngle(-90.0f, 0.0f, 0.0f), 5.0f, 255, 0, 0, 255, true, 1.0f);
				}
			}
		}
	}
}

void IMovement::TraverseLadder()
{
	LadderState newState;

	switch (m_ladderState)
	{
	case IMovement::APPROACHING_LADDER_UP:
		newState = ApproachUpLadder();
		break;
	case IMovement::EXITING_LADDER_UP:
		newState = DismountLadderTop();
		break;
	case IMovement::USING_LADDER_UP:
		newState = UseLadderUp();
		break;
	case IMovement::APPROACHING_LADDER_DOWN:
		newState = ApproachDownLadder();
		break;
	case IMovement::EXITING_LADDER_DOWN:
		newState = DismountLadderBottom();
		break;
	case IMovement::USING_LADDER_DOWN:
		newState = UseLadderDown();
		break;
	default:
		return;
	}

	if (newState != m_ladderState)
	{
		CBaseBot* me = GetBot<CBaseBot>();

		if (me->IsDebugging(BOTDEBUG_MOVEMENT))
		{
			me->DebugPrintToConsole(255, 255, 0, "%s LADDER STATE CHANGED FROM %i TO %i \n", me->GetDebugIdentifier(), static_cast<int>(m_ladderState), static_cast<int>(newState));
		}

		ChangeLadderState(newState);
	}
}

void IMovement::ElevatorUpdate()
{
	if (m_elevatorState == NOT_USING_ELEVATOR)
	{
		return;
	}

	ElevatorState initialState = m_elevatorState;

	switch (m_elevatorState)
	{
	case IMovement::MOVE_TO_WAIT_POS:
		m_elevatorState = EState_MoveToWaitPosition();
		break;
	case IMovement::CALL_ELEVATOR:
		m_elevatorState = EState_CallElevator();
		break;
	case IMovement::WAIT_FOR_ELEVATOR:
		m_elevatorState = EState_WaitForElevator();
		break;
	case IMovement::ENTER_ELEVATOR:
		m_elevatorState = EState_EnterElevator();
		break;
	case IMovement::OPERATE_ELEVATOR:
		m_elevatorState = EState_OperateElevator();
		break;
	case IMovement::RIDE_ELEVATOR:
		m_elevatorState = EState_RideElevator();
		break;
	case IMovement::EXIT_ELEVATOR:
		m_elevatorState = EState_ExitElevator();
		break;
	default:
		CleanUpElevator();
		return;
	}

	if (GetBot()->IsDebugging(BOTDEBUG_MOVEMENT) && initialState != m_elevatorState)
	{
		GetBot()->DebugPrintToConsole(255, 105, 180, "%s Elevator State Transition From [%i] to [%i]!\n", GetBot()->GetDebugIdentifier(), static_cast<int>(initialState), static_cast<int>(m_elevatorState));
	}

	// transition to not using elevator state
	if (m_elevatorState == NOT_USING_ELEVATOR)
	{
		CleanUpElevator();
	}
}

void IMovement::ObstacleBreakUpdate()
{
	if (!m_isBreakingObstacle)
	{
		return;
	}

	CBaseEntity* obstacle = m_obstacleEntity.Get();

	if (m_obstacleBreakTimeout.IsElapsed() || obstacle == nullptr)
	{
		OnDoneBreakingObstacle(obstacle, m_obstacleBreakTimeout.IsElapsed());

		m_obstacleBreakTimeout.Invalidate();
		m_isBreakingObstacle = false;
		m_obstacleEntity = nullptr;
		GetBot()->GetControlInterface()->ReleaseAllAttackButtons();
		return;
	}

	CBaseBot* bot = GetBot();
	Vector eyepos = bot->GetEyeOrigin();
	Vector origin = bot->GetAbsOrigin();
	Vector pos;
	Vector center = UtilHelpers::getWorldSpaceCenter(obstacle);
	UtilHelpers::math::CalcClosestPointOfEntity(obstacle, eyepos, pos);
	bot->GetInventoryInterface()->SelectBestWeaponForBreakables();
	
	MoveTowards(center, MOVEWEIGHT_CRITICAL);

	if ((eyepos - pos).IsLengthLessThan(256.0f))
	{
		if (center.z < origin.z)
		{
			bot->GetControlInterface()->PressCrouchButton();
		}

		bot->GetControlInterface()->AimAt(pos, IPlayerController::LOOK_MOVEMENT, 0.5f, "Looking at breakable entity!");

		if (bot->GetControlInterface()->IsAimOnTarget())
		{
			bot->GetCombatInterface()->DoPrimaryAttack();
		}
	}
}

void IMovement::OnJumpComplete()
{
	CBaseBot* me = GetBot<CBaseBot>();

	if (m_isJumpingAcrossGap)
	{
		// the bot some times backtracks, either falling or entering a loop
		// force the path to update it's goal position
		me->UpdateLastKnownNavArea(true);
		if (me->GetActiveNavigator()) { me->GetActiveNavigator()->AdvanceGoalToNearest(); }
	}

	// jump complete
	m_isJumpingAcrossGap = false;
	m_isClimbingObstacle = false;
	m_isAirborne = false;
	m_isJumping = false;
	m_wasLaunched = false;
	m_jumpTimer.Invalidate();
	m_jumpCooldown.Start(sm_navbot_movement_jump_cooldown.GetFloat());
	me->UpdateLastKnownNavArea(true);
	DoCounterStrafe(0.25f); // experimental to regain control post landing
	StopAndWait(0.25f); // wait some time for better precision in consecutive jumps
	me->GetControlInterface()->ReleaseJumpButton();
	me->GetControlInterface()->ReleaseCrouchButton();

	if (me->IsDebugging(BOTDEBUG_MOVEMENT))
	{
		me->DebugPrintToConsole(0, 170, 0, "%s JUMP COMPLETE \n", me->GetDebugIdentifier());
	}
}

bool IMovement::CalculateStrafeJumpMidPoint(const Vector& start, const Vector& end)
{
	CBaseBot* bot = GetBot<CBaseBot>();
	trace::CTraceFilterSimple filter(bot->GetEntity(), COLLISION_GROUP_PLAYER_MOVEMENT);
	trace_t tr;
	const float halfhull = GetHullWidth() * 0.5f;
	const float halfheight = GetStandingHullHeight() * 0.5f;
	const Vector mins{ -halfhull, -halfhull, 0.0f };
	const Vector maxs{ halfhull, halfhull, halfheight };
	const Vector dir = UtilHelpers::math::BuildDirectionVector(start, end);
	const Vector offset{ 0.0f, 0.0f, halfheight };
	const float midrange = ((end - start).Length()) * 0.5f;
	const Vector traceStart = start + offset;
	float limit = sm_navbot_movement_strafejump_max_angle.GetFloat();

	for (float y = 5.0f; y <= limit; y += 5.0f)
	{
		QAngle angle{ 0.0f, y, 0.0f };
		Vector newdir;
		VectorRotate(dir, angle, newdir);
		Vector midpoint = traceStart + (newdir * midrange);

		trace::hull(traceStart, midpoint, mins, maxs, MASK_PLAYERSOLID, &filter, tr);

		if (!tr.DidHit())
		{
			m_sjMidPoint = midpoint;
			m_sjIsToTheLeft = true;
			return true;
		}
	}

	limit *= -1.0f;

	for (float y = -5.0f; y >= limit; y -= 5.0f)
	{
		QAngle angle{ 0.0f, y, 0.0f };
		Vector newdir;
		VectorRotate(dir, angle, newdir);
		Vector midpoint = traceStart + (newdir * midrange);

		trace::hull(traceStart, midpoint, mins, maxs, MASK_PLAYERSOLID, &filter, tr);

		if (!tr.DidHit())
		{
			m_sjMidPoint = midpoint;
			m_sjIsToTheLeft = false;
			return true;
		}
	}

	return false;
}

void IMovement::StrafeJumpUpdate()
{
	StrafeJumpState next;

	switch (m_strafeJumpState)
	{
	case StrafeJumpState::STRAFEJUMP_INIT:
		next = StrafeJump_Init();
		break;
	case StrafeJumpState::STRAFEJUMP_TO_MIDPOINT:
		next = StrafeJump_ToMidPoint();
		break;
	case StrafeJumpState::STRAFEJUMP_TO_ENDPOINT:
		next = StrafeJump_ToEndPoint();
		break;
	default:
		return;
	}

	m_strafeJumpState = next;
}

void IMovement::UnstuckTeleport(CBaseBot* bot, CMeshNavigator* navigator, const CBasePathSegment* goal)
{
	const CBasePathSegment* ground = navigator->GetFirstGroundSegment(goal);

	if (!ground) { return; }

	sdkcalls->CBaseEntity_Teleport(bot->GetEntity(), &ground->goal);
	ClearStuckStatus("Teleported!");
}

// approach a ladder that we will go up
IMovement::LadderState IMovement::ApproachUpLadder()
{
	if (m_ladder == nullptr)
	{
		return NOT_USING_LADDER;
	}

	// remember, the ladder is facing the opposide direction of the bot, -1.0 means the bot is facing the ladder directly
	constexpr float FACING_LADDER_DOT = -0.85f;
	
	// the ladder should always have a connection to the ladder exit area, if not let it crash to notify a programmer
	const LadderToAreaConnection* connection = m_ladder->GetConnectionToArea(m_ladderExit);
	CBaseBot* bot = GetBot<CBaseBot>();
	const Vector origin = bot->GetAbsOrigin();
	const bool debugging = bot->IsDebugging(BOTDEBUG_MOVEMENT);

	if (IsOnLadder())
	{
		bot->GetControlInterface()->ReleaseMovementButtons();

		if (!m_ladderWait.HasStarted())
		{
			m_ladderWait.Start(0.25f);
			bot->DebugPrintToConsole(BOTDEBUG_MOVEMENT, 0, 200, 200, "%s GRABBED LADDER (UP)! \n", bot->GetDebugIdentifier());
		}
		else if (m_ladderWait.IsElapsed())
		{
			bot->DebugPrintToConsole(BOTDEBUG_MOVEMENT, 0, 200, 200, "%s WAIT TIMER EXPIRED (UP)! \n", bot->GetDebugIdentifier());
			return USING_LADDER_UP; // wait timer expired, we are on a ladder
		}
	}
	else
	{
		Vector to = UtilHelpers::math::BuildDirectionVector(origin, m_ladderMoveGoal);
		const float dot = DotProduct(to, m_ladder->GetNormal());

		// not facing the ladder, move in front of it.
		if (dot > FACING_LADDER_DOT)
		{
			Vector moveTo = m_ladderMoveGoal + (m_ladder->GetNormal() * GetHullWidth() * 1.6f);
			MoveTowards(moveTo, MOVEWEIGHT_CRITICAL);
			FaceTowards(moveTo + Vector(0.0f, 0.0f, navgenparams->human_eye_height), true);

			if (debugging)
			{
				bot->DebugPrintToConsole(180, 220, 180, "%s APPROACHUPLADDER : NOT FACING LADDER! %g \n", bot->GetDebugIdentifier(), dot);
				NDebugOverlay::HorzArrow(origin, moveTo, 8.0f, 255, 0, 0, 255, true, NDEBUG_PERSIST_FOR_ONE_TICK);
			}
		}
		else
		{
			// move to the ladder connection point (this is on the ladder)
			MoveTowards(m_ladderMoveGoal, MOVEWEIGHT_CRITICAL);
			FaceTowards(m_ladderMoveGoal + Vector(0.0f, 0.0f, navgenparams->human_eye_height), true);
			const Vector& point = connection->GetConnectionPoint();

			if (bot->GetRangeTo(point) < CBaseExtPlayer::PLAYER_USE_RADIUS)
			{
				if (m_useLadderTimer.IsElapsed())
				{
					bot->GetControlInterface()->PressUseButton();
					m_useLadderTimer.Start(0.2f);
				}
			}
		}

	}

	if (debugging)
	{
		NDebugOverlay::Cross3D(m_ladderMoveGoal, 24.0f, 0, 128, 0, true, NDEBUG_PERSIST_FOR_ONE_TICK);
		NDebugOverlay::Line(bot->GetAbsOrigin(), m_ladderMoveGoal, 0, 128, 0, true, NDEBUG_PERSIST_FOR_ONE_TICK);
	}

	return APPROACHING_LADDER_UP;
}

IMovement::LadderState IMovement::ApproachDownLadder()
{
	if (m_ladder == nullptr)
	{
		return NOT_USING_LADDER;
	}

	CBaseBot* me = GetBot<CBaseBot>();
	auto origin = me->GetAbsOrigin();
	const float z = origin.z;
	const bool debug = me->IsDebugging(BOTDEBUG_MOVEMENT);

	if (z <= m_ladder->m_bottom.z + GetMaxJumpHeight())
	{
		m_ladderTimer.Start(2.0f);
		return EXITING_LADDER_DOWN;
	}

	Vector climbPoint{ 0.0f, 0.0f, 0.0f };

	if (!IsOnGround())
	{
		// Airborne, probably missed the ladder, move towards it to grab it
		climbPoint = m_ladder->m_top - (0.5f * GetHullWidth() * m_ladder->GetNormal());

		if (!me->IsMovingTowards(climbPoint, 0.95f) && !m_wasLaunched)
		{
			// Cheat: Launch towards the ladder climb point to help the bot grab it
			Vector launch = me->CalculateLaunchVector(climbPoint, GetWalkSpeed());
			me->SetAbsVelocity(launch);
			m_wasLaunched = true;
		}
	}
	else if (m_ladder->GetLadderType() == CNavLadder::USEABLE_LADDER)
	{
		// useable ladder top/bottom positions are at the center of the ladder
		climbPoint = m_ladder->m_top;
	}
	else
	{
		climbPoint = m_ladder->m_top + (0.8f * GetHullWidth() * m_ladder->GetNormal());
	}

	climbPoint.z = m_ladder->ClampZ(z - (GetStepHeight() / 4.0f));

	Vector to = climbPoint - origin;
	to.z = 0.0f;
	float mountRange = to.NormalizeInPlace();
	Vector moveGoal = climbPoint;

	if (debug)
	{
		NDebugOverlay::HorzArrow(GetBot()->GetAbsOrigin(), moveGoal, 4.0f, 0, 255, 0, 255, false, 0.2f);
		NDebugOverlay::Cross3D(moveGoal, 12.0f, 0, 0, 255, false, 0.2f);
	}

	if (IsOnLadder())
	{
		GetBot()->GetControlInterface()->ReleaseMovementButtons();

		if (!m_ladderWait.HasStarted())
		{
			m_ladderWait.Start(0.25f);
			GetBot()->DebugPrintToConsole(BOTDEBUG_MOVEMENT, 0, 200, 200, "%s GRABBED LADDER (DOWN)! \n", GetBot()->GetDebugIdentifier());
		}
		else if (m_ladderWait.IsElapsed())
		{
			GetBot()->DebugPrintToConsole(BOTDEBUG_MOVEMENT, 0, 200, 200, "%s WAIT TIMER EXPIRED (DOWN)! \n", GetBot()->GetDebugIdentifier());
			return USING_LADDER_DOWN; // wait timer expired, we are on a ladder
		}
	}
	else
	{
		Vector lookAt = m_ladder->m_top;
		lookAt.z = m_ladder->ClampZ(z);

		GetBot()->GetControlInterface()->AimAt(lookAt, IPlayerController::LOOK_MOVEMENT, 0.1f, "Looking at ladder mount position!");
		MoveTowards(moveGoal, MOVEWEIGHT_CRITICAL);

		if (m_ladder->GetLadderType() == CNavLadder::USEABLE_LADDER)
		{

			// useable ladder within use range, press use button.
			if (GetBot()->GetAbsOrigin().AsVector2D().DistTo(m_ladder->m_top.AsVector2D()) < CBaseExtPlayer::PLAYER_USE_RADIUS)
			{
				if (m_useLadderTimer.IsElapsed())
				{
					GetBot()->GetControlInterface()->PressUseButton();
					m_useLadderTimer.Start(0.1f);
					GetBot()->DebugPrintToConsole(BOTDEBUG_MOVEMENT, 0, 156, 200, "%s USE LADDER\n", GetBot()->GetDebugIdentifier());
				}
			}
		}
	}

	return APPROACHING_LADDER_DOWN;
}

IMovement::LadderState IMovement::UseLadderUp()
{
	if (m_ladder == nullptr || m_ladderTimer.IsElapsed())
	{
		return NOT_USING_LADDER;
	}

	auto input = GetBot()->GetControlInterface();
	auto origin = GetBot()->GetAbsOrigin();
	const float z = origin.z;

	if (!IsOnLadder())
	{
		// Some ladders are very short at the top and the bot will frequently overshoot it
		if (z >= m_ladderGoalZ || z >= m_ladder->m_top.z)
		{
			return EXITING_LADDER_UP;
		}

		// fell off the ladder before reaching the goal height
		return NOT_USING_LADDER;
	}


	// Bot is not at the top of the ladder but is above the landing nav area height plus some offset distance
	if (z >= m_ladderGoalZ)
	{

		if (GetBot()->IsDebugging(BOTDEBUG_MOVEMENT))
		{
			GetBot()->DebugPrintToConsole(0, 128, 0, "%s: USE LADDER (UP) REACHED Z GOAL (z >= m_ladderGoalZ)\n", GetBot()->GetDebugIdentifier());
		}

		return EXITING_LADDER_UP;
	}

	if (z >= m_ladder->m_top.z)
	{
		// reached ladder top

		if (GetBot()->IsDebugging(BOTDEBUG_MOVEMENT))
		{
			GetBot()->DebugPrintToConsole(0, 128, 0, "%s: USE LADDER (UP) REACHED Z GOAL (z >= m_ladder->m_top.z)\n", GetBot()->GetDebugIdentifier());
		}

		return EXITING_LADDER_UP;
	}

	Vector goal = origin + 100.0f * (-m_ladder->GetNormal() + Vector(0.0f, 0.0f, 2.0f));
	input->AimAt(goal, IPlayerController::LOOK_MOVEMENT, 0.1f, "Going up a ladder.");
	MoveTowards(goal, MOVEWEIGHT_CRITICAL);

	if (GetBot()->IsDebugging(BOTDEBUG_MOVEMENT))
	{
		NDebugOverlay::VertArrow(origin, goal, 4.0f, 0, 255, 0, 255, false, 0.2f);
		NDebugOverlay::Cross3D(goal, 12.0f, 0, 0, 255, false, 0.2f);
		m_ladderExit->DrawFilled(0, 255, 255, 200);
		NDebugOverlay::Text(m_ladderExit->GetCenter(), false, 0.1f, "LADDER EXIT AREA %3.2f", m_ladderGoalZ);
		NDebugOverlay::Text(origin, false, 0.1f, "BOT Z: %3.2f", origin.z);
		GetBot()->DebugPrintToConsole(51, 153, 153, "%s: Using Ladder Up. Distance to Z goal: %3.2f\n", GetBot()->GetDebugIdentifier(), m_ladderGoalZ - z);
	}

	return USING_LADDER_UP;
}

IMovement::LadderState IMovement::UseLadderDown()
{
	if (m_ladder == nullptr || m_ladderTimer.IsElapsed())
	{
		return NOT_USING_LADDER;
	}

	CBaseBot* me = GetBot<CBaseBot>();
	auto input = me->GetControlInterface();
	auto origin = me->GetAbsOrigin();
	const float z = origin.z;
	const bool isOnLadder = IsOnLadder();
	const bool isDebugging = me->IsDebugging(BOTDEBUG_MOVEMENT);

	// Fell off the ladder
	if (!isOnLadder)
	{
		if (z <= m_ladder->m_bottom.z)
		{
			return NOT_USING_LADDER;
		}

		// move towards the ladder to grab it again
		if (!IsOnGround())
		{
			Vector goal = origin + 100.0f * (-m_ladder->GetNormal() + Vector(0.0f, 0.0f, -2.0f));
			input->AimAt(goal, IPlayerController::LOOK_MOVEMENT, 0.1f);
			MoveTowards(goal, MOVEWEIGHT_CRITICAL);

			if (isDebugging)
			{
				debugoverlay->AddTextOverlay(origin + Vector(0.0f, 0.0f, 24.0f), 0.1f, "BOT OFF LADDER!");
				NDebugOverlay::HorzArrow(origin, goal, 8.0f, 255, 0, 0, 255, true, 0.1f);
			}

			return USING_LADDER_DOWN;
		}
	}

	if (z <= m_ladderGoalZ)
	{
		return EXITING_LADDER_DOWN;
	}

	if (z <= m_ladder->m_bottom.z)
	{
		return EXITING_LADDER_DOWN;
	}

	Vector goal = origin + 100.0f * (m_ladder->GetNormal() + Vector(0.0f, 0.0f, -2.0f));
	input->AimAt(goal, IPlayerController::LOOK_MOVEMENT, 0.1f);
	MoveTowards(goal, MOVEWEIGHT_CRITICAL);

	if (isDebugging)
	{
		NDebugOverlay::VertArrow(origin, goal, 4.0f, 0, 255, 0, 255, false, 0.2f);
		NDebugOverlay::Cross3D(goal, 12.0f, 0, 0, 255, false, 0.2f);
		me->DebugPrintToConsole(51, 153, 153, "%s: Using Ladder Down. Distance to Z goal: %3.2f\n", me->GetDebugIdentifier(), fabsf(m_ladderGoalZ - z));
	}

	return USING_LADDER_DOWN;
}

IMovement::LadderState IMovement::DismountLadderTop()
{
	if (m_ladder == nullptr)
	{
		return NOT_USING_LADDER;
	}

	CBaseBot* me = GetBot<CBaseBot>();
	const Vector origin = me->GetAbsOrigin();
	auto input = me->GetControlInterface();
	const Vector& dismountGoal = m_ladderExit->GetCenter();
	const bool isOnLadder = IsOnLadder();
	const bool isOnGround = IsOnGround();

	input->AimAt(dismountGoal + Vector(0.0f, 0.0f, GetCrouchedHullHeight()), IPlayerController::LOOK_MOVEMENT, 0.5f, "Looking at ladder top dismount goal!");

	// not on a ladder, not on the ground and above the dismount goal Z, probably overshot the ladder and airborne
	if (!isOnLadder && !isOnGround && origin.z >= (dismountGoal.z - GetStepHeight()))
	{
		// Fake jump off the ladder
		Vector launch = me->CalculateLaunchVector(dismountGoal, GetWalkSpeed());
		me->SetAbsVelocity(launch);
		return NOT_USING_LADDER;
	}

	if (input->IsAimOnTarget())
	{
		MoveTowards(dismountGoal, MOVEWEIGHT_CRITICAL);

		// Not on the ladder and is on solid ground
		if (!isOnLadder && isOnGround)
		{
			// See if there are any gaps on the ground, if yes we simulate a fake jump
			if (HasPotentialGap(me->GetAbsOrigin(), dismountGoal, nullptr))
			{
				// Fake jump off the ladder
				Vector launch = me->CalculateLaunchVector(dismountGoal, GetWalkSpeed());
				me->SetAbsVelocity(launch);
			}

			return NOT_USING_LADDER;
		}
	}

	return EXITING_LADDER_UP;
}

IMovement::LadderState IMovement::DismountLadderBottom()
{
	if (m_ladder == nullptr)
	{
		return NOT_USING_LADDER;
	}

	CBaseBot* me = GetBot<CBaseBot>();
	auto input = me->GetControlInterface();
	const Vector origin = me->GetAbsOrigin();
	Vector toExit = (m_ladderExit->GetCenter() - origin);
	toExit.z = 0.0f;
	toExit.NormalizeInPlace();
	Vector lookAt = me->GetEyeOrigin() + (toExit * 256.0f);
	const bool isOnLadder = IsOnLadder();
	const bool isOnGround = IsOnGround();
	const Vector& moveGoal = m_ladderExit->GetCenter();

	input->AimAt(lookAt, IPlayerController::LOOK_MOVEMENT, 0.2f, "Looking at ladder dismount area!");
	MoveTowards(moveGoal, MOVEWEIGHT_CRITICAL);

	
	if (!isOnLadder)
	{
		if (isOnGround)
		{
			// Off the ladder and touching ground
			return NOT_USING_LADDER;
		}

		if (HasPotentialGap(origin, moveGoal, nullptr))
		{
			// Fake jump off the ladder
			Vector launch = me->CalculateLaunchVector(moveGoal, GetWalkSpeed());
			me->SetAbsVelocity(launch);
			return NOT_USING_LADDER;
		}
	}

	return EXITING_LADDER_DOWN;
}

void IMovement::OnLadderStateChanged(LadderState oldState, LadderState newState)
{
	// time to climb is the length divided by this value
	constexpr auto LADDER_TIME_DIVIDER = 20.0f;
	CBaseBot* me = GetBot<CBaseBot>();

	switch (newState)
	{
	case IMovement::NOT_USING_LADDER:
	{
		m_wasLaunched = false;
		m_ladder = nullptr;
		m_ladderExit = nullptr;
		m_ladderTimer.Invalidate();
		m_ladderWait.Invalidate();
		m_ladderGoalZ = 0.0f;
		m_ladderMoveGoal = vec3_origin;
		SetDesiredSpeed(GetRunSpeed());
		me->UpdateLastKnownNavArea(true);
		return;
	}
	case IMovement::EXITING_LADDER_UP:
	case IMovement::EXITING_LADDER_DOWN:
	{
		m_ladderTimer.Start(4.0f);
		m_ladderWait.Invalidate();
		SetDesiredSpeed(GetRunSpeed());
		m_wasLaunched = false;
		return;
	}

	case IMovement::APPROACHING_LADDER_UP:
	{
		float z = GetBot()->GetAbsOrigin().z;
		z = m_ladder->ClampZ(z);

		m_ladderMoveGoal = m_ladder->m_bottom;
		m_ladderMoveGoal.z = z;
		me->GetControlInterface()->SnapAimAt(m_ladderMoveGoal, IPlayerController::LOOK_MOVEMENT);
		m_ladderTimer.Start(4.0f);
		SetDesiredSpeed(GetWalkSpeed());
		return;
	}
	case IMovement::USING_LADDER_UP:
	{
		// calculate Z goal

		if (m_ladder->GetLadderType() == CNavLadder::LadderType::USEABLE_LADDER)
		{
			m_ladderGoalZ = m_ladder->GetConnectionToArea(m_ladderExit)->GetConnectionPoint().z - (GetStepHeight() * 0.5f);
		}
		else
		{
			// for simple ladders, try to go a little above it
			m_ladderGoalZ = m_ladder->GetConnectionToArea(m_ladderExit)->GetConnectionPoint().z + GetCrouchedHullHeight();

			if (m_ladderGoalZ >= m_ladder->m_top.z)
			{
				m_ladderGoalZ = m_ladder->m_top.z - (GetStepHeight() * 0.5f);
			}
		}

		
		float time = m_ladder->m_length / LADDER_TIME_DIVIDER;
		m_ladderTimer.Start(time);
		SetDesiredSpeed(GetRunSpeed());
		return;
	}
	case IMovement::APPROACHING_LADDER_DOWN:
	{
		m_ladderTimer.Start(4.0f);
		Vector lookAt = m_ladder->m_top;
		lookAt.z = m_ladder->ClampZ(me->GetAbsOrigin().z);
		me->GetControlInterface()->SnapAimAt(lookAt, IPlayerController::LOOK_MOVEMENT);
		SetDesiredSpeed(GetWalkSpeed());
		return;
	}
	case IMovement::USING_LADDER_DOWN:
	{
		// calculate Z goal
		m_ladderGoalZ = m_ladder->GetConnectionToArea(m_ladderExit)->GetConnectionPoint().z + (GetStepHeight() * 0.5f);
		Vector lookAt = m_ladder->m_top;
		lookAt.z = m_ladderGoalZ;
		me->GetControlInterface()->SnapAimAt(lookAt, IPlayerController::LOOK_MOVEMENT);
		float time = m_ladder->m_length / LADDER_TIME_DIVIDER;
		m_ladderTimer.Start(time);
		SetDesiredSpeed(GetRunSpeed());
		return;
	}
	default:
		return;
	}
}

IMovement::ElevatorState IMovement::EState_MoveToWaitPosition()
{
	/* The first state for riding elevators. 
	* The bot will move to the wait position first for aligment purposes.
	*/

	CBaseBot* me = GetBot();

	if (m_fromFloor->wait_position.IsZero(0.5f))
	{
		m_elevatorTimeout.Invalidate();
		return ElevatorState::CALL_ELEVATOR;
	}

	Vector myPos = me->GetAbsOrigin();
	float zDiff = std::fabs(myPos.z - m_fromFloor->wait_position.z);

	if (zDiff > GetStepHeight())
	{
		if (me->IsDebugging(BOTDEBUG_MOVEMENT))
		{
			me->DebugPrintToConsole(255, 0, 0, "%s <Elevator Move to Wait Position> Z diff > step height!\n    %3.2f > %3.2f \n    (%3.2f, %3.2f, %3.2f) (%3.2f, %3.2f, %3.2f) \n", 
				me->GetDebugIdentifier(), zDiff, GetStepHeight(), myPos.x, myPos.y, myPos.z, m_fromFloor->wait_position.x, m_fromFloor->wait_position.y, m_fromFloor->wait_position.z);
		}

		return ElevatorState::NOT_USING_ELEVATOR;
	}

	if (me->GetRangeTo(m_fromFloor->wait_position) < IMovement::ELEV_MOVE_RANGE)
	{
		if (m_elevator->GetType() == CNavElevator::ElevatorType::AUTO_TRIGGER)
		{
			return ElevatorState::ENTER_ELEVATOR; // activated by a trigger, just enter it
		}

		m_elevatorTimeout.Invalidate();
		return ElevatorState::CALL_ELEVATOR;
	}

	MoveTowards(m_fromFloor->wait_position, MOVEWEIGHT_CRITICAL);

	return ElevatorState::MOVE_TO_WAIT_POS;
}

IMovement::ElevatorState IMovement::EState_CallElevator()
{
	/* Move towars the elevator call button and press it. Fails if the call button is invalid. */

	if (!m_elevatorTimeout.HasStarted())
	{
		float time = m_fromFloor->GetDistanceTo(m_toFloor) / m_elevator->GetSpeed();
		float extra = m_elevator->IsMultiFloorElevator() ? 10.0f : 5.0f; // 10 extra seconds for multi floor type, else 5

		m_elevatorTimeout.Start(time + extra);
	}

	if (m_elevatorTimeout.IsElapsed())
	{
		if (GetBot()->IsDebugging(BOTDEBUG_MOVEMENT))
		{
			GetBot()->DebugPrintToConsole(255, 105, 180, "%s Call Elevator timed out!\n", GetBot()->GetDebugIdentifier());
		}

		return ElevatorState::NOT_USING_ELEVATOR;
	}

	if (m_fromFloor->is_here)
	{
		m_elevatorTimeout.Invalidate();
		return ElevatorState::ENTER_ELEVATOR;
	}

	CBaseEntity* callbutton = m_fromFloor->call_button.handle.Get();
	CBaseBot* bot = GetBot();
	float minrange = (CBaseExtPlayer::PLAYER_USE_RADIUS * 0.7f);

	if (m_fromFloor->shootable_button)
	{
		bot->GetInventoryInterface()->SelectBestHitscanWeapon();
		auto weapon = bot->GetInventoryInterface()->GetActiveBotWeapon();

		if (weapon)
		{
			minrange = weapon->GetWeaponInfo()->GetAttackInfo(WeaponInfo::AttackFunctionType::PRIMARY_ATTACK).GetMaxRange();
		}
	}

	if (callbutton == nullptr)
	{
		return ElevatorState::NOT_USING_ELEVATOR;
	}

	{
		Vector pos = UtilHelpers::getWorldSpaceCenter(callbutton);
		MoveTowards(pos, MOVEWEIGHT_CRITICAL);
		bot->GetControlInterface()->AimAt(pos, IPlayerController::LOOK_MOVEMENT, 0.1f, "Looking at elevator call button!");

		if ((bot->GetEyeOrigin() - pos).IsLengthLessThan(minrange))
		{
			if (bot->GetControlInterface()->IsAimOnTarget())
			{
				if (m_fromFloor->shootable_button)
				{
					bot->GetControlInterface()->PressAttackButton(0.2f);
				}
				else
				{
					bot->GetControlInterface()->PressUseButton();
				}

				return ElevatorState::WAIT_FOR_ELEVATOR;
			}
		}
	}

	return ElevatorState::CALL_ELEVATOR;
}

IMovement::ElevatorState IMovement::EState_WaitForElevator()
{
	/* 
	 * The call button has been pressed, wait for the elevator to arrive at my current floor 
	 * Timeout timer for safety.
	 */

	if (!m_elevatorTimeout.HasStarted())
	{
		float time = m_fromFloor->GetDistanceTo(m_toFloor) / m_elevator->GetSpeed();
		float extra = m_elevator->IsMultiFloorElevator() ? 10.0f : 5.0f; // 10 extra seconds for multi floor type, else 5

		m_elevatorTimeout.Start(time + extra);
	}

	if (m_elevatorTimeout.IsElapsed())
	{
		if (GetBot()->IsDebugging(BOTDEBUG_MOVEMENT))
		{
			GetBot()->DebugPrintToConsole(255, 105, 180, "%s Wait Elevator timed out!\n", GetBot()->GetDebugIdentifier());
		}

		return ElevatorState::NOT_USING_ELEVATOR;
	}

	CBaseBot* me = GetBot();

	// elevator has arrived, time to enter it
	if (m_fromFloor->is_here)
	{
		m_elevatorTimeout.Invalidate();
		return ElevatorState::ENTER_ELEVATOR;
	}

	// move back to wait position
	if (m_fromFloor->wait_position.IsZero(0.5f))
	{
		return ElevatorState::WAIT_FOR_ELEVATOR;
	}

	if (me->GetRangeTo(m_fromFloor->wait_position) > IMovement::ELEV_MOVE_RANGE)
	{
		MoveTowards(m_fromFloor->wait_position, MOVEWEIGHT_CRITICAL);
	}

	return ElevatorState::WAIT_FOR_ELEVATOR;
}

IMovement::ElevatorState IMovement::EState_EnterElevator()
{
	// get inside the elevator (elevator nav area center)

	CBaseBot* me = GetBot();

	const Vector& pos = m_fromFloor->GetArea()->GetCenter();
	MoveTowards(pos, MOVEWEIGHT_CRITICAL);

	float zDiff = fabs(pos.z - me->GetAbsOrigin().z);

	if (zDiff > GetMaxJumpHeight())
	{
		return ElevatorState::NOT_USING_ELEVATOR;
	}

	if (me->GetRangeTo(pos) <= ELEV_MOVE_RANGE)
	{
		return ElevatorState::OPERATE_ELEVATOR;
	}

	return ElevatorState::ENTER_ELEVATOR;
}

IMovement::ElevatorState IMovement::EState_OperateElevator()
{
	// press the elevator use button, fail if it doesn't have one

	CBaseEntity* button = m_fromFloor->use_button.handle.Get();

	if (button == nullptr)
	{
		return NOT_USING_ELEVATOR;
	}

	CBaseBot* me = GetBot();
	Vector pos = UtilHelpers::getWorldSpaceCenter(button);
	float minrange = (CBaseExtPlayer::PLAYER_USE_RADIUS * 0.9f);

	if (m_fromFloor->shootable_button)
	{
		me->GetInventoryInterface()->SelectBestHitscanWeapon();
		auto weapon = me->GetInventoryInterface()->GetActiveBotWeapon();

		if (weapon)
		{
			minrange = weapon->GetWeaponInfo()->GetAttackInfo(WeaponInfo::AttackFunctionType::PRIMARY_ATTACK).GetMaxRange();
		}
	}

	MoveTowards(pos, MOVEWEIGHT_CRITICAL);
	me->GetControlInterface()->AimAt(pos, IPlayerController::LOOK_MOVEMENT, 0.1f, "Looking at elevator use button!");

	if ((me->GetEyeOrigin() - pos).IsLengthLessThan(minrange))
	{
		if (me->GetControlInterface()->IsAimOnTarget())
		{
			if (m_fromFloor->shootable_button)
			{
				me->GetControlInterface()->PressAttackButton(0.2f);
			}
			else
			{
				me->GetControlInterface()->PressUseButton();
			}

			m_elevatorTimeout.Invalidate();
			return ElevatorState::RIDE_ELEVATOR;
		}
	}

	return ElevatorState::OPERATE_ELEVATOR;
}

IMovement::ElevatorState IMovement::EState_RideElevator()
{
	// ride the elevator by standing still
	// fail on timeout

	if (!m_elevatorTimeout.HasStarted())
	{
		float time = m_fromFloor->GetDistanceTo(m_toFloor) / m_elevator->GetSpeed();
		float extra = m_elevator->IsMultiFloorElevator() ? 10.0f : 5.0f; // 10 extra seconds for multi floor type, else 5

		m_elevatorTimeout.Start(time + extra);
		GetBot()->GetControlInterface()->AimAt(m_toFloor->GetArea()->GetCenter(), IPlayerController::LOOK_MOVEMENT, 0.5f, "Riding elevator!");
	}

	if (m_elevatorTimeout.IsElapsed())
	{
		if (GetBot()->IsDebugging(BOTDEBUG_MOVEMENT))
		{
			GetBot()->DebugPrintToConsole(255, 105, 180, "%s Ride Elevator timed out!\n", GetBot()->GetDebugIdentifier());
		}

		return ElevatorState::NOT_USING_ELEVATOR;
	}

	// additional checks for multi floor elevators
	if (m_elevator->IsMultiFloorElevator())
	{
		// get the floor the elevator is stopped at
		auto currentFloor = m_elevator->GetStoppedFloor();

		// if the elevator is stopped and is not my goal floor or my initial floor, operate it again
		if (currentFloor != nullptr && currentFloor != m_toFloor && currentFloor != m_fromFloor)
		{
			// A multifloor elevator has stopped, press the use button again

			if (!currentFloor->HasUseButton())
			{
				return ElevatorState::NOT_USING_ELEVATOR;
			}

			if (GetBot()->IsDebugging(BOTDEBUG_MOVEMENT))
			{
				GetBot()->DebugPrintToConsole(255, 105, 180, "%s Multi-Floor elevator is stopped but I am not at my destination! Operating elevator again!\n", GetBot()->GetDebugIdentifier());
			}

			m_fromFloor = currentFloor; // update from floor to the current floor
			m_elevatorTimeout.Invalidate();
			return ElevatorState::OPERATE_ELEVATOR; // go to operate state to press the new use button
		}
	}

	// elevator arrived at my target, exit it!
	if (m_toFloor->is_here)
	{
		m_elevatorTimeout.Invalidate();
		return ElevatorState::EXIT_ELEVATOR;
	}

	return ElevatorState::RIDE_ELEVATOR;
}

IMovement::ElevatorState IMovement::EState_ExitElevator()
{
	CBaseBot* me = GetBot();

	// if no wait position, just end it.
	if (m_toFloor->wait_position.IsZero(0.5f))
	{
		return ElevatorState::NOT_USING_ELEVATOR;
	}

	float zDiff = fabs(m_toFloor->wait_position.z - me->GetAbsOrigin().z);

	if (zDiff > GetMaxJumpHeight())
	{
		return ElevatorState::NOT_USING_ELEVATOR;
	}

	// move to exit position
	MoveTowards(m_toFloor->wait_position, MOVEWEIGHT_CRITICAL);

	// exit position reached, time to go back to normal navigation
	if (me->GetRangeTo(m_toFloor->wait_position) < IMovement::ELEV_MOVE_RANGE)
	{
		return ElevatorState::NOT_USING_ELEVATOR;
	}

	return ElevatorState::EXIT_ELEVATOR;
}

IMovement::StrafeJumpState IMovement::StrafeJump_Init()
{
	CBaseBot* me = GetBot<CBaseBot>();

	// fell before the bot could jump
	if (!IsOnGround())
	{
		return NOT_STRAFE_JUMPING;
	}

	const Vector start = me->GetAbsOrigin();
	Vector dir = UtilHelpers::math::BuildDirectionVector(start, GetStrafeJumpMidPoint());
	float speed = GetRunSpeed();
	Vector velocity = me->GetAbsVelocity();
	// cheat: make the bot be at full speed during the initial jump and also aligned
	velocity.x = speed * dir.x;
	velocity.y = speed * dir.y;
	me->SetAbsVelocity(velocity);
	CrouchJump();

	return STRAFEJUMP_TO_MIDPOINT;
}

static void CalculateStrafeJumpLookAtPos(const bool isLeft, const Vector& veldir, const Vector& origin, Vector& out)
{
	float angleOffset = sm_navbot_movement_strafejump_look_angle_offset.GetFloat();

	if (isLeft)
	{
		angleOffset *= -1.0f; // when strafing to the left, look to the right
	}

	QAngle angle{ 0.0f, angleOffset, 0.0f };
	Vector dir;
	VectorRotate(veldir, angle, dir);
	out = origin + (dir * 128.0f);
}

IMovement::StrafeJumpState IMovement::StrafeJump_ToMidPoint()
{
	CBaseBot* me = GetBot<CBaseBot>();
	// const float delta = me->GetLastUpdateTimeDelta() * 3.0f;
	MoveTowards(GetStrafeJumpEndPoint(), MOVEWEIGHT_CRITICAL);
	// AccelerateTowards(GetStrafeJumpMidPoint(), nullptr, &delta);
	const Vector origin = me->GetEyeOrigin();
	Vector veldir = me->GetAbsVelocity();
	veldir.z = 0.0f;
	veldir.NormalizeInPlace();
	Vector lookat;
	CalculateStrafeJumpLookAtPos(m_sjIsToTheLeft, veldir, origin, lookat);
	me->GetControlInterface()->AimAt(lookat, IPlayerController::LOOK_MOVEMENT, 0.2f, "Strafe jumping!");

	const Vector current = me->GetAbsOrigin();
	const float dist = (current - GetStrafeJumpMidPoint()).AsVector2D().Length();

	if (dist <= (GetHullWidth() * 0.5f))
	{
		if (me->IsDebugging(BOTDEBUG_MOVEMENT))
		{
			me->DebugPrintToConsole(0, 180, 0, "%s REACHED STRAFE JUMP MIDPOINT <%g %g %g> (%g) \n", me->GetDebugIdentifier(), current.x, current.y, current.z, dist);
		}

		Vector vel = me->GetAbsVelocity();
		vel.z = 0.0f;
		const float myspeed = vel.Length();
		const float launchspeed = std::max(myspeed, GetWalkSpeed());

		Vector launch = me->CalculateLaunchVector(m_sjEndPoint, launchspeed);
		me->SetAbsVelocity(launch);
		return STRAFEJUMP_TO_ENDPOINT;
	}

	// fell and hit ground
	if (IsOnGround())
	{
		return NOT_STRAFE_JUMPING;
	}

	return STRAFEJUMP_TO_MIDPOINT;
}

IMovement::StrafeJumpState IMovement::StrafeJump_ToEndPoint()
{
	CBaseBot* me = GetBot<CBaseBot>();
	MoveTowards(GetStrafeJumpEndPoint(), MOVEWEIGHT_CRITICAL);
	// const float delta = std::min(me->GetLastUpdateTimeDelta() * 8.0f, 1.0f);
	// AccelerateTowards(GetStrafeJumpEndPoint(), nullptr, &delta);
	// Vector lookat = GetStrafeJumpEndPoint();
	// lookat.z += GetCrouchedHullHeight();
	// me->GetControlInterface()->AimAt(lookat, IPlayerController::LOOK_MOVEMENT, 0.5f, "Strafe jumping!");

	const Vector origin = me->GetEyeOrigin();
	Vector veldir = me->GetAbsVelocity();
	veldir.z = 0.0f;
	veldir.NormalizeInPlace();
	Vector lookat;
	CalculateStrafeJumpLookAtPos(m_sjIsToTheLeft, veldir, origin, lookat);
	me->GetControlInterface()->AimAt(lookat, IPlayerController::LOOK_MOVEMENT, 0.2f, "Strafe jumping!");

	if (IsOnGround())
	{
		if (me->IsDebugging(BOTDEBUG_MOVEMENT))
		{
			const Vector current = me->GetAbsOrigin();
			const float zdiff = std::abs(current.z - m_sjEndPoint.z);
			me->DebugPrintToConsole(0, 180, 0, "%s STRAFE JUMP COMPLETE: ON GROUND (%g) \n", me->GetDebugIdentifier(), zdiff);
		}

		me->UpdateLastKnownNavArea(true);
		return NOT_STRAFE_JUMPING;
	}

	return STRAFEJUMP_TO_ENDPOINT;
}

void IMovement::_Reset()
{
	m_ladder = nullptr;
	m_ladderExit = nullptr;
	m_ladderState = NOT_USING_LADDER;
	m_ladderTimer.Invalidate();
	m_useLadderTimer.Invalidate();
	m_landingGoal = vec3_origin;
	m_jumpCooldown.Invalidate();
	m_jumpTimer.Invalidate();
	m_doMidAirCJ.Invalidate();
	m_isJumping = false;
	m_isClimbingObstacle = false;
	m_isJumpingAcrossGap = false;
	m_isAirborne = false;
	m_isUsingCatapult = false;
	m_wasLaunched = false;
	m_catapultCorrectVelocityTimer.Invalidate();
	m_stuck.Reset();
	m_motionVector = vec3_origin;
	m_groundMotionVector = vec2_origin;
	m_speed = 0.0f;
	m_groundspeed = 0.0f;
	m_maxspeed = GetBot()->GetMaxSpeed();
	m_desiredspeed = m_maxspeed;
	m_ladderGoalZ = 0.0f;
	m_elevator = nullptr;
	m_fromFloor = nullptr;
	m_toFloor = nullptr;
	m_elevatorTimeout.Invalidate();
	m_elevatorState = NOT_USING_ELEVATOR;
	m_lastMoveWeight = 0;
	m_isBreakingObstacle = false;
	m_crouchToBreak = false;
	m_obstacleBreakTimeout.Invalidate();
	m_obstacleEntity = nullptr;
	m_catapultStartPosition = vec3_origin;
	m_isStopAndWait = false;
	m_stopAndWaitTimer.Invalidate();
	m_strafeJumpState = NOT_STRAFE_JUMPING;
	m_sjMidPoint = vec3_origin;
	m_sjEndPoint = vec3_origin;
	m_movementType = IMovement::MovementType::MOVE_RUNNING;
	m_lastMTRequestPriority = IMovement::MovementRequestPriority::MOVEREQUEST_PRIO_LOW;
	m_MTRequestTimer.Invalidate();
}


