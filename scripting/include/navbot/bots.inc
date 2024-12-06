#if defined _navbot_bots_included
 #endinput
#endif
#define _navbot_bots_included

// A NavBot bot instance
methodmap NavBot
{
	/**
	 * Creates a new NavBot instance or returns an existing one.
	 * @param client Client index to attach the NavBot instance to.
	 * @error Invalid client index.
	 */
	public native NavBot(int client);

	public native bool IsPluginBot();

	property int Index
	{
		public get() { return view_as<int>(this); }
	}

	property bool IsNull
	{
		public get() { return view_as<int>(this) == 0; }
	}
}

/**
 * Gets a NavBot instance by client index.
 * @param client  Client index
 * @return  NavBot instance or NULL_NAVBOT is a navbot doesn't exists for the given client index.
 */
native NavBot GetNavBotByIndex(int client);

const NavBot NULL_NAVBOT = view_as<NavBot>(0);

#include "interfaces/path.inc"