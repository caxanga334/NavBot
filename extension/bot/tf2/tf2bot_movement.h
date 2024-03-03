#ifndef NAVBOT_TF2_MOVEMENT_H_
#define NAVBOT_TF2_MOVEMENT_H_
#pragma once

#include <bot/interfaces/movement.h>

class CTF2Bot;

class CTF2BotMovement : public IMovement
{
public:
	CTF2BotMovement(CBaseBot* bot);
	virtual ~CTF2BotMovement();

	static constexpr float PLAYER_HULL_WIDTH = 48.0f;
	static constexpr float PLAYER_HULL_STAND = 82.0f;
	static constexpr float PLAYER_HULL_CROUCH = 62.0f;

	virtual void Reset() override;
	virtual void Frame() override;

	virtual float GetHullWidth() override;
	virtual float GetStandingHullHeigh() override;
	virtual float GetCrouchedHullHeigh() override;
	// https://developer.valvesoftware.com/wiki/Team_Fortress_2/Mapper%27s_Reference#Jump_Distances
	virtual float GetMaxDoubleJumpHeight() const override { return 110.0f; } // add some safety margin
	virtual float GetMaxGapJumpDistance() const override;

	virtual void DoubleJump() override;

	virtual bool IsAbleToDoubleJump() override;
	// Can the bot perform a 'blast jump' (Example: TF2's rocket jump)
	virtual bool IsAbleToBlastJump() override;

	virtual void JumpAcrossGap(const Vector& landing, const Vector& forward) override;

private:
	CTF2Bot* GetTF2Bot() const;
	static constexpr float min_health_for_rocket_jumps() { return 100.0f; }
	// if the gap length on a jump over gap is greater than this, then a scout bot will perform a double jump
	static constexpr float scout_gap_jump_do_double_distance() { return 280.0f; }

	int m_doublejumptimer;
	int m_djboosttimer;
};

#endif // !NAVBOT_TF2_MOVEMENT_H_
