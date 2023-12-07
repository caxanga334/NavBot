#include <vector>

#include <extension.h>
#include <manager.h>
#include <extplayer.h>
#include <bot/basebot.h>

extern CExtManager* extmanager;

// #ifdef SMNAV_FEAT_BOT is not needed in this file, it's not include to the source list by default

CON_COMMAND(smnav_bot_add, "Adds a extenion bot to the game.")
{
	extmanager->AddBot();
}

#ifdef SMNAV_DEBUG

CON_COMMAND(smnav_debug_bot_look, "Debug the bot look functions.")
{
	auto& bots = extmanager->GetAllBots();
	auto edict = gamehelpers->EdictOfIndex(1); // get listen server host

	for (auto& botptr : bots)
	{
		auto bot = botptr.get();
		bot->GetControlInterface()->AimAt(edict, IPlayerController::LOOK_CRITICAL, 10.0f);
	}
}

#endif // SMNAV_DEBUG
