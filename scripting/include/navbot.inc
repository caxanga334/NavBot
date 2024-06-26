/**
 * Navbot's SourcePawn include file
 */
 
#if defined _navbot_included
 #endinput
#endif
#define _navbot_included

/**
 * Checks if the given client is a NavBot managed fake client.
 *
 * @param client        Client index.
 * @error               Invalid client index
 */
native bool IsNavBot(int client);

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