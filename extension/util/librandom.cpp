#include NAVBOT_PCH_FILE
#include <bot/basebot.h>
#include "librandom.h"

static librandom::RandomNumberGenerator<std::mt19937, unsigned int> s_randomgen;
librandom::RandomNumberGenerator<std::mt19937, unsigned int>* randomgen = &s_randomgen;

int librandom::generate_random_int(int min, int max)
{
	RandomNumberGenerator<std::mt19937, unsigned int> rng;
	rng.RandomReSeed();

	return rng.GetRandomInt<int>(min, max);
}

void librandom::ReSeedGlobalGenerators()
{
	s_randomgen.RandomReSeed();
	CBaseBot::s_botrng.RandomReSeed();
	CBaseBot::s_usercmdrng.RandomReSeed();
}
