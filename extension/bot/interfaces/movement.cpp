#include <extension.h>
#include <bot/interfaces/playercontrol.h>
#include <bot/basebot.h>
#include <navmesh/nav_mesh.h>
#include <navmesh/nav_area.h>
#include <navmesh/nav_ladder.h>
#include <manager.h>
#include <mods/basemod.h>
#include <util/UtilTrace.h>
#include <util/EntityUtils.h>
#include <util/entprops.h>
#include "movement.h"

extern CExtManager* extmanager;

constexpr auto DEFAULT_PLAYER_STANDING_HEIGHT = 72.0f;
constexpr auto DEFAULT_PLAYER_DUCKING_HEIGHT = 36.0f;
constexpr auto DEFAULT_PLAYER_HULL_WIDTH = 32.0f;

class CMovementTraverseFilter : public CTraceFilterSimple
{
public:
	CMovementTraverseFilter(CBaseBot* bot, IMovement* mover, const bool now = true);

	virtual bool ShouldHitEntity(IHandleEntity* pHandleEntity, int contentsMask) override;

private:
	CBaseBot* m_me;
	IMovement* m_mover;
	bool m_now;
};

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

IMovement::IMovement(CBaseBot* bot) : IBotInterface(bot)
{
	m_ladder = nullptr;
	m_internal_jumptimer = -1;
}

IMovement::~IMovement()
{
}

void IMovement::Reset()
{
	m_internal_jumptimer = -1;
	m_jumptimer.Invalidate();
	m_ladder = nullptr;
	m_state = STATE_NONE;
}

void IMovement::Update()
{
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

void IMovement::ClimbLadder(CNavLadder* ladder)
{
}

void IMovement::Jump()
{
	// Execute a crouch jump
	// See shounic's video https://www.youtube.com/watch?v=7z_p_RqLhkA

	// First the bot will crouch
	GetBot()->GetControlInterface()->PressCrouchButton(0.1f); // hold the crouch button for about 7 ticks

	// This is a tick timer, the bot will jump when it reaches 0
	m_internal_jumptimer = 5; // jump after 5 ticks
	m_jumptimer.Start(0.8f); // Timer for 'Is the bot performing a jump'
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

