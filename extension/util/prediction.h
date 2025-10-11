#ifndef NAVBOT_PREDICTION_LIB_H_
#define NAVBOT_PREDICTION_LIB_H_
#pragma once

class CBotWeapon;
class CBaseExtPlayer;
class CBaseBot;
class WeaponAttackFunctionInfo;

namespace pred
{
	Vector SimpleProjectileLead(const Vector& targetPosition, const Vector& targetVelocity, const float projectileSpeed, const float rangeBetween);
	float SimpleGravityCompensation(const float time, const float proj_gravity);

	inline const float GetProjectileTravelTime(const float projectileSpeed, const float rangeBetween)
	{
		return rangeBetween / projectileSpeed;
	}

	float GetGrenadeZ(const float rangeBetween, const float projectileSpeed);

	float GravityComp(const float rangeBetween, const float projGravity, const float elevationRate);

	Vector IterativeProjectileLead(const Vector& shooterPosition, const Vector& initialTargetPosition, const Vector& targetVelocity, const float projectileSpeed, const int maxIterations);

	Vector IterativeBallisticLeadAgaistPlayer(const Vector& shooterPosition, const Vector& initialTargetPosition, const Vector& targetVelocity, const WeaponAttackFunctionInfo* attackInfo, const int maxIterations);
}

#endif // !NAVBOT_PREDICTION_LIB_H_
