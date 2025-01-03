#ifndef SMNAV_LIB_RANDOM_NUMBER_H_
#define SMNAV_LIB_RANDOM_NUMBER_H_
#pragma once

#include <random>
#include <chrono>
#include <vector>

namespace librandom
{
	// for some static variables from the nav mesh, for everything else use the classes below
	int generate_random_int(int min, int max);

	template <typename E, typename S = unsigned int>
	class RandomNumberGenerator
	{
	public:
		RandomNumberGenerator() {}

		void ReSeed();
		void ReSeed(S seed);
		void RandomReSeed();

		template <typename T>
		T GetRandomInt(T min, T max);

		template <typename T>
		T GetRandomReal(T min, T max);

	private:
		E engine;
	};

	template <typename E = std::random_device>
	class RandomNumberGeneratorNoSeed
	{
	public:
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
	inline void RandomNumberGenerator<E, S>::ReSeed(S seed)
	{
		engine.seed(seed);
	}

	template<typename E, typename S>
	inline void RandomNumberGenerator<E, S>::RandomReSeed()
	{
		std::random_device seedgenerator;
		S seed = static_cast<S>(seedgenerator());
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

	template<typename E>
	template<typename T>
	inline T RandomNumberGeneratorNoSeed<E>::GetRandomInt(T min, T max)
	{
		std::uniform_int_distribution<T> dist(min, max);
		return dist(engine);
	}

	template<typename E>
	template<typename T>
	inline T RandomNumberGeneratorNoSeed<E>::GetRandomReal(T min, T max)
	{
		std::uniform_real_distribution<T> dist(min, max);
		return dist(engine);
	}
}

// default generator using Mersenne Twister 19937
extern librandom::RandomNumberGenerator<std::mt19937, unsigned int>* randomgen;

namespace librandom::utils
{
	template <typename T>
	inline T GetRandomElementFromVector(const std::vector<T>& vec)
	{
		return vec[randomgen->GetRandomInt<size_t>(0U, vec.size() - 1U)];
	}
}

#endif // !SMNAV_LIB_RANDOM_NUMBER_H_