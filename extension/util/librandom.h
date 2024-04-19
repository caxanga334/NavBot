#ifndef SMNAV_LIB_RANDOM_NUMBER_H_
#define SMNAV_LIB_RANDOM_NUMBER_H_
#pragma once

#include <random>
#include <chrono>

namespace librandom
{
	int generate_random_int(int min, int max);
	unsigned int generate_random_uint(unsigned int min, unsigned int max);
	// 50% chance for true
	bool generate_random_bool();
	float generate_random_float(float min, float max);
	double generate_random_double(double min, double max);
	bool random_chance(int chance = 50);

	template <typename E, typename S = unsigned int>
	class RandomNumberGenerator
	{
	public:
		RandomNumberGenerator() {}

		void ReSeed();
		void RandomReSeed();

		template <typename T>
		T GetRandomInt(T min, T max);

		template <typename T>
		T GetRandomReal(T min, T max);

	private:
		E engine;
	};


	template<typename E, typename S>
	inline void RandomNumberGenerator<E,S>::ReSeed()
	{
		S seed = std::chrono::system_clock::now().time_since_epoch().count();
		engine.seed(seed);
	}

	template<typename E, typename S>
	inline void RandomNumberGenerator<E, S>::RandomReSeed()
	{
		S seed = engine();
		engine.seed(seed);
	}

	template<typename E, typename S>
	template<typename T>
	inline T RandomNumberGenerator<E, S>::GetRandomInt(T min, T max)
	{
		std::uniform_int_distribution<T> dist(min, max);
		return dist(engine);
	}

	template<typename E, typename S>
	template<typename T>
	inline T RandomNumberGenerator<E, S>::GetRandomReal(T min, T max)
	{
		std::uniform_real_distribution<T> dist(min, max);
		return dist(engine);
	}
}

// default generator using Mersenne Twister 19937
extern librandom::RandomNumberGenerator<std::mt19937, unsigned int>* randomgen;

#endif // !SMNAV_LIB_RANDOM_NUMBER_H_