#ifndef NAVBOT_PREDICTION_LIB_H_
#define NAVBOT_PREDICTION_LIB_H_
#pragma once

namespace pred
{
	Vector SimpleProjectileLead(const Vector& targetPosition, const Vector& targetVelocity, const float projectileSpeed, const float rangeBetween);
}

#endif // !NAVBOT_PREDICTION_LIB_H_
