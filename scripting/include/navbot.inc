/**
 * Navbot's SourcePawn include file
 */
 
#if defined _navbot_included
 #endinput
#endif
#define _navbot_included

#include "navbot/bots.inc"

/**
 * Checks if the given client is a NavBot managed fake client.
 *
 * @param client        Client index.
 * @return					True if the given client is a NavBot, false otherwise.
 * @error               Invalid client index
 */
native bool IsNavBot(int client);

/**
 * Adds a NavBot.
 *
 * @param client			The index of the bot just added.
 * @param botName			Optional bot name. NULL_STRING to use a random name.
 * @return					True if the bot was added, false otherwise.
 * @error					AddBot unavailable.
 */
native bool AddNavBot(int& client, const char[] botName = NULL_STRING);

/**
 * Checks if NavBot's Navigation Mesh for the current map is loaded.
 *
 * @return					True if loaded, false otherwise.
 */
native bool IsNavMeshLoaded();

/**
 * Do not edit below this line!
 */
public Extension __ext_navbot =
{
	name = "navbot",
	file = "navbot.ext",
#if defined AUTOLOAD_EXTENSIONS
	autoload = 1,
#else
	autoload = 0,
#endif
#if defined REQUIRE_EXTENSIONS
	required = 1,
#else
	required = 0,
#endif
};