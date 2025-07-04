#include NAVBOT_PCH_FILE
#include <algorithm>

#include <extension.h>
#include <manager.h>
#include <bot/basebot.h>
#include <util/helpers.h>
#include <util/entprops.h>
#include <entities/baseentity.h>
#include "prediction.h"

#undef min
#undef max
#undef clamp

Vector pred::SimpleProjectileLead(const Vector& targetPosition, const Vector& targetVelocity, const float projectileSpeed, const float rangeBetween)
{
	float time = GetProjectileTravelTime(projectileSpeed, rangeBetween);

	// time = time + (gpGlobals->interval_per_tick * 2.0f); // add two ticks ahead

	Vector result = targetPosition + (targetVelocity * time);
	return result;
}

float pred::SimpleGravityCompensation(const float time, const float proj_gravity)
{
	return (CExtManager::GetSvGravityValue() / 2.0f) * (time * time) * proj_gravity;
}

float pred::GetGrenadeZ(const float rangeBetween, const float projectileSpeed)
{
	constexpr auto PROJECTILE_SPEED_CONST = 0.707f;

	const float time = GetProjectileTravelTime(projectileSpeed * PROJECTILE_SPEED_CONST, rangeBetween);
	return std::min(0.0f, ((powf(2.0f, time) - 1.0f) * (CExtManager::GetSvGravityValue() * 0.1f)));
}

float pred::GravityComp(const float rangeBetween, const float projGravity, const float elevationRate)
{
	constexpr auto STEP_DIVIDER = 50.0f;
	float steps = rangeBetween / STEP_DIVIDER;
	float z = (CExtManager::GetSvGravityValue() * projGravity) * (elevationRate * steps);

	return z;
}
