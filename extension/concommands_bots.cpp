#include NAVBOT_PCH_FILE
#include <vector>
#include <string>

#include <extension.h>
#include <manager.h>
#include <extplayer.h>
#include <bot/basebot.h>

CON_COMMAND(sm_navbot_add, "Adds a Nav Bot to the game.")
{
	DECLARE_COMMAND_ARGS;

	if (!extmanager->AreBotsSupported())
	{
		Msg("Bot support is disabled! Check SourceMod logs for more information. \n");
		return;
	}

	if (!TheNavMesh->IsLoaded())
	{
		Warning("Nav Mesh not loaded, bots will not be able to move! \n");
	}

	bool printHelp = !!args.FindArg("-help");

	if (printHelp)
	{

		META_CONPRINT("[SM] Usage: sm_navbot_add <args ...>\n");
		META_CONPRINT("Arguments: \n-count : number of bots to add\n-name : the bot's name (only works if adding 1 bot)\n");
		META_CONPRINT("-help : prints this message.\n");
		return;
	}

	int count = args.FindArgInt("-count", 1);

	count = std::max(count, 1);

	if (count == 1)
	{
		bool bHasName = !!args.FindArg("-name");

		if (bHasName)
		{
			std::string name{ args.FindArg("-name") };

			extmanager->AddBot(&name, nullptr);
		}
		else
		{
			extmanager->AddBot(nullptr, nullptr);
		}
	}
	else
	{
		while (count > 0)
		{
			extmanager->AddBot(nullptr, nullptr);
			count--;
		}
	}
}

CON_COMMAND(sm_navbot_kick, "Removes a Nav Bot from the game.")
{
	DECLARE_COMMAND_ARGS;

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
