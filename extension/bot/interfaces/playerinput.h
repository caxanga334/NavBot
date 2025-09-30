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

	/**
	 * @brief Which attack type was last used by the bot?
	 */
	enum class AttackType : int
	{
		ATTACK_NONE,
		ATTACK_PRIMARY,
		ATTACK_SECONDARY,

		MAX_ATTACK_TYPES
	};

	void ReleaseAllButtons();
	void ReleaseMovementButtons(const bool uncrouch = false);
	void ReleaseAllAttackButtons();
	void ResetInputData();
	void PressAttackButton(const float duration = -1.0f);
	void ReleaseAttackButton();
	bool IsPressingAttackButton();
	void PressSecondaryAttackButton(const float duration = -1.0f);
	void ReleaseSecondaryAttackButton();
	bool IsPressingSecondaryAttackButton();
	void PressSpecialAttackButton(const float duration = -1.0f);
	void ReleaseSpecialAttackButton();
	void PressJumpButton(const float duration = -1.0f);
	void ReleaseJumpButton();
	bool IsPressingJumpButton() const;
	void PressCrouchButton(const float duration = -1.0f);
	void ReleaseCrouchButton();
	bool IsPressingCrouchButton() const;
	void PressForwardButton(const float duration = -1.0f);
	void ReleaseForwardButton();
	void PressBackwardsButton(const float duration = -1.0f);
	void ReleaseBackwardsButton();
	void PressUseButton(const float duration = -1.0f);
	void ReleaseUseButton();
	bool IsPressingTheUseButton();
	void PressMoveLeftButton(const float duration = -1.0f);
	void ReleaseMoveLeftButton();
	void PressMoveRightButton(const float duration = -1.0f);
	void ReleaseMoveRightButton();
	void PressMoveUpButton(const float duration = -1.0f);
	void ReleaseMoveUpButton();
	bool IsPressingMoveUpButton();
	void PressMoveDownButton(const float duration = -1.0f);
	void ReleaseMoveDownButton();
	bool IsPressingMoveDownButton();
	void PressReloadButton(const float duration = -1.0f);
	void ReleaseReloadButton();
	void PressAlt1Button(const float duration = -1.0f);
	void ReleaseAlt1Button();
	void SetMovementScale(const float forward, const float side, const float duration = 0.01f);
	
	inline const float GetForwardScale() const { return m_forwardscale; }
	inline const float GetSideScale() const { return m_sidescale; }
	inline bool ShouldApplyScale() const { return !m_buttonscaletimer.IsElapsed(); }

	// Buttons that will be sent between update
	inline int GetOldButtonsToSend() const
	{
		return m_oldbuttons | m_buttons;
	}

	inline void OnPostRunCommand()
	{
		m_buttons = 0;
	}

	virtual void ProcessButtons(int& buttons) = 0;

	inline AttackType GetLastUsedAttackType() const { return m_lastUsedAttackType; }

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
	CountdownTimer m_moveupbuttontimer;
	CountdownTimer m_movedownbuttontimer;
	CountdownTimer m_reloadbuttontimer;
	CountdownTimer m_alt1buttontimer;
	CountdownTimer m_buttonscaletimer;
	AttackType m_lastUsedAttackType;

	// Updates m_buttons with a list of button currently held down
	void CompileButtons();
};

inline IPlayerInput::IPlayerInput()
{
	m_buttons = 0;
	m_oldbuttons = 0;
	m_forwardscale = 1.0f;
	m_sidescale = 1.0f;
	m_lastUsedAttackType = AttackType::ATTACK_NONE;
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
	m_moveupbuttontimer.Invalidate();
	m_movedownbuttontimer.Invalidate();
	m_reloadbuttontimer.Invalidate();
	m_alt1buttontimer.Invalidate();
}

inline void IPlayerInput::ReleaseMovementButtons(const bool uncrouch)
{
	ReleaseForwardButton();
	ReleaseBackwardsButton();
	ReleaseMoveLeftButton();
	ReleaseMoveRightButton();
	ReleaseMoveUpButton();
	ReleaseMoveDownButton();
	ReleaseJumpButton();

	if (uncrouch)
	{
		ReleaseCrouchButton();
	}
}

inline void IPlayerInput::ReleaseAllAttackButtons()
{
	ReleaseAttackButton();
	ReleaseSecondaryAttackButton();
	ReleaseSpecialAttackButton();
	m_lastUsedAttackType = AttackType::ATTACK_NONE;
}

inline void IPlayerInput::ResetInputData()
{
	m_oldbuttons = 0;
	m_forwardscale = 1.0f;
	m_sidescale = 1.0f;
	m_buttonscaletimer.Invalidate();
	m_lastUsedAttackType = AttackType::ATTACK_NONE;
}

inline void IPlayerInput::PressAttackButton(const float duration)
{
	m_buttons |= INPUT_ATTACK;
	m_leftmousebuttontimer.Start(duration);
	m_lastUsedAttackType = AttackType::ATTACK_PRIMARY;
}

inline void IPlayerInput::ReleaseAttackButton()
{
	m_buttons &= ~INPUT_ATTACK;
	m_leftmousebuttontimer.Invalidate();

	if (m_lastUsedAttackType == AttackType::ATTACK_PRIMARY)
	{
		m_lastUsedAttackType = AttackType::ATTACK_NONE;
	}
}

inline bool IPlayerInput::IsPressingAttackButton()
{
	return m_leftmousebuttontimer.HasStarted() && !m_leftmousebuttontimer.IsElapsed();
}

inline void IPlayerInput::PressSecondaryAttackButton(const float duration)
{
	m_buttons |= INPUT_ATTACK2;
	m_rightmousebuttontimer.Start(duration);
	m_lastUsedAttackType = AttackType::ATTACK_SECONDARY;
}

inline void IPlayerInput::ReleaseSecondaryAttackButton()
{
	m_buttons &= ~INPUT_ATTACK2;
	m_rightmousebuttontimer.Invalidate();

	if (m_lastUsedAttackType == AttackType::ATTACK_SECONDARY)
	{
		m_lastUsedAttackType = AttackType::ATTACK_NONE;
	}
}

inline bool IPlayerInput::IsPressingSecondaryAttackButton()
{
	return m_rightmousebuttontimer.HasStarted() && !m_rightmousebuttontimer.IsElapsed();
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

inline bool IPlayerInput::IsPressingJumpButton() const
{
	return m_jumpbuttontimer.HasStarted() && !m_jumpbuttontimer.IsElapsed();
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

inline bool IPlayerInput::IsPressingCrouchButton() const
{
	return m_crouchbuttontimer.HasStarted() && !m_crouchbuttontimer.IsElapsed();
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

inline bool IPlayerInput::IsPressingTheUseButton()
{
	return m_usebuttontimer.HasStarted() && !m_usebuttontimer.IsElapsed();
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

inline void IPlayerInput::PressMoveUpButton(const float duration)
{
	m_moveupbuttontimer.Start(duration);
}

inline void IPlayerInput::ReleaseMoveUpButton()
{
	m_moveupbuttontimer.Invalidate();
}

inline bool IPlayerInput::IsPressingMoveUpButton()
{
	return m_moveupbuttontimer.HasStarted() && !m_moveupbuttontimer.IsElapsed();
}

inline void IPlayerInput::PressMoveDownButton(const float duration)
{
	m_movedownbuttontimer.Start(duration);
}

inline void IPlayerInput::ReleaseMoveDownButton()
{
	m_movedownbuttontimer.Invalidate();
}

inline bool IPlayerInput::IsPressingMoveDownButton()
{
	return m_movedownbuttontimer.HasStarted() && !m_movedownbuttontimer.IsElapsed();
}

inline void IPlayerInput::PressReloadButton(const float duration)
{
	m_buttons |= INPUT_RELOAD;
	m_reloadbuttontimer.Start(duration);
}

inline void IPlayerInput::ReleaseReloadButton()
{
	m_buttons &= ~INPUT_RELOAD;
	m_reloadbuttontimer.Invalidate();
}

inline void IPlayerInput::PressAlt1Button(const float duration)
{
	m_buttons |= INPUT_ALT1;
	m_alt1buttontimer.Start(duration);
}

inline void IPlayerInput::ReleaseAlt1Button()
{
	m_buttons &= ~INPUT_ALT1;
	m_alt1buttontimer.Invalidate();
}

inline void IPlayerInput::SetMovementScale(const float forward, const float side, const float duration)
{
	m_forwardscale = forward;
	m_sidescale = side;
	m_buttonscaletimer.Start(duration);
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

