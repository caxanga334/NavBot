#if defined _navbot_bots_included
 #endinput
#endif
#define _navbot_bots_included

#include "interfaces/base_interface.inc"
#include "interfaces/inventory.inc"
#include "interfaces/movement.inc"
#include "interfaces/sensor.inc"
#include "interfaces/playercontroller.inc"
#include "interfaces/sharedbotmemory.inc"

// Pass this to SetSkillLevel to generate a random difficulty profile for the bot.
#define NAVBOT_SKILL_RANDOM -1

enum NavBotPluginCommandTypes
{
	NAVBOT_PLUGINCMD_SCRIPTED = 0, // Scripted behavior, uses callbacks.
	NAVBOT_PLUGINCMD_ATTACK_MOVE, // Moves to a position, stops to attack enemies.
	NAVBOT_PLUGINCMD_MOVE_TO, // Moves to a position, attack enemies but don't stop.
	NAVBOT_PLUGINCMD_SEEK_AND_DESTROY, // Chase an entity until it's dead.
	NAVBOT_PLUGINCMD_USE_ENTITY, // Move to an entity and press the use button on it.
	NAVBOT_PLUGINCMD_WAIT, // Stops moving and waits for N seconds.
	NAVBOT_PLUGINCMD_PATROL, // Patrols the areas near the bot.
	NAVBOT_PLUGINCMD_ROAM, // Moves to a random destination.

	MAX_PLUGIN_COMMAND_TYPES
};

// A NavBot bot instance
methodmap NavBot < NavBotBasePlayer
{
	/**
	 * Adds a NavBot to the game. Methodmap version of AddNavBot.
	 * 
	 * @param botName 	Optional bot name.
	 * @return 			NavBot instance that was added or NULL if the bot was not added.
	 * @error			AddBot unavailable.
	 */
	public native NavBot(const char[] botName = NULL_STRING);
	/**
	 * Sets the skill group used by this bot.
	 * 
	 * @param skill_level	Skill group to use.
	 * @error				Invalid bot instance.
	 */
	public native void SetSkillLevel(int skill_level);
	/**
	 * Adds a client command to the bot client command's queue.
	 * 
	 * Use this to prevent the bot from getting kicked due to sending too many string commands.
	 * 
	 * @param cmd		The command to send.
	 * @error			Invalid bot instance or NULL string passed.
	 */
	public native void DelayedFakeClientCommand(const char[] cmd);
	// Resets the bot
	public native void Reset();
	/**
	 * Returns a memory address to the bot's inventory interface.
	 * 
	 * @return			Memory address of the bot's inventory interface.
	 */
	public native Address GetInventoryInterface();
	/**
	 * Returns a memory address to the bot's movement interface.
	 * 
	 * @return			Memory address of the bot's movement interface.
	 */
	public native Address GetMovementInterface();
	/**
	 * Returns a memory address to the bot's movement interface.
	 * 
	 * @return			Memory address of the bot's movement interface.
	 */
	public native Address GetSensorInterface();
	/**
	 * Returns a memory address to the bot's movement interface.
	 * 
	 * @return			Memory address of the bot's movement interface.
	 */
	public native Address GetPlayerControllerInterface();
	/**
	 * Tests if the bot's line of fire is clear to the given position.
	 * 
	 * @param vTo		Position to test if LOF is clear to.
	 * @return			True if clear, false if obstructed.
	 * @error			Invalid NavBot instance.
	 */
	public native bool IsLineOfFireClear(const float vTo[3]);
	// Makes the bot send the given impulse in their next UserCmd
	public native void SendImpulse(int impulse);
	/**
	 * Gets the bot debug identifier (useful when printing/logging debug messages specific to this bot instance)
	 * 
	 * @param buffer	Char array to store the identifier string (at least 128 recommended).
	 * @param size		Size of the buffer array.
	 */
	public native void GetDebugIdentifier(char[] buffer, int size);
	/**
	 * Sends a plugin command/order to the bot.
	 * 
	 * These commands won't work if gamemode behavior is disabled (sm_navbot_debug_disable_gamemode_ai 1).
	 * 
	 * @param type		Command/order type to send.
	 * @parma ...		Command parameters, depends on the command type.
	 * @error			Invalid bot, invalid command type, invalid or bad command arguments.
	 */
	public native void SendPluginCommand(NavBotPluginCommandTypes type, any ...);
	/**
	 * Sends a NAVBOT_PLUGINCMD_SCRIPTED plugin command/order to the bot.
	 * 
	 * @param update	Behavior update callback.
	 */
	public native void SendScriptedPluginCommand(NavBotScriptedBehaviorCB update);

	/**
	 * Expected arguments for each command type.
	 * 
	 * NAVBOT_PLUGINCMD_ATTACK_MOVE and NAVBOT_PLUGINCMD_MOVE_TO: One world position vector (float pos[3])
	 * NAVBOT_PLUGINCMD_SEEK_AND_DESTROY and NAVBOT_PLUGINCMD_USE_ENTITY: One entity index/reference to the target entity.
	 * NAVBOT_PLUGINCMD_WAIT: One float, the time in seconds the bot should wait
	 * NAVBOT_PLUGINCMD_PATROL: No additional parameters needed.
	 * NAVBOT_PLUGINCMD_ROAM: One float, the maximum search distance for a random destination.
	 */
}

typeset NavBotScriptedBehaviorCB
{
	/**
	 * Main callback for NAVBOT_PLUGINCMD_SCRIPTED.
	 * 
	 * @param bot			Bot running the scripted behavior.
	 * @param moveGoal		World position the bot will path to if the result is different from Plugin_Continue.
	 * @param routeType		Which route type the navigator should use when computing a path.
	 * @return				Return Plugin_Stop top end the behavior. Everything else keeps the behavior running.
	 */
	function Action (NavBot bot, float moveGoal[3], NavBotRouteType routeType);
};

/**
 * Called when a new NavBot Plugin Bot instance is about to be attached to an entity
 * 
 * @param entity Entity index that the plugin bot will be attached to.
 * @return Plugin_Continue to allow the plugin bot to be created. Anything else to prevent the bot from being created.
 */
forward Action OnPrePluginBotAdd(int entity);

/**
 * Called when a NavBot Plugin Bot was attached to an entity.
 * 
 * @param entity Client index that just got a plugin bot attached to.
 */
forward void OnPluginBotAdded(int entity);

const NavBot NULL_NAVBOT = view_as<NavBot>(0);

#include "interfaces/path.inc"
#include "interfaces/dynamic_priority.inc"
