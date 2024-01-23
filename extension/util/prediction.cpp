#include <algorithm>

#include <extension.h>
#include <extplayer.h>
#include <ifaces_extern.h>
#include <util/helpers.h>
#include <util/entprops.h>
#include <util/UtilTrace.h>
#include <mathlib/vector.h>
#include "prediction.h"

#undef min
#undef max
#undef clamp

Vector pred::SimpleProjectileLead(const Vector& targetPosition, const Vector& targetVelocity, const float projectileSpeed, const float rangeBetween)
{
	const float time = GetProjectileTravelTime(projectileSpeed, rangeBetween);
	Vector result = targetPosition + (targetVelocity * time);
	return result;
}

float pred::SimpleGravityCompensation(const float time, const float proj_gravity)
{
	ConVarRef sv_gravity("sv_gravity");

	return (sv_gravity.GetFloat() / 2.0f) * (time * time) * proj_gravity;
}

float pred::GetGrenadeZ(const float rangeBetween, const float projectileSpeed)
{
	ConVarRef sv_gravity("sv_gravity");
	constexpr auto PROJECTILE_SPEED_CONST = 0.707f;

	const float time = GetProjectileTravelTime(projectileSpeed * PROJECTILE_SPEED_CONST, rangeBetween);
	return std::min(0.0f, ((powf(2.0f, time) - 1.0f) * (sv_gravity.GetFloat() * 0.1f)));
}
