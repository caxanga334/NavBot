#ifndef UTIL_RANDOM_NUMBER_GENERATOR_H
#define UTIL_RANDOM_NUMBER_GENERATOR_H

#include <random>

inline int UTIL_GetRandomInt(const int min, const int max)
{
	std::random_device generator;
	std::uniform_int_distribution<int> dist(min, max);
	return dist(generator);
}

inline float UTIL_GetRandomFloat(const float min, const float max)
{
	std::random_device generator;
	std::uniform_real_distribution<float> dist(min, max);
	return dist(generator);
}

#endif // !UTIL_RANDOM_NUMBER_GENERATOR_H

