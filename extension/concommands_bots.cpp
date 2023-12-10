#include <vector>

#include <extension.h>
#include <manager.h>
#include <extplayer.h>
#include <bot/basebot.h>

extern CExtManager* extmanager;

CON_COMMAND(sm_navbot_add, "Adds a Nav Bot to the game.")
{
	extmanager->AddBot();
}

#ifdef EXT_DEBUG

CON_COMMAND(sm_navbot_debug_bot_look, "Debug the bot look functions.")
{
	auto& bots = extmanager->GetAllBots();
	auto edict = gamehelpers->EdictOfIndex(1); // get listen server host

	for (auto& botptr : bots)
	{
		auto bot = botptr.get();
		bot->GetControlInterface()->AimAt(edict, IPlayerController::LOOK_CRITICAL, 10.0f);
	}
}

#endif // EXT_DEBUG
