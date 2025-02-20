#include <cstring>
#include <extension.h>
#include <sdkports/debugoverlay_shared.h>
#include <bot/interfaces/playercontrol.h>
#include <bot/basebot.h>
#include <navmesh/nav_mesh.h>
#include <navmesh/nav_area.h>
#include <navmesh/nav_ladder.h>
#include <manager.h>
#include <mods/basemod.h>
#include <util/entprops.h>
#include <util/helpers.h>
#include <entities/baseentity.h>
#include "movement.h"

constexpr auto DEFAULT_PLAYER_STANDING_HEIGHT = 72.0f;
constexpr auto DEFAULT_PLAYER_DUCKING_HEIGHT = 36.0f;
constexpr auto DEFAULT_PLAYER_HULL_WIDTH = 32.0f;

CMovementTraverseFilter::CMovementTraverseFilter(CBaseBot* bot, IMovement* mover, const bool now) :
	trace::CTraceFilterSimple(bot->GetEntity(), COLLISION_GROUP_NONE)
{
	m_me = bot;
	m_mover = mover;
	m_now = now;
}

bool CMovementTraverseFilter::ShouldHitEntity(int entity, CBaseEntity* pEntity, edict_t* pEdict, const int contentsMask)
{
	if (pEntity != nullptr && pEdict != nullptr)
	{
		if (pEntity == m_me->GetEntity())
		{
			return false; // Don't hit myself
		}

		if (CTraceFilterSimple::ShouldHitEntity(entity, pEntity, pEdict, contentsMask))
		{
			return !m_mover->IsEntityTraversable(entity, pEdict, pEntity, m_now);
		}
	}

	return false;
}

CTraceFilterOnlyActors::CTraceFilterOnlyActors(CBaseEntity* pPassEnt, int collisionGroup) :
	trace::CTraceFilterSimple(pPassEnt, collisionGroup)
{
}

bool CTraceFilterOnlyActors::ShouldHitEntity(int entity, CBaseEntity* pEntity, edict_t* pEdict, const int contentsMask)
{
	if (trace::CTraceFilterSimple::ShouldHitEntity(entity, pEntity, pEdict, contentsMask))
	{
		return UtilHelpers::IsPlayerIndex(entity);
	}

	return false;
}

IMovement::IMovement(CBaseBot* bot) : IBotInterface(bot)
{
	m_jump_zboost_timer.Invalidate();
	m_jumptimer.Invalidate();
	m_ladder = nullptr;
	m_ladderExit = nullptr;
	m_ladderState = NOT_USING_LADDER;
	m_ladderTimer.Invalidate();
	m_useLadderTimer.Invalidate();
	m_landingGoal = vec3_origin;
	m_isClimbingObstacle = false;
	m_isJumpingAcrossGap = false;
	m_isAirborne = false;
	m_stuck.Reset();
	m_motionVector = vec3_origin;
	m_groundMotionVector = vec2_origin;
	m_speed = 0.0f;
	m_groundspeed = 0.0f;
	m_maxspeed = 0.0f;
	m_ladderGoalZ = 0.0f;
	m_elevator = nullptr;
	m_fromFloor = nullptr;
	m_toFloor = nullptr;
	m_elevatorTimeout.Invalidate();
	m_elevatorState = NOT_USING_ELEVATOR;
	m_lastMoveWeight = 0;
	m_isBreakingObstacle = false;
	m_obstacleBreakTimeout.Invalidate();
	m_obstacleEntity = nullptr;
	m_desiredspeed = 0.0f;
}

IMovement::~IMovement()
{
}

void IMovement::Reset()
{
	m_jump_zboost_timer.Invalidate();
	m_jumptimer.Invalidate();
	m_ladder = nullptr;
	m_ladderExit = nullptr;
	m_ladderState = NOT_USING_LADDER;
	m_ladderTimer.Invalidate();
	m_useLadderTimer.Invalidate();
	m_landingGoal = vec3_origin;
	m_isClimbingObstacle = false;
	m_isJumpingAcrossGap = false;
	m_isAirborne = false;
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
	m_obstacleBreakTimeout.Invalidate();
	m_obstacleEntity = nullptr;
}

void IMovement::Update()
{
	m_maxspeed = GetBot()->GetMaxSpeed();

	StuckMonitor();
	ObstacleBreakUpdate();

	auto velocity = GetBot()->GetAbsVelocity();
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

	if (IsUsingLadder())
	{
		return;
	}

	// the code below should not run when the bot is using ladders

	if (m_isJumpingAcrossGap || m_isClimbingObstacle)
	{
		Vector toLanding = m_landingGoal - GetBot()->GetAbsOrigin();
		toLanding.z = 0.0f;
		toLanding.NormalizeInPlace();

		if (m_isAirborne)
		{
			GetBot()->GetControlInterface()->AimAt(GetBot()->GetEyeOrigin() + 100.0f * toLanding, IPlayerController::LOOK_MOVEMENT, 0.25f);

			if (IsOnGround())
			{
				// jump complete
				m_isJumpingAcrossGap = false;
				m_isClimbingObstacle = false;
				m_isAirborne = false;
			}
		}
		else // not airborne, jump!
		{
			if (!IsClimbingOrJumping())
			{
				Jump();
			}

			if (!IsOnGround())
			{
				m_isAirborne = true;
			}
		}

		MoveTowards(m_landingGoal, MOVEWEIGHT_CRITICAL);
	}
}

void IMovement::Frame()
{
	if (m_jump_zboost_timer.HasStarted() && m_jump_zboost_timer.IsElapsed())
	{
		Vector vel = GetBot()->GetAbsVelocity();
		vel.z = GetCrouchJumpZBoost();
		GetBot()->SetAbsVelocity(vel);

		if (GetBot()->IsDebugging(BOTDEBUG_MOVEMENT))
		{
			GetBot()->DebugPrintToConsole(BOTDEBUG_MOVEMENT, 200, 255, 200, "%s Crouch Jump! (JUMP) \n", GetBot()->GetDebugIdentifier());
		}

		m_jump_zboost_timer.Invalidate();
	}

	if (m_braketimer.HasStarted() && !m_braketimer.IsElapsed())
	{
		// Cheat: force velocity to zero. The correct way would be to setup counter-strafing
		Vector vel(0.0f, 0.0f, 0.0f);
		GetBot()->SetAbsVelocity(vel);
	}

	// Was on Update, moved to frame
	TraverseLadder();
	ElevatorUpdate();
}

float IMovement::GetHullWidth()
{
	float scale = GetBot()->GetModelScale();
	return DEFAULT_PLAYER_HULL_WIDTH * scale;
}

float IMovement::GetStandingHullHeigh()
{
	float scale = GetBot()->GetModelScale();
	return DEFAULT_PLAYER_STANDING_HEIGHT * scale;
}

float IMovement::GetCrouchedHullHeigh()
{
	float scale = GetBot()->GetModelScale();
	return DEFAULT_PLAYER_DUCKING_HEIGHT * scale;
}

float IMovement::GetProneHullHeigh()
{
	return 0.0f; // implement if mod has prone support (IE: DoD:S)
}

unsigned int IMovement::GetMovementTraceMask()
{
	return MASK_PLAYERSOLID;
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
	if (m_lastMoveWeight > weight)
	{
		return;
	}

	m_lastMoveWeight = weight;

	auto me = GetBot();
	auto input = me->GetControlInterface();
	auto origin = me->GetAbsOrigin();
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
		NDebugOverlay::Line(origin, pos, 0, 255, 0, false, 0.2f);
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

void IMovement::FaceTowards(const Vector& pos, const bool important)
{
	auto input = GetBot()->GetControlInterface();
	auto eyes = GetBot()->GetEyeOrigin();
	IPlayerController::LookPriority priority = important ? IPlayerController::LookPriority::LOOK_MOVEMENT : IPlayerController::LookPriority::LOOK_NONE;

	Vector lookAt(pos.x, pos.y, eyes.z);
	input->AimAt(lookAt, priority, 0.1f, "IMovement::FaceTowards");
}

void IMovement::AdjustSpeedForPath(CMeshNavigator* path)
{
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
	ChangeLadderState(LadderState::APPROACHING_LADDER_UP);

	auto bot = GetBot();
	
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
	GetBot()->GetControlInterface()->PressJumpButton();
	m_jumptimer.Start(0.8f);
}

void IMovement::CrouchJump()
{
	// Execute a crouch jump
	// See shounic's video https://www.youtube.com/watch?v=7z_p_RqLhkA

	// First the bot will crouch
	GetBot()->GetControlInterface()->PressCrouchButton(0.7f); // hold the crouch button for a while
	GetBot()->GetControlInterface()->PressJumpButton(0.3f);

	if (IsOnGround()) // prevents adding the Z boost while on the air
	{
		// This is a tick timer, the bot will jump when it reaches 0
		m_jump_zboost_timer.Start(0.150f);
	}

	m_jumptimer.Start(0.8f); // Timer for 'Is the bot performing a jump'

	if (GetBot()->IsDebugging(BOTDEBUG_MOVEMENT))
	{
		GetBot()->DebugPrintToConsole(BOTDEBUG_MOVEMENT, 200, 255, 200, "%s Crouch Jump! (CROUCH) \n", GetBot()->GetDebugIdentifier());
	}
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
	GetBot()->GetControlInterface()->AimAt(landing, IPlayerController::LOOK_MOVEMENT, 1.0f);

	Jump();

	m_isJumpingAcrossGap = true;
	m_landingGoal = landing;
	m_isAirborne = false;

	if (GetBot()->IsDebugging(BOTDEBUG_MOVEMENT))
	{
		Vector top = GetBot()->GetAbsOrigin();
		top.z += GetMaxJumpHeight();

		NDebugOverlay::VertArrow(GetBot()->GetAbsOrigin(), top, 5.0f, 0, 0, 255, 255, false, 5.0f);
		NDebugOverlay::HorzArrow(top, landing, 5.0f, 0, 255, 0, 255, false, 5.0f);
	}
}

bool IMovement::ClimbUpToLedge(const Vector& landingGoal, const Vector& landingForward, edict_t* obstacle)
{
	CrouchJump(); // Always do a crouch jump

	m_isClimbingObstacle = true;
	m_landingGoal = landingGoal;
	m_isAirborne = false;

	return true;
}

bool IMovement::DoubleJumpToLedge(const Vector& landingGoal, const Vector& landingForward, edict_t* obstacle)
{
	if (!IsAbleToDoubleJump())
		return false;

	DoubleJump();

	m_isClimbingObstacle = true;
	m_landingGoal = landingGoal;
	m_isAirborne = false;

	return true;
}

bool IMovement::IsAbleToClimbOntoEntity(edict_t* entity)
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
	Vector maxs(hullsize, hullsize, GetCrouchedHullHeigh());
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

bool IMovement::HasPotentialGap(const Vector& from, const Vector& to, float& fraction)
{
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
			fraction = (f - step) / (length + step);
			return true;
		}

		start += delta;
	}

	fraction = 1.0f;
	return false;
}

bool IMovement::IsEntityTraversable(int index, edict_t* edict, CBaseEntity* entity, const bool now)
{
	if (index == 0) // index 0 is the world
	{
		return false;
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

		// https://github.com/ValveSoftware/source-sdk-2013/blob/0d8dceea4310fde5706b3ce1c70609d72a38efdf/sp/src/game/server/BasePropDoor.h#L84
		constexpr auto DOOR_STATE_OPEN = 2; // value taken from enum at BasePropDoor.h

		if (doorstate == DOOR_STATE_OPEN)
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

	return GetBot()->IsAbleToBreak(edict);
}

bool IMovement::IsOnGround()
{
	return GetBot()->GetGroundEntity() != nullptr;
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
	Vector maxs(hullwidth, hullwidth, GetStandingHullHeigh());
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

void IMovement::TryToUnstuck()
{
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

		return;
	}

	Vector start = bot->GetEyeOrigin();
	Vector end(start.x, start.y, start.z + 64.0f);
	trace::CTraceFilterNoNPCsOrPlayers filter(bot->GetEntity(), COLLISION_GROUP_PLAYER_MOVEMENT);
	trace_t tr;
	trace::line(start, end, MASK_PLAYERSOLID, &filter, tr);

	if (!tr.DidHit())
	{
		CrouchJump();
	}
}

void IMovement::ObstacleOnPath(CBaseEntity* obstacle, const Vector& goalPos, const Vector& forward, const Vector& left)
{
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
	return true;
}

bool IMovement::IsUseableObstacle(CBaseEntity* entity)
{
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
	else if (std::strcmp(classname, "prop_door_rotating") == 0)
	{
		return true;
	}

	return false;
}

void IMovement::StuckMonitor()
{
	auto bot = GetBot();
	auto origin = bot->GetAbsOrigin();
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
					bot->DelayedFakeClientCommand("kill");
				}

				if (bot->IsDebugging(BOTDEBUG_MOVEMENT))
				{
					bot->DebugPrintToConsole(BOTDEBUG_MOVEMENT, 200, 255, 200, "%s STILL STUCK! \n", bot->GetDebugIdentifier());
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
					bot->DebugPrintToConsole(BOTDEBUG_MOVEMENT, 200, 255, 200, "%s GOT STUCK! \n", bot->GetDebugIdentifier());
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
		m_obstacleBreakTimeout.Invalidate();
		m_isBreakingObstacle = false;
		m_obstacleEntity = nullptr;
		GetBot()->GetControlInterface()->ReleaseAllAttackButtons();
		return;
	}

	CBaseBot* bot = GetBot();
	Vector pos = UtilHelpers::getWorldSpaceCenter(obstacle);
	bot->GetInventoryInterface()->SelectBestWeaponForBreakables();

	bot->GetControlInterface()->AimAt(pos, IPlayerController::LOOK_MOVEMENT, 0.5f, "Looking at breakable entity!");
	MoveTowards(pos, MOVEWEIGHT_CRITICAL);

	if (bot->GetControlInterface()->IsAimOnTarget())
	{
		bot->GetControlInterface()->PressAttackButton();
	}
}

// approach a ladder that we will go up
IMovement::LadderState IMovement::ApproachUpLadder()
{
	if (m_ladder == nullptr)
	{
		return NOT_USING_LADDER;
	}
	
	// the ladder should always have a connection to the ladder exit area, if not let it crash to notify a programmer
	const LadderToAreaConnection* connection = m_ladder->GetConnectionToArea(m_ladderExit);

	if (IsOnLadder())
	{
		GetBot()->GetControlInterface()->ReleaseMovementButtons();

		if (!m_ladderWait.HasStarted())
		{
			m_ladderWait.Start(0.25f);
			GetBot()->DebugPrintToConsole(BOTDEBUG_MOVEMENT, 0, 200, 200, "%s GRABBED LADDER (UP)! \n", GetBot()->GetDebugIdentifier());
		}
		else if (m_ladderWait.IsElapsed())
		{
			GetBot()->DebugPrintToConsole(BOTDEBUG_MOVEMENT, 0, 200, 200, "%s WAIT TIMER EXPIRED (UP)! \n", GetBot()->GetDebugIdentifier());
			return USING_LADDER_UP; // wait timer expired, we are on a ladder
		}
	}
	else
	{
		// move to the ladder connection point (this is on the ladder)
		MoveTowards(m_ladderMoveGoal, MOVEWEIGHT_CRITICAL);
		FaceTowards(m_ladderMoveGoal + Vector(0.0f, 0.0f, navgenparams->human_eye_height), true);

		if (GetBot()->GetRangeTo(connection->GetConnectionPoint()) < CBaseExtPlayer::PLAYER_USE_RADIUS)
		{
			if (m_useLadderTimer.IsElapsed())
			{
				GetBot()->GetControlInterface()->PressUseButton();
				m_useLadderTimer.Start(0.2f);
			}
		}
	}

	if (GetBot()->IsDebugging(BOTDEBUG_MOVEMENT))
	{
		NDebugOverlay::Cross3D(m_ladderMoveGoal, 24.0f, 0, 128, 0, true, NDEBUG_PERSIST_FOR_ONE_TICK);
		NDebugOverlay::Line(GetBot()->GetAbsOrigin(), m_ladderMoveGoal, 0, 128, 0, true, NDEBUG_PERSIST_FOR_ONE_TICK);
	}

	return APPROACHING_LADDER_UP;
}

IMovement::LadderState IMovement::ApproachDownLadder()
{
	if (m_ladder == nullptr)
	{
		return NOT_USING_LADDER;
	}

	auto origin = GetBot()->GetAbsOrigin();
	const float z = origin.z;
	const bool debug = GetBot()->IsDebugging(BOTDEBUG_MOVEMENT);

	if (z <= m_ladder->m_bottom.z + GetMaxJumpHeight())
	{
		m_ladderTimer.Start(2.0f);
		return EXITING_LADDER_DOWN;
	}

	Vector climbPoint;

	if (m_ladder->GetLadderType() == CNavLadder::USEABLE_LADDER)
	{
		// useable ladder top/bottom positions are at the center of the ladder
		climbPoint = m_ladder->m_top;
	}
	else
	{
		climbPoint = m_ladder->m_top + (0.25f * GetHullWidth() * m_ladder->GetNormal());
	}

	climbPoint.z = m_ladder->ClampZ(z - (GetStepHeight() / 4.0f));

	Vector to = climbPoint - origin;
	to.z = 0.0f;
	float mountRange = to.NormalizeInPlace();
	Vector moveGoal = climbPoint;

	if (debug)
	{
		NDebugOverlay::VertArrow(GetBot()->GetAbsOrigin(), moveGoal, 4.0f, 0, 255, 0, 255, false, 0.2f);
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
	if (m_ladder == nullptr || !IsOnLadder() || m_ladderTimer.IsElapsed())
	{
		return NOT_USING_LADDER;
	}

	auto input = GetBot()->GetControlInterface();
	auto origin = GetBot()->GetAbsOrigin();
	const float z = origin.z;

	// Bot is not at the top of the ladder but is above the landing nav area height plus some offset distance
	if (z >= m_ladderGoalZ)
	{

		if (GetBot()->IsDebugging(BOTDEBUG_MOVEMENT))
		{
			GetBot()->DebugPrintToConsole(0, 128, 0, "%s: USE LADDER (UP) REACHED Z GOAL (z >= m_ladderGoalZ)\n", GetBot()->GetDebugIdentifier());
		}

		return EXITING_LADDER_UP;
	}
	else if (z >= m_ladder->m_top.z)
	{
		// reached ladder top

		if (GetBot()->IsDebugging(BOTDEBUG_MOVEMENT))
		{
			GetBot()->DebugPrintToConsole(0, 128, 0, "%s: USE LADDER (UP) REACHED Z GOAL (z >= m_ladder->m_top.z)\n", GetBot()->GetDebugIdentifier());
		}

		return EXITING_LADDER_UP;
	}
	else
	{
		Vector goal = origin + 100.0f * (-m_ladder->GetNormal() + Vector(0.0f, 0.0f, 2.0f));
		input->AimAt(goal, IPlayerController::LOOK_MOVEMENT, 0.1f, "Going up a ladder.");
		MoveTowards(goal, MOVEWEIGHT_CRITICAL);

		if (GetBot()->IsDebugging(BOTDEBUG_MOVEMENT))
		{
			NDebugOverlay::VertArrow(GetBot()->GetAbsOrigin(), goal, 4.0f, 0, 255, 0, 255, false, 0.2f);
			NDebugOverlay::Cross3D(goal, 12.0f, 0, 0, 255, false, 0.2f);
			m_ladderExit->DrawFilled(0, 255, 255, 200);
			NDebugOverlay::Text(m_ladderExit->GetCenter(), false, 0.1f, "LADDER EXIT AREA %3.2f", m_ladderGoalZ);
			NDebugOverlay::Text(origin, false, 0.1f, "BOT Z: %3.2f", origin.z);
			GetBot()->DebugPrintToConsole(51, 153, 153, "%s: Using Ladder Up. Distance to Z goal: %3.2f\n", GetBot()->GetDebugIdentifier(), m_ladderGoalZ - z);
		}
	}

	return USING_LADDER_UP;
}

IMovement::LadderState IMovement::UseLadderDown()
{
	if (m_ladder == nullptr || !IsOnLadder() || m_ladderTimer.IsElapsed())
	{
		return NOT_USING_LADDER;
	}

	auto input = GetBot()->GetControlInterface();
	auto origin = GetBot()->GetAbsOrigin();
	const float z = origin.z;

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

	if (GetBot()->IsDebugging(BOTDEBUG_MOVEMENT))
	{
		NDebugOverlay::VertArrow(GetBot()->GetAbsOrigin(), goal, 4.0f, 0, 255, 0, 255, false, 0.2f);
		NDebugOverlay::Cross3D(goal, 12.0f, 0, 0, 255, false, 0.2f);
		GetBot()->DebugPrintToConsole(51, 153, 153, "%s: Using Ladder Down. Distance to Z goal: %3.2f\n", GetBot()->GetDebugIdentifier(), fabsf(m_ladderGoalZ - z));
	}

	return USING_LADDER_DOWN;
}

IMovement::LadderState IMovement::DismountLadderTop()
{
	if (m_ladder == nullptr)
	{
		return NOT_USING_LADDER;
	}

	if (!IsOnLadder())
	{
		return NOT_USING_LADDER; // bot left ladder
	}

	auto input = GetBot()->GetControlInterface();
	auto origin = GetBot()->GetAbsOrigin();
	auto eyepos = GetBot()->GetEyeOrigin();
	const float z = origin.z;
	Vector toGoal = m_ladderExit->GetCenter() - origin;
	toGoal.z = 0.0f;
	float range = toGoal.NormalizeInPlace();
	toGoal.z = 1.0f;

	Vector goalPos = eyepos + 100.0f * toGoal;
	input->AimAt(goalPos, IPlayerController::LOOK_MOVEMENT, 0.2f, "Dismount Ladder");

	if (!input->IsAimOnTarget())
	{
		// wait until the bot is looking at the dismount position
		return EXITING_LADDER_UP;
	}

	MoveTowards(m_ladderExit->GetCenter(), MOVEWEIGHT_CRITICAL);

	if (m_ladder->GetLadderType() == CNavLadder::USEABLE_LADDER)
	{
		if (m_useLadderTimer.IsElapsed())
		{
			input->PressUseButton();
			m_useLadderTimer.Start(0.1f);
		}
	}

	if (GetBot()->IsDebugging(BOTDEBUG_MOVEMENT))
	{
		NDebugOverlay::VertArrow(GetBot()->GetAbsOrigin(), goalPos, 4.0f, 0, 255, 0, 255, false, 0.2f);
		NDebugOverlay::Cross3D(goalPos, 12.0f, 0, 0, 255, false, 0.2f);
	}

	constexpr auto tolerance = 10.0f;
	if (GetBot()->GetLastKnownNavArea() == m_ladderExit && range < tolerance)
	{
		m_ladder = nullptr;
		m_ladderExit = nullptr;
		return NOT_USING_LADDER;
	}

	return EXITING_LADDER_UP;
}

IMovement::LadderState IMovement::DismountLadderBottom()
{
	if (m_ladder == nullptr)
	{
		return NOT_USING_LADDER;
	}

	auto input = GetBot()->GetControlInterface();
	Vector toExit = (m_ladderExit->GetCenter() - GetBot()->GetAbsOrigin());
	toExit.z = 0.0f;
	toExit.NormalizeInPlace();
	Vector lookAt = GetBot()->GetEyeOrigin() + (toExit * 128.0f);

	input->AimAt(lookAt, IPlayerController::LOOK_MOVEMENT, 0.2f, "Looking at ladder dismount area!");
	MoveTowards(m_ladderExit->GetCenter(), MOVEWEIGHT_CRITICAL);

	if (!input->IsAimOnTarget())
	{
		// wait until the bot is looking at the dismount position
		return EXITING_LADDER_DOWN;
	}

	if (IsOnLadder())
	{
		if (m_ladder->GetLadderType() == CNavLadder::USEABLE_LADDER)
		{
			if (m_useLadderTimer.IsElapsed())
			{
				input->PressUseButton();
				m_useLadderTimer.Start(0.1f);
			}
		}

		// hopefully there is ground below us
		Jump();
	}
	else
	{
		return NOT_USING_LADDER;
	}

	return EXITING_LADDER_DOWN;
}

void IMovement::OnLadderStateChanged(LadderState oldState, LadderState newState)
{
	// time to climb is the length divided by this value
	constexpr auto LADDER_TIME_DIVIDER = 20.0f;

	switch (newState)
	{
	case IMovement::NOT_USING_LADDER:
		m_ladder = nullptr;
		m_ladderExit = nullptr;
		m_ladderTimer.Invalidate();
		m_ladderWait.Invalidate();
		m_ladderGoalZ = 0.0f;
		m_ladderMoveGoal = vec3_origin;
		return;
	case IMovement::EXITING_LADDER_UP:
	case IMovement::EXITING_LADDER_DOWN:
	{
		m_ladderTimer.Start(4.0f);
		m_ladderWait.Invalidate();
		SetDesiredSpeed(GetRunSpeed());
		return;
	}

	case IMovement::APPROACHING_LADDER_UP:
	{
		float z = GetBot()->GetAbsOrigin().z;
		z = m_ladder->ClampZ(z);

		m_ladderMoveGoal = m_ladder->m_bottom;
		m_ladderMoveGoal.z = z;
		GetBot()->GetControlInterface()->SnapAimAt(m_ladderMoveGoal, IPlayerController::LOOK_MOVEMENT);
		m_ladderTimer.Start(4.0f);
		SetDesiredSpeed(GetWalkSpeed());
		return;
	}
	case IMovement::USING_LADDER_UP:
	{
		// calculate Z goal
		m_ladderGoalZ = m_ladder->GetConnectionToArea(m_ladderExit)->GetConnectionPoint().z - (GetStepHeight() * 0.8f);
		float time = m_ladder->m_length / LADDER_TIME_DIVIDER;
		m_ladderTimer.Start(time);
		SetDesiredSpeed(GetRunSpeed());
		return;
	}
	case IMovement::APPROACHING_LADDER_DOWN:
	{
		m_ladderTimer.Start(4.0f);
		Vector lookAt = m_ladder->m_top;
		lookAt.z = m_ladder->ClampZ(GetBot()->GetAbsOrigin().z);
		GetBot()->GetControlInterface()->SnapAimAt(lookAt, IPlayerController::LOOK_MOVEMENT);
		SetDesiredSpeed(GetWalkSpeed());
		return;
	}
	case IMovement::USING_LADDER_DOWN:
	{
		// calculate Z goal
		m_ladderGoalZ = m_ladder->GetConnectionToArea(m_ladderExit)->GetConnectionPoint().z + (GetStepHeight() * 0.8f);
		Vector lookAt = m_ladder->m_top;
		lookAt.z = m_ladderGoalZ;
		GetBot()->GetControlInterface()->SnapAimAt(lookAt, IPlayerController::LOOK_MOVEMENT);
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
	else
	{
		MoveTowards(m_fromFloor->wait_position, MOVEWEIGHT_CRITICAL);
	}

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

		if (weapon.get() != nullptr)
		{
			minrange = weapon->GetWeaponInfo()->GetAttackInfo(WeaponInfo::AttackFunctionType::PRIMARY_ATTACK).GetMaxRange();
		}
	}

	if (callbutton == nullptr)
	{
		return ElevatorState::NOT_USING_ELEVATOR;
	}
	else
	{
		Vector pos = UtilHelpers::getWorldSpaceCenter(callbutton);
		MoveTowards(pos, MOVEWEIGHT_CRITICAL);
		bot->GetControlInterface()->AimAt(pos, IPlayerController::LOOK_MOVEMENT, 0.1f, "Looking at elevator call button!");

		if (bot->GetRangeTo(pos) < minrange)
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
	else
	{
		if (me->GetRangeTo(m_fromFloor->wait_position) > IMovement::ELEV_MOVE_RANGE)
		{
			MoveTowards(m_fromFloor->wait_position, MOVEWEIGHT_CRITICAL);
		}
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
	float minrange = (CBaseExtPlayer::PLAYER_USE_RADIUS * 0.7f);

	if (m_fromFloor->shootable_button)
	{
		me->GetInventoryInterface()->SelectBestHitscanWeapon();
		auto weapon = me->GetInventoryInterface()->GetActiveBotWeapon();

		if (weapon.get() != nullptr)
		{
			minrange = weapon->GetWeaponInfo()->GetAttackInfo(WeaponInfo::AttackFunctionType::PRIMARY_ATTACK).GetMaxRange();
		}
	}

	MoveTowards(pos, MOVEWEIGHT_CRITICAL);
	me->GetControlInterface()->AimAt(pos, IPlayerController::LOOK_MOVEMENT, 0.1f, "Looking at elevator use button!");

	if (me->GetRangeTo(pos) < minrange)
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


