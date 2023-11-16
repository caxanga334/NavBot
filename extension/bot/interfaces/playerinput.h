#ifndef SMNAV_BOT_PLAYER_INPUT_H_
#define SMNAV_BOT_PLAYER_INPUT_H_
#pragma once

class CBotCmd;

// AM's HL2-SDK doesn't have all buttons.
// See: https://github.com/alliedmodders/sourcemod/blob/master/plugins/include/entity_prop_stocks.inc#L99

constexpr auto INPUT_ATTACK = (1 << 0);
constexpr auto INPUT_JUMP = (1 << 1); // Jump
constexpr auto INPUT_DUCK = (1 << 2); // Crouch
constexpr auto INPUT_FORWARD = (1 << 3); // Walk forwards
constexpr auto INPUT_BACK = (1 << 4); // Walk backwards
constexpr auto INPUT_USE = (1 << 5); // +use, press buttons, elevators, pick up items ...
constexpr auto INPUT_CANCEL = (1 << 6);
constexpr auto INPUT_LEFT = (1 << 7); // turn the camera to the left
constexpr auto INPUT_RIGHT = (1 << 8); // turn the camere to the right
constexpr auto INPUT_MOVELEFT = (1 << 9); // Strafe left
constexpr auto INPUT_MOVERIGHT = (1 << 10); //  Strafe right
constexpr auto INPUT_ATTACK2 = (1 << 11);
constexpr auto INPUT_RUN = (1 << 12);
constexpr auto INPUT_RELOAD = (1 << 13);
constexpr auto INPUT_ALT1 = (1 << 14);
constexpr auto INPUT_ALT2 = (1 << 15);
constexpr auto INPUT_SCORE = (1 << 16);
constexpr auto INPUT_SPEED = (1 << 17); // Mod dependant, some means sprinting, others means walking silently
constexpr auto INPUT_WALK = (1 << 18);
constexpr auto INPUT_ZOOM = (1 << 19);
constexpr auto INPUT_WEAPON1 = (1 << 20); // Meaning defined by weapon
constexpr auto INPUT_WEAPON2 = (1 << 21); // Meaning defined by weapon
constexpr auto INPUT_BULLRUSH = (1 << 22);
constexpr auto INPUT_GRENADE1 = (1 << 23);
constexpr auto INPUT_GRENADE2 = (1 << 24);
constexpr auto INPUT_ATTACK3 = (1 << 25); // Special attack (TF2)


#include <sdkports/sdk_timers.h>

// Handles client buttons sent to the server
class IPlayerInput
{
public:
	IPlayerInput();

	void ReleaseAllButtons();
	void PressAttackButton(const float duration = -1.0f);
	void ReleaseAttackButton();
	void PressSecondaryAttackButton(const float duration = -1.0f);
	void ReleaseSecondaryAttackButton();
	void PressSpecialAttackButton(const float duration = -1.0f);
	void ReleaseSpecialAttackButton();
	void PressJumpButton(const float duration = -1.0f);
	void ReleaseJumpButton();
	void PressCrouchButton(const float duration = -1.0f);
	void ReleaseCrouchButton();
	void PressForwardButton(const float duration = -1.0f);
	void ReleaseForwardButton();
	void PressBackwardsButton(const float duration = -1.0f);
	void ReleaseBackwardsButton();
	void PressUseButton(const float duration = -1.0f);
	void ReleaseUseButton();
	void PressMoveLeftButton(const float duration = -1.0f);
	void ReleaseMoveLeftButton();
	void PressMoveRightButton(const float duration = -1.0f);
	void ReleaseMoveRightButton();
	void PressReloadButton(const float duration = -1.0f);
	void ReleaseReloadButton();
	void SetMovementScale(const float forward, const float side);

	virtual void ProcessButtons(CBotCmd* cmd) = 0;

protected:
	int m_buttons; // Buttons to be sent in the next user command
	int m_oldbuttons; // Buttons that were pressed in the last user command sent
	float m_forwardscale; // Scale for forward movement
	float m_sidescale; // Scale for side movement
	CountdownTimer m_leftmousebuttontimer; // +attack
	CountdownTimer m_rightmousebuttontimer; // +attack2
	CountdownTimer m_specialmousebuttontimer; // +attack3 (used by TF2)
	CountdownTimer m_jumpbuttontimer;
	CountdownTimer m_crouchbuttontimer;
	CountdownTimer m_forwardbuttontimer;
	CountdownTimer m_backwardsbuttontimer;
	CountdownTimer m_usebuttontimer;
	CountdownTimer m_moveleftbuttontimer;
	CountdownTimer m_moverightbuttontimer;
	CountdownTimer m_reloadbuttontimer;

	// Updates m_buttons with a list of button currently held down
	void CompileButtons();
};

inline IPlayerInput::IPlayerInput()
{
	m_buttons = 0;
	m_oldbuttons = 0;
	m_forwardscale = 1.0f;
	m_sidescale = 1.0f;
}

inline void IPlayerInput::ReleaseAllButtons()
{
	m_buttons = 0;
	m_leftmousebuttontimer.Invalidate();
	m_rightmousebuttontimer.Invalidate();
	m_specialmousebuttontimer.Invalidate();
	m_jumpbuttontimer.Invalidate();
	m_crouchbuttontimer.Invalidate();
	m_forwardbuttontimer.Invalidate();
	m_backwardsbuttontimer.Invalidate();
	m_usebuttontimer.Invalidate();
	m_moveleftbuttontimer.Invalidate();
	m_moverightbuttontimer.Invalidate();
	m_reloadbuttontimer.Invalidate();
}

inline void IPlayerInput::PressAttackButton(const float duration)
{
	m_buttons |= INPUT_ATTACK;
	m_leftmousebuttontimer.Start(duration);
}

inline void IPlayerInput::ReleaseAttackButton()
{
	m_buttons &= ~INPUT_ATTACK;
	m_leftmousebuttontimer.Invalidate();
}

inline void IPlayerInput::PressSecondaryAttackButton(const float duration)
{
	m_buttons |= INPUT_ATTACK2;
	m_rightmousebuttontimer.Start(duration);
}

inline void IPlayerInput::ReleaseSecondaryAttackButton()
{
	m_buttons &= ~INPUT_ATTACK2;
	m_rightmousebuttontimer.Invalidate();
}

inline void IPlayerInput::PressSpecialAttackButton(const float duration)
{
	m_buttons |= INPUT_ATTACK3;
	m_specialmousebuttontimer.Start(duration);
}

inline void IPlayerInput::ReleaseSpecialAttackButton()
{
	m_buttons &= ~INPUT_ATTACK3;
	m_specialmousebuttontimer.Invalidate();
}

inline void IPlayerInput::PressJumpButton(const float duration)
{
	m_buttons |= INPUT_JUMP;
	m_jumpbuttontimer.Start(duration);
}

inline void IPlayerInput::ReleaseJumpButton()
{
	m_buttons &= ~INPUT_JUMP;
	m_jumpbuttontimer.Invalidate();
}

inline void IPlayerInput::PressCrouchButton(const float duration)
{
	m_buttons |= INPUT_DUCK;
	m_crouchbuttontimer.Start(duration);
}

inline void IPlayerInput::ReleaseCrouchButton()
{
	m_buttons &= ~INPUT_DUCK;
	m_crouchbuttontimer.Invalidate();
}

inline void IPlayerInput::PressForwardButton(const float duration)
{
	m_buttons |= INPUT_FORWARD;
	m_forwardbuttontimer.Start(duration);
}

inline void IPlayerInput::ReleaseForwardButton()
{
	m_buttons &= ~INPUT_FORWARD;
	m_forwardbuttontimer.Invalidate();
}

inline void IPlayerInput::PressBackwardsButton(const float duration)
{
	m_buttons |= INPUT_BACK;
	m_backwardsbuttontimer.Invalidate();
}

inline void IPlayerInput::ReleaseBackwardsButton()
{
	m_buttons &= ~INPUT_BACK;
	m_backwardsbuttontimer.Invalidate();
}

inline void IPlayerInput::PressUseButton(const float duration)
{
	m_buttons |= INPUT_USE;
	m_usebuttontimer.Start(duration);
}

inline void IPlayerInput::ReleaseUseButton()
{
	m_buttons &= ~INPUT_USE;
	m_usebuttontimer.Invalidate();
}

inline void IPlayerInput::PressMoveLeftButton(const float duration)
{
	m_buttons |= INPUT_MOVELEFT;
	m_moveleftbuttontimer.Start(duration);
}

inline void IPlayerInput::ReleaseMoveLeftButton()
{
	m_buttons &= ~INPUT_MOVELEFT;
	m_moveleftbuttontimer.Invalidate();
}

inline void IPlayerInput::PressMoveRightButton(const float duration)
{
	m_buttons |= INPUT_MOVERIGHT;
	m_moverightbuttontimer.Start(duration);
}

inline void IPlayerInput::ReleaseMoveRightButton()
{
	m_buttons &= ~INPUT_MOVERIGHT;
	m_moverightbuttontimer.Invalidate();
}

inline void IPlayerInput::PressReloadButton(const float duration)
{
	m_buttons |= INPUT_RELOAD;
	m_reloadbuttontimer.Invalidate();
}

inline void IPlayerInput::ReleaseReloadButton()
{
	m_buttons &= ~INPUT_RELOAD;
	m_reloadbuttontimer.Invalidate();
}

inline void IPlayerInput::SetMovementScale(const float forward, const float side)
{
	m_forwardscale = forward;
	m_sidescale = side;
}

inline void IPlayerInput::CompileButtons()
{
	if (!m_leftmousebuttontimer.IsElapsed())
		m_buttons |= INPUT_ATTACK;

	if (!m_rightmousebuttontimer.IsElapsed())
		m_buttons |= INPUT_ATTACK2;

	if (!m_specialmousebuttontimer.IsElapsed())
		m_buttons |= INPUT_ATTACK3;

	if (!m_jumpbuttontimer.IsElapsed())
		m_buttons |= INPUT_JUMP;

	if (!m_crouchbuttontimer.IsElapsed())
		m_buttons |= INPUT_DUCK;

	if (!m_forwardbuttontimer.IsElapsed())
		m_buttons |= INPUT_FORWARD;

	if (!m_backwardsbuttontimer.IsElapsed())
		m_buttons |= INPUT_BACK;

	if (!m_usebuttontimer.IsElapsed())
		m_buttons |= INPUT_USE;

	if (!m_moveleftbuttontimer.IsElapsed())
		m_buttons |= INPUT_MOVELEFT;

	if (!m_moverightbuttontimer.IsElapsed())
		m_buttons |= INPUT_MOVERIGHT;

	if (!m_reloadbuttontimer.IsElapsed())
		m_buttons |= INPUT_RELOAD;
}

#endif // !SMNAV_BOT_PLAYER_INPUT_H_

