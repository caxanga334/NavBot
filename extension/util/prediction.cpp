#include <extension.h>
#include <extplayer.h>
#include <ifaces_extern.h>
#include <util/helpers.h>
#include <util/entprops.h>
#include <util/UtilTrace.h>
#include <mathlib/vector.h>
#include "prediction.h"

Vector pred::SimpleProjectileLead(const Vector& targetPosition, const Vector& targetVelocity, const float projectileSpeed, const float rangeBetween)
{
	const float time = rangeBetween / projectileSpeed;
	Vector result = targetPosition + (targetVelocity * time);
	return result;
}
