#include <extension.h>
#include <bot/interfaces/playercontrol.h>
#include <bot/basebot.h>
#include <navmesh/nav_mesh.h>
#include <navmesh/nav_area.h>
#include <navmesh/nav_ladder.h>
#include <manager.h>
#include <mods/basemod.h>
#include <util/EntityUtils.h>
#include <util/entprops.h>
#include "movement.h"

extern CExtManager* extmanager;

constexpr auto DEFAULT_PLAYER_STANDING_HEIGHT = 72.0f;
constexpr auto DEFAULT_PLAYER_DUCKING_HEIGHT = 36.0f;
constexpr auto DEFAULT_PLAYER_HULL_WIDTH = 32.0f;

CMovementTraverseFilter::CMovementTraverseFilter(CBaseBot* bot, IMovement* mover, const bool now) :
	CTraceFilterSimple(bot->GetEdict()->GetIServerEntity(), COLLISION_GROUP_PLAYER_MOVEMENT)
{
	m_me = bot;
	m_mover = mover;
	m_now = now;
}

bool CMovementTraverseFilter::ShouldHitEntity(IHandleEntity* pHandleEntity, int contentsMask)
{
	edict_t* entity = entityFromEntityHandle(pHandleEntity);

	if (entity == m_me->GetEdict())
	{
		return false; // don't hit the bot
	}

	if (CTraceFilterSimple::ShouldHitEntity(pHandleEntity, contentsMask))
	{
		return !m_mover->IsEntityTraversable(entity, m_now);
	}

	return false;
}

CTraceFilterOnlyActors::CTraceFilterOnlyActors(const IHandleEntity* handleentity) :
	CTraceFilterSimple(handleentity, COLLISION_GROUP_NONE, nullptr)
{
}

bool CTraceFilterOnlyActors::ShouldHitEntity(IHandleEntity* pHandleEntity, int contentsMask)
{
	if (CTraceFilterSimple::ShouldHitEntity(pHandleEntity, contentsMask))
	{
		edict_t* entity = entityFromEntityHandle(pHandleEntity);
		int index = gamehelpers->IndexOfEdict(entity);

		if (UtilHelpers::IsPlayerIndex(index))
		{
			return true; // hit players
		}
	}

	return false;
}

IMovement::IMovement(CBaseBot* bot) : IBotInterface(bot)
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
}

void IMovement::Update()
{
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
			GetBot()->GetControlInterface()->PressJumpButton(); 
		}
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
	Vector eyeforward;
	me->EyeVectors(&eyeforward);

	Vector2D forward(eyeforward.x, eyeforward.y);
	forward.NormalizeInPlace();
	Vector2D right(forward.y, -forward.x);

	// Build direction vector towards goal
	Vector2D to = (pos - me->GetAbsOrigin()).AsVector2D();
	float distancetogoal = to.NormalizeInPlace();

	float ahead = to.Dot(forward);
	float side = to.Dot(right);

	constexpr float epsilon = 0.25f;

	if (ahead > epsilon)
	{
		input->PressForwardButton();
	}
	else if (ahead < -epsilon)
	{
		input->PressBackwardsButton();
	}

	if (side > epsilon)
	{
		input->PressMoveLeftButton();
	}
	else if (side < -epsilon)
	{
		input->PressMoveRightButton();
	}

}

// makes the bot stop moving
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
}

void IMovement::DescendLadder(const CNavLadder* ladder, CNavArea* dismount)
{
	m_ladderState = LadderState::APPROACHING_LADDER_DOWN;
	m_ladder = ladder;
	m_ladderExit = dismount;
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
	GetBot()->GetControlInterface()->PressCrouchButton(0.1f); // hold the crouch button for about 7 ticks

	// This is a tick timer, the bot will jump when it reaches 0
	m_internal_jumptimer = 5; // jump after 5 ticks
	m_jumptimer.Start(0.8f); // Timer for 'Is the bot performing a jump'
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
}

bool IMovement::ClimbUpToLedge(const Vector& landingGoal, const Vector& landingForward, edict_t* obstacle)
{
	Jump();

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

	if (FClassnameIs(entity, "func_door*") == true || FClassnameIs(entity, "prop_door*") == true)
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
	CTraceFilterNoNPCsOrPlayer filter(GetBot()->GetEdict()->GetIServerEntity(), COLLISION_GROUP_PLAYER_MOVEMENT);
	Vector start = pos + Vector(0.0f, 0.0f, GetStepHeight());
	Vector end = pos + Vector(0.0f, 0.0f, -GetMaxJumpHeight());
	UTIL_TraceLine(start, end, GetMovementTraceMask(), &filter, &result);

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
bool IMovement::IsPotentiallyTraversable(const Vector& from, const Vector& to, float& fraction, const bool now)
{
	float heightdiff = to.z - from.z;

	if (heightdiff >= GetMaxJumpHeight())
	{
		Vector along = to - from;
		along.NormalizeInPlace();
		if (along.z > GetTraversableSlopeLimit()) // too high, can't climb this ramp
		{
			fraction = 0.0f;
			return false;
		}
	}

	trace_t result;
	CMovementTraverseFilter filter(GetBot(), this, now);
	const float hullsize = GetHullWidth() * 0.25f; // use a smaller hull since we won't resolve collisions now
	const float height = GetStepHeight();
	Vector mins(-hullsize, -hullsize, height);
	Vector maxs(hullsize, hullsize, GetCrouchedHullHeigh());
	UTIL_TraceHull(from, to, mins, maxs, GetMovementTraceMask(), nullptr, COLLISION_GROUP_PLAYER_MOVEMENT, &result);

	fraction = result.fraction;

	return result.fraction >= 1.0f && !result.startsolid;
}

bool IMovement::HasPotentialGap(const Vector& from, const Vector& to, float& fraction)
{
	// get movement fraction
	float traversableFraction = 0.0f;
	IsPotentiallyTraversable(from, to, traversableFraction, true);

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

	if (FClassnameIs(entity, "func_door*") == true)
	{
		return true;
	}

	if (FClassnameIs(entity, "prop_door*") == true)
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

	if (FClassnameIs(entity, "func_brush") == true)
	{
		int solidity = 0;
		if (entprops->GetEntProp(index, Prop_Data, "m_iSolidity", solidity) == true)
		{
			switch (solidity)
			{
			case BRUSHSOLID_NEVER:
			case BRUSHSOLID_TOGGLE:
			{
				return true;
			}
			case BRUSHSOLID_ALWAYS:
			default:
			{
				return false;
			}
			}
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

void IMovement::ClearStuckStatus()
{
	m_stuck.ClearStuck(GetBot()->GetAbsOrigin());
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
		}
		else
		{
			if (m_stuck.stuckrechecktimer.IsElapsed())
			{
				m_stuck.stuckrechecktimer.Start(1.0f);

				// resend stuck event
				bot->OnStuck();
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
			float minspeed = 0.1f * GetMovementSpeed() * 0.1f;
			float mintimeforstuck = STUCK_RADIUS / minspeed;

			if (m_stuck.stucktimer.IsGreaterThen(mintimeforstuck))
			{
				// bot is taking too long to move, consider it stuck
				m_stuck.Stuck(bot);
				bot->OnStuck();
			}
		}
	}


}


