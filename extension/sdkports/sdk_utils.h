#ifndef SDK_PORT_UTILS_H_
#define SDK_PORT_UTILS_H_
#pragma once

#include <extension.h>
#include <mathlib/mathlib.h>

inline float UTIL_VecToYaw(const Vector& vec)
{
	if (vec.y == 0 && vec.x == 0)
		return 0;

	float yaw = atan2(vec.y, vec.x);

	yaw = RAD2DEG(yaw);

	if (yaw < 0)
		yaw += 360;

	return yaw;
}


inline float UTIL_VecToPitch(const Vector& vec)
{
	if (vec.y == 0 && vec.x == 0)
	{
		if (vec.z < 0)
			return 180.0;
		else
			return -180.0;
	}

	float dist = vec.Length2D();
	float pitch = atan2(-vec.z, dist);

	pitch = RAD2DEG(pitch);

	return pitch;
}

inline Vector UTIL_YawToVector(float yaw)
{
	Vector ret;

	ret.z = 0;
	float angle = DEG2RAD(yaw);
	SinCos(angle, &ret.y, &ret.x);

	return ret;
}

inline edict_t* UTIL_GetListenServerEnt()
{
	return gamehelpers->EdictOfIndex(1);
}

inline edict_t* UTIL_GetListenServerHost()
{
	return gamehelpers->EdictOfIndex(1);
}

#endif