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

enum NavBotAISoundType
{
	NAVBOT_SOUND_INVALID = -1,		// Invalid sound (don't use it)
	NAVBOT_SOUND_GENERIC = 0, 		// generic sounds
	NAVBOT_SOUND_WEAPON, 			// weapon sound (except firing)
	NAVBOT_SOUND_GUNFIRE, 			// gun fire
	NAVBOT_SOUND_PLAYER, 			// player sounds
	NAVBOT_SOUND_FOOTSTEP, 			// NPC/player footstep
	NAVBOT_SOUND_VOICE, 			// NPC/player voice
	NAVBOT_SOUND_WORLD, 			// world emitted sounds
	NAVBOT_SOUND_DOOR, 				// door opening/closing
	NAVBOT_SOUND_TURRET, 			// turret/sentry gun sound
	NAVBOT_SOUND_DANGER, 			// something dangerous

	NAVBOT_MAX_SOUND_TYPES
};

/**
 * Fires an AI sound event for all NavBot's bots.
 *
 * @param entity			The entity that made the sound.
 * @param origin			The sound position uses the entity's WorldSpaceCenter by default, you can pass a position here to override it.
 * @param soundType			Sound type made by the entity.
 * @param maxRadius			Maximum distance from origin the sound can be heard. Negative values means it's global.
 * @error					Invalid entity, invalid sound type or volume value given.
 */
native void FireNavBotSoundEvent(int entity, const float origin[3] = NULL_VECTOR, NavBotAISoundType soundType, const float maxRadius);

/**
 * Called when a new NavBot instance is about to be created.
 * 
 * @return Plugin_Continue to allow the bot to be added. Anything else to prevent the bot from being added.
 */
forward Action OnPreNavBotAdd();

/**
 * Called when a NavBot was just added to the game.
 * 
 * @param bot Client index of the bot that was just added.
 */
forward void OnNavBotAdded(int bot);

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