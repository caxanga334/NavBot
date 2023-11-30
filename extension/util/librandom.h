#ifndef SMNAV_LIB_RANDOM_NUMBER_H_
#define SMNAV_LIB_RANDOM_NUMBER_H_
#pragma once

namespace random
{
	int generate_random_int(int min, int max);
	unsigned int generate_random_uint(unsigned int min, unsigned int max);
	// 50% chance for true
	bool generate_random_bool();
	float generate_random_float(float min, float max);
	double generate_random_double(double min, double max);
	bool random_chance(int chance = 50);
}

#endif // !SMNAV_LIB_RANDOM_NUMBER_H_

