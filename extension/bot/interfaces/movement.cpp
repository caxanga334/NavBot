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
			return !m_mover->IsEntityTraversable(pEdict, m_now);
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
	Reset();
}

IMovement::~IMovement()
{
}

void IMovement::Reset()
{
	m_internal_jumptimer = -1;
	m_jumptimer.Invalidate();
	m_ladder = nullptr;
	m_ladderExit = nullptr;
	m_ladderState = NOT_USING_LADDER;
	m_landingGoal = vec3_origin;
	m_isClimbingObstacle = false;
	m_isJumpingAcrossGap = false;
	m_isAirborne = false;
	m_stuck.Reset();
	m_motionVector = vec3_origin;
	m_groundMotionVector = vec2_origin;
	m_speed = 0.0f;
	m_groundspeed = 0.0f;
}

void IMovement::Update()
{
	m_basemovespeed = GetBot()->GetMaxSpeed();

	StuckMonitor();

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

	if (TraverseLadder())
	{
		return;
	}

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

		MoveTowards(m_landingGoal);
	}
}

void IMovement::Frame()
{
	// update crouch-jump timer
	if (m_internal_jumptimer >= 0)
	{
		m_internal_jumptimer--;

		if (m_internal_jumptimer == 0)
		{
			// GetBot()->GetControlInterface()->PressJumpButton(0.23f);
			Vector vel = GetBot()->GetAbsVelocity();
			vel.z = GetCrouchJumpZBoost();
			GetBot()->SetAbsVelocity(vel);

			if (GetBot()->IsDebugging(BOTDEBUG_MOVEMENT))
			{
				GetBot()->DebugPrintToConsole(BOTDEBUG_MOVEMENT, 200, 255, 200, "%s Crouch Jump! (JUMP) \n", GetBot()->GetDebugIdentifier());
			}
		}
	}

	if (m_braketimer.HasStarted() && !m_braketimer.IsElapsed())
	{
		// Cheat: force velocity to zero. The correct way would be to setup counter-strafing
		Vector vel(0.0f, 0.0f, 0.0f);
		GetBot()->SetAbsVelocity(vel);
	}
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

void IMovement::MoveTowards(const Vector& pos)
{
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
	input->AimAt(lookAt, priority, 0.1f);
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
	m_ladderState = LadderState::APPROACHING_LADDER_UP;
	m_ladder = ladder;
	m_ladderExit = dismount;

	auto bot = GetBot();
	
	if (bot->IsDebugging(BOTDEBUG_MOVEMENT))
	{
		bot->DebugPrintToConsole(BOTDEBUG_MOVEMENT, 200, 0, 200, "%s CLIMBING LADDER! \n", bot->GetDebugIdentifier());

		m_ladderExit->DrawFilled(255, 0, 255, 255, 5.0f, true);
		NDebugOverlay::Text(m_ladderExit->GetCenter() + Vector(0.0f, 0.0f, StepHeight), "DISMOUNT AREA!", false, 5.0f);
	}
}

void IMovement::DescendLadder(const CNavLadder* ladder, CNavArea* dismount)
{
	m_ladderState = LadderState::APPROACHING_LADDER_DOWN;
	m_ladder = ladder;
	m_ladderExit = dismount;

	auto bot = GetBot();

	if (bot->IsDebugging(BOTDEBUG_MOVEMENT))
	{
		bot->DebugPrintToConsole(BOTDEBUG_MOVEMENT, 200, 0, 200, "%s DESCENDING LADDER! \n", bot->GetDebugIdentifier());

		m_ladderExit->DrawFilled(255, 0, 255, 255, 5.0f, true);
		NDebugOverlay::Text(m_ladderExit->GetCenter() + Vector(0.0f, 0.0f, StepHeight), "DISMOUNT AREA!", false, 5.0f);
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

	// This is a tick timer, the bot will jump when it reaches 0
	m_internal_jumptimer = 8; // jump after some time
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
	Jump();

	// look towards the jump target
	GetBot()->GetControlInterface()->AimAt(landing, IPlayerController::LOOK_MOVEMENT, 1.0f);

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
	case IMovement::USING_LADDER_UP:
	case IMovement::EXITING_LADDER_DOWN:
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
 * @return true if the bot can walk, false otherwise
*/
bool IMovement::IsPotentiallyTraversable(const Vector& from, const Vector& to, float* fraction, const bool now, CBaseEntity** obstacle)
{
	float heightdiff = to.z - from.z;

	if (heightdiff >= GetMaxJumpHeight())
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

bool IMovement::IsEntityTraversable(edict_t* entity, const bool now)
{
	int index = gamehelpers->IndexOfEdict(entity);

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
		if (entprops->GetEntProp(index, Prop_Data, "m_eDoorState", doorstate) == false)
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

void IMovement::StuckMonitor()
{
	auto bot = GetBot();
	auto origin = bot->GetAbsOrigin();
	constexpr auto STUCK_RADIUS = 100.0f; // distance the bot has to move to consider not stuck

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
			ClearStuckStatus();
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

bool IMovement::TraverseLadder()
{
	switch (m_ladderState)
	{
	case IMovement::APPROACHING_LADDER_UP:
		m_ladderState = ApproachUpLadder();
		return true;
	case IMovement::EXITING_LADDER_UP:
		m_ladderState = DismountLadderTop();
		return true;
	case IMovement::USING_LADDER_UP:
		m_ladderState = UseLadderUp();
		return true;
	case IMovement::APPROACHING_LADDER_DOWN:
		m_ladderState = ApproachDownLadder();
		return true;
	case IMovement::EXITING_LADDER_DOWN:
		m_ladderState = DismountLadderBottom();
		return true;
	case IMovement::USING_LADDER_DOWN:
		m_ladderState = UseLadderDown();
		return true;
	default:
		m_ladder = nullptr;
		m_ladderExit = nullptr;
		m_ladderTimer.Invalidate();

		return false;
	}
}

IMovement::LadderState IMovement::ApproachUpLadder()
{
	if (m_ladder == nullptr)
	{
		return NOT_USING_LADDER;
	}

	auto origin = GetBot()->GetAbsOrigin();
	const float z = origin.z;
	const bool debug = GetBot()->IsDebugging(BOTDEBUG_MOVEMENT);

	// sanity check in case the bot movement fails somehow and resets while being above the ladder already
	if (z >= m_ladder->m_top.z - GetStepHeight())
	{
		m_ladderTimer.Start(2.0f);
		return EXITING_LADDER_UP;
	}

	if (z <= m_ladder->m_bottom.z - GetMaxJumpHeight())
	{
		return NOT_USING_LADDER; // can't reach ladder bottom
	}

	FaceTowards(m_ladder->m_bottom, true);
	MoveTowards(m_ladder->m_bottom);

	if (m_ladder->GetLadderType() == CNavLadder::USEABLE_LADDER)
	{

		// useable ladder within use range, press use button.
		if (GetBot()->GetAbsOrigin().AsVector2D().DistTo(m_ladder->m_bottom.AsVector2D()) < CBaseExtPlayer::PLAYER_USE_RADIUS)
		{
			if (!GetBot()->GetControlInterface()->IsPressingTheUseButton())
			{
				GetBot()->GetControlInterface()->PressUseButton(0.1f);
			}
		}
	}

	if (debug)
	{
		NDebugOverlay::VertArrow(GetBot()->GetAbsOrigin(), m_ladder->m_bottom, 4.0f, 0, 255, 0, 255, false, 0.2f);
		NDebugOverlay::Cross3D(m_ladder->m_bottom, 12.0f, 0, 0, 255, false, 0.2f);
	}

	if (IsOnLadder()) // bot grabbed the ladder
	{
		if (debug)
		{
			Vector mins = GetBot()->GetMins();
			Vector maxs = GetBot()->GetMaxs();

			NDebugOverlay::Box(GetBot()->GetAbsOrigin(), mins, maxs, 0, 0, 255, 255, 5.0f);
			GetBot()->DebugPrintToConsole(BOTDEBUG_MOVEMENT, 0, 200, 200, "%s GRABBED LADDER (UP DIR)! \n", GetBot()->GetDebugIdentifier());
		}

		GetBot()->GetControlInterface()->ReleaseMovementButtons();
		return USING_LADDER_UP; // climb it
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

	Vector climbPoint = m_ladder->m_top + 0.25f * GetHullWidth() * m_ladder->GetNormal();
	Vector to = climbPoint - origin;
	to.z = 0.0f;
	float mountRange = to.NormalizeInPlace();
	Vector moveGoal;

	constexpr auto VERY_CLOSE_RANGE = 10.0f;
	constexpr auto MOVE_DIST = 100.0f;
	if (mountRange < VERY_CLOSE_RANGE)
	{
		// bot is very close to the ladder, just move forward
		auto& forward = GetMotionVector();
		moveGoal = origin + MOVE_DIST * forward;
	}
	else
	{
		if (DotProduct(to, m_ladder->GetNormal()) < 0.0f)
		{
			moveGoal = m_ladder->m_top - MOVE_DIST * m_ladder->GetNormal();
		}
		else
		{
			moveGoal = m_ladder->m_top + MOVE_DIST * m_ladder->GetNormal();
		}
	}

	FaceTowards(moveGoal, true);
	MoveTowards(moveGoal);

	if (m_ladder->GetLadderType() == CNavLadder::USEABLE_LADDER)
	{

		// useable ladder within use range, press use button.
		if (GetBot()->GetAbsOrigin().AsVector2D().DistTo(m_ladder->m_top.AsVector2D()) < CBaseExtPlayer::PLAYER_USE_RADIUS)
		{
			if (!GetBot()->GetControlInterface()->IsPressingTheUseButton())
			{
				GetBot()->GetControlInterface()->PressUseButton(0.1f);
			}
		}
	}

	if (debug)
	{
		NDebugOverlay::VertArrow(GetBot()->GetAbsOrigin(), moveGoal, 4.0f, 0, 255, 0, 255, false, 0.2f);
		NDebugOverlay::Cross3D(moveGoal, 12.0f, 0, 0, 255, false, 0.2f);
	}

	if (IsOnLadder())
	{
		if (debug)
		{
			Vector mins = GetBot()->GetMins();
			Vector maxs = GetBot()->GetMaxs();

			NDebugOverlay::Box(GetBot()->GetAbsOrigin(), mins, maxs, 0, 0, 255, 255, 5.0f);
			GetBot()->DebugPrintToConsole(BOTDEBUG_MOVEMENT, 0, 200, 200, "%s GRABBED LADDER (DOWN DIR)! \n", GetBot()->GetDebugIdentifier());
		}

		GetBot()->GetControlInterface()->ReleaseMovementButtons();
		return USING_LADDER_DOWN; // bot grabbed the ladder
	}

	return APPROACHING_LADDER_DOWN;
}

IMovement::LadderState IMovement::UseLadderUp()
{
	if (m_ladder == nullptr)
	{
		return NOT_USING_LADDER;
	}

	if (IsOnLadder() == false)
	{
		return NOT_USING_LADDER; // bot fell
	}

	auto input = GetBot()->GetControlInterface();
	auto origin = GetBot()->GetAbsOrigin();
	const float z = origin.z;
	const float landingz = m_ladderExit->GetZ(m_ladder->m_top) + (GetStepHeight() * 0.25f);

	// Bot is not at the top of the ladder but is above the landing nav area height plus some offset distance
	if (z >= landingz)
	{
		m_ladderTimer.Start(2.0f);
		return EXITING_LADDER_UP;
	}

	if (z >= m_ladder->m_top.z)
	{
		// reached ladder top
		m_ladderTimer.Start(2.0f);
		return EXITING_LADDER_UP;
	}

	Vector goal = origin + 100.0f * (-m_ladder->GetNormal() + Vector(0.0f, 0.0f, 2.0f));
	input->AimAt(goal, IPlayerController::LOOK_MOVEMENT, 0.1f, "Going up a ladder.");
	MoveTowards(goal);

	if (GetBot()->IsDebugging(BOTDEBUG_MOVEMENT))
	{
		NDebugOverlay::VertArrow(GetBot()->GetAbsOrigin(), goal, 4.0f, 0, 255, 0, 255, false, 0.2f);
		NDebugOverlay::Cross3D(goal, 12.0f, 0, 0, 255, false, 0.2f);
		m_ladderExit->DrawFilled(0, 255, 255, 200);
	}

	return USING_LADDER_UP;
}

IMovement::LadderState IMovement::UseLadderDown()
{
	if (m_ladder == nullptr)
	{
		return NOT_USING_LADDER;
	}

	if (IsOnLadder() == false)
	{
		return NOT_USING_LADDER; // bot fell
	}

	auto input = GetBot()->GetControlInterface();
	auto origin = GetBot()->GetAbsOrigin();
	const float z = origin.z;

	if (z <= m_ladder->m_bottom.z + GetStepHeight())
	{
		m_ladderTimer.Start(2.0f);
		return EXITING_LADDER_DOWN;
	}

	Vector goal = origin + 100.0f * (m_ladder->GetNormal() + Vector(0.0f, 0.0f, 2.0f));
	input->AimAt(goal, IPlayerController::LOOK_MOVEMENT, 0.1f);
	MoveTowards(goal);

	if (GetBot()->IsDebugging(BOTDEBUG_MOVEMENT))
	{
		NDebugOverlay::VertArrow(GetBot()->GetAbsOrigin(), goal, 4.0f, 0, 255, 0, 255, false, 0.2f);
		NDebugOverlay::Cross3D(goal, 12.0f, 0, 0, 255, false, 0.2f);
	}

	return USING_LADDER_DOWN;
}

IMovement::LadderState IMovement::DismountLadderTop()
{
	if (m_ladder == nullptr)
	{
		return NOT_USING_LADDER;
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
	input->AimAt(goalPos, IPlayerController::LOOK_MOVEMENT, 0.1f);

	MoveTowards(goalPos);

	if (m_ladder->GetLadderType() == CNavLadder::USEABLE_LADDER)
	{
		if (!input->IsPressingTheUseButton())
		{
			input->PressUseButton(0.1f);
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

	if (IsOnLadder())
	{
		if (m_ladder->GetLadderType() == CNavLadder::USEABLE_LADDER)
		{
			if (!input->IsPressingTheUseButton())
			{
				input->PressUseButton(0.1f);
			}
		}


		// hopefully there is ground below us
		Jump();
	}

	return NOT_USING_LADDER;
}


