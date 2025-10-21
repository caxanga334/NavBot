#include NAVBOT_PCH_FILE
#include <algorithm>

#include <extension.h>
#include <manager.h>
#include <util/helpers.h>
#include <util/entprops.h>
#include <entities/baseentity.h>
#include <bot/basebot.h>
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

Vector pred::IterativeProjectileLead(const Vector& shooterPosition, const Vector& initialTargetPosition, const Vector& targetVelocity, const float projectileSpeed, const int maxIterations)
{
	Vector targetPos = initialTargetPosition;
	Vector targetDir = targetPos - shooterPosition;
	float range = targetDir.NormalizeInPlace();
	float time = GetProjectileTravelTime(projectileSpeed, range);

	for (int i = 0; i < maxIterations; i++)
	{
		Vector predictedPosition = initialTargetPosition + (targetVelocity * time);

		targetDir = predictedPosition - shooterPosition;
		range = targetDir.NormalizeInPlace();
		time = GetProjectileTravelTime(projectileSpeed, range);

		targetPos = predictedPosition;
	}

	return targetPos;
}

Vector pred::IterativeBallisticLeadAgaistPlayer(const Vector& shooterPosition, const Vector& initialTargetPosition, const Vector& targetVelocity, const WeaponAttackFunctionInfo* attackInfo, const int maxIterations)
{
	const float projectileSpeed = attackInfo->GetProjectileSpeed();
	Vector targetPos = initialTargetPosition;
	Vector targetDir = targetPos - shooterPosition;
	float range = targetDir.NormalizeInPlace();
	float time = GetProjectileTravelTime(projectileSpeed, range);

	for (int i = 0; i < maxIterations; i++)
	{
		Vector predictedPosition = initialTargetPosition + (targetVelocity * time);

		targetDir = predictedPosition - shooterPosition;
		range = targetDir.NormalizeInPlace();
		time = GetProjectileTravelTime(projectileSpeed, range);

		const float elevation_rate = RemapValClamped(range, attackInfo->GetBallisticElevationStartRange(), 
			attackInfo->GetBallisticElevationEndRange(), 
			attackInfo->GetBallisticElevationMinRate(), attackInfo->GetBallisticElevationMaxRate());

		const float z = GravityComp(range, attackInfo->GetGravity(), elevation_rate);

		predictedPosition.z += z;

		targetPos = predictedPosition;
	}

	return targetPos;
}

void pred::ProjectileData_t::FillFromAttackInfo(const WeaponAttackFunctionInfo* attackInfo)
{
	this->speed = attackInfo->GetProjectileSpeed();
	this->gravity = attackInfo->GetGravity();
	this->ballistic_start_range = attackInfo->GetBallisticElevationStartRange();
	this->ballistic_end_range = attackInfo->GetBallisticElevationEndRange();
	this->ballistic_min_rate = attackInfo->GetBallisticElevationMinRate();
	this->ballistic_max_rate = attackInfo->GetBallisticElevationMaxRate();
}

Vector pred::IterativeBallisticLead(const Vector& shooterPosition, const Vector& initialTargetPosition, const Vector& targetVelocity, const ProjectileData_t& data, const int maxIterations)
{
	const float projectileSpeed = data.speed;
	Vector targetPos = initialTargetPosition;
	Vector targetDir = targetPos - shooterPosition;
	float range = targetDir.NormalizeInPlace();
	float time = GetProjectileTravelTime(projectileSpeed, range);

	for (int i = 0; i < maxIterations; i++)
	{
		Vector predictedPosition = initialTargetPosition + (targetVelocity * time);

		targetDir = predictedPosition - shooterPosition;
		range = targetDir.NormalizeInPlace();
		time = GetProjectileTravelTime(projectileSpeed, range);

		const float elevation_rate = RemapValClamped(range, data.ballistic_start_range, data.ballistic_end_range,
			data.ballistic_min_rate, data.ballistic_max_rate);

		const float z = GravityComp(range, data.gravity, elevation_rate);

		predictedPosition.z += z;

		targetPos = predictedPosition;
	}

	return targetPos;
}
