#include <vector>
#include <string>

#include <extension.h>
#include <manager.h>
#include <extplayer.h>
#include <bot/basebot.h>

CON_COMMAND(sm_navbot_add, "Adds a Nav Bot to the game.")
{
	extmanager->AddBot();
}

CON_COMMAND(sm_navbot_kick, "Removes a Nav Bot from the game.")
{
	if (args.ArgC() < 2)
	{
		extmanager->RemoveRandomBot("Nav Bot: Kicked by admin command.");
		return;
	}

	std::string targetname(args[1]);
	extmanager->ForEachBot([&targetname](CBaseBot* bot) {
		auto gp = playerhelpers->GetGamePlayer(bot->GetIndex());

		if (gp)
		{
			auto szname = gp->GetName();

			if (szname)
			{
				std::string botname(szname);
				
				if (botname.find(targetname) != std::string::npos) // partial name search
				{
					gp->Kick("Nav Bot: Kicked by admin command.");
				}
			}
		}
	});
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
