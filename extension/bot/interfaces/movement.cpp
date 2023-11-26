#include <bot/interfaces/playercontrol.h>
#include <bot/basebot.h>
#include <navmesh/nav_mesh.h>
#include <navmesh/nav_area.h>
#include <navmesh/nav_ladder.h>
#include <manager.h>
#include <mods/basemod.h>
#include "movement.h"

extern CExtManager* extmanager;

constexpr auto DEFAULT_PLAYER_STANDING_HEIGHT = 72.0f;
constexpr auto DEFAULT_PLAYER_DUCKING_HEIGHT = 36.0f;
constexpr auto DEFAULT_PLAYER_HULL_WIDTH = 32.0f;

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

