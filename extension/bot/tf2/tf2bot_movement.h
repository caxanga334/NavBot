#ifndef NAVBOT_TF2_MOVEMENT_H_
#define NAVBOT_TF2_MOVEMENT_H_
#pragma once

#include <bot/interfaces/movement.h>

class CTF2Bot;

class CTF2BotMovement : public IMovement
{
public:
	CTF2BotMovement(CBaseBot* bot);
	~CTF2BotMovement() override;

	void Reset() override;
	void Update() override;

	// https://developer.valvesoftware.com/wiki/Team_Fortress_2/Mapper%27s_Reference#Jump_Distances
	float GetMaxJumpHeight() const override { return 72.0f; }
	float GetMaxDoubleJumpHeight() const override { return 116.0f; }
	float GetMaxGapJumpDistance() const override;
	void CrouchJump() override;
	void BlastJumpTo(const Vector& start, const Vector& landingGoal, const Vector& forward) override;
	bool IsAbleToDoubleJump() const override;
	// Can the bot perform a 'blast jump' (Example: TF2's rocket jump)
	bool IsAbleToBlastJump() const override;
	bool IsAbleToUseGrapplingHook() const override;
	bool IsAbleToUseOffMeshConnection(OffMeshConnectionType type, const NavOffMeshConnection* connection) const override;
protected:
	bool GapJumpRequiresDoubleJump(const Vector& landing, const Vector& forward) const override;
public:
	bool IsEntityTraversable(int index, edict_t* edict, CBaseEntity* entity, const bool now = true) override;
	bool IsControllingMovements() const override;
	bool IsPathingAllowed() const override;
	bool NeedsWeaponControl() const override;
	bool UseGrapplingHook(const Vector& start, const Vector& end) override;
	bool IsPathSegmentReached(const CMeshNavigator* nav, const CBasePathSegment* goal, bool& resultoverride) const override;

private:
	static constexpr float min_health_for_rocket_jumps() { return 130.0f; }
	// if the gap length on a jump over gap is greater than this, then a scout bot will perform a double jump
	static constexpr float scout_gap_jump_do_double_distance() { return 280.0f; }

	Vector m_blastJumpLandingGoal;
	Vector m_blastJumpStart;
	bool m_bIsBlastJumping;
	bool m_bBlastJumpAlreadyFired;
	bool m_bIsUsingGrapplingHook;
	CountdownTimer m_blastJumpFireTimer;
	CountdownTimer m_failTimer;
	Vector m_grapplingHookGoal;

	// returns true if the bot is aiming at
	bool DoRocketJumpAim();
	void BlastJumpUpdate();
	void OnEndBlastJump();
	void GrapplingHookUpdate();
};

#endif // !NAVBOT_TF2_MOVEMENT_H_
