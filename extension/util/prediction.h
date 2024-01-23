#ifndef NAVBOT_PREDICTION_LIB_H_
#define NAVBOT_PREDICTION_LIB_H_
#pragma once

namespace pred
{
	Vector SimpleProjectileLead(const Vector& targetPosition, const Vector& targetVelocity, const float projectileSpeed, const float rangeBetween);
	float SimpleGravityCompensation(const float time, const float proj_gravity);

	inline const float GetProjectileTravelTime(const float projectileSpeed, const float rangeBetween)
	{
		return rangeBetween / projectileSpeed;
	}

	float GetGrenadeZ(const float rangeBetween, const float projectileSpeed);
}

#endif // !NAVBOT_PREDICTION_LIB_H_
