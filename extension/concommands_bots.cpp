#include NAVBOT_PCH_FILE
#include <vector>
#include <string>

#include <extension.h>
#include <manager.h>
#include <extplayer.h>
#include <bot/basebot.h>

CON_COMMAND(sm_navbot_add, "Adds a Nav Bot to the game.")
{
	if (!extmanager->AreBotsSupported())
	{
		Msg("Bot support is disabled! Check SourceMod logs for more information. \n");
		return;
	}

	if (!TheNavMesh->IsLoaded())
	{
		Warning("Nav Mesh not loaded, bots will not be able to move! \n");
	}

	if (args.ArgC() < 2)
	{
		extmanager->AddBot(nullptr, nullptr);
	}
	else
	{
		std::string botname(args[1]);
		extmanager->AddBot(&botname, nullptr);
	}	
}

CON_COMMAND(sm_navbot_kick, "Removes a Nav Bot from the game.")
{
	if (args.ArgC() < 2)
	{
		extmanager->RemoveRandomBot("Nav Bot: Kicked by admin command.");
		return;
	}

	if (strncasecmp(args[1], "all", 3) == 0)
	{
		extmanager->RemoveAllBots("Nav Bot: Kicked by admin command.");
		return;
	}

	std::string targetname(args[1]);
	auto functor = [&targetname](CBaseBot* bot) {
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
	};
	extmanager->ForEachBot(functor);
}
