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

		/**
		 * @brief Generates a random integer.
		 * @tparam T Integer type.
		 * @param min Minimum value.
		 * @param max Maximum value.
		 * @return Randomly generated value.
		 */
		template <typename T>
		T GetRandomInt(T min, T max);

		/**
		 * @brief Generates a random real.
		 * @tparam T Real type (float, double)
		 * @param min Minimum value.
		 * @param max Maximum value.
		 * @return Randomly generated value.
		 */
		template <typename T>
		T GetRandomReal(T min, T max);

		/**
		 * @brief Generates a random bool value.
		 * @param chance Chance for the result to be true. Defaults to 50%.
		 * @return Randomly generated boolean value.
		 */
		bool GetRandomChance(const int chance = 50)
		{
			return chance >= this->GetRandomInt<int>(1, 100);
		}

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

namespace librandom
{
	// Sets a new randomly generated seed to the global random number generators
	void ReSeedGlobalGenerators();
}

namespace librandom::utils
{
	template <typename T>
	inline T GetRandomElementFromVector(const std::vector<T>& vec)
	{
		return vec[randomgen->GetRandomInt<size_t>(0U, vec.size() - 1U)];
	}
}

#endif // !SMNAV_LIB_RANDOM_NUMBER_H_