#ifndef UTIL_RANDOM_NUMBER_GENERATOR_H
#define UTIL_RANDOM_NUMBER_GENERATOR_H

#include <vstdlib/random.h>

extern CUniformRandomStream g_NavRandom;

inline int UTIL_GetRandomInt(const int min, const int max)
{
	return g_NavRandom.RandomInt(min, max);
}

inline float UTIL_GetRandomFloat(const float min, const float max)
{
	return g_NavRandom.RandomFloat(min, max);
}

#endif // !UTIL_RANDOM_NUMBER_GENERATOR_H

