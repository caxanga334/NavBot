#if defined _navbot_bots_included
 #endinput
#endif
#define _navbot_bots_included

// A NavBot bot instance
methodmap NavBot
{
	/**
	 * Adds a NavBot to the game. Methodmap version of AddNavBot.
	 * 
	 * @param botName 	Optional bot name.
	 * @return 			NavBot instance that was added or NULL if the bot was not added.
	 * @error			AddBot unavailable.
	 */
	public native NavBot(const char[] botName = NULL_STRING);

	// True if this NavBot instance is a Plugin Bot.
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

methodmap PluginBot < NavBot
{
	/**
	 * Creates a new NavBot Plugin Bot instance or returns an existing one.
	 * @param client Client index to attach the NavBot instance to.
	 * @return PluginBot instance. Will be NULL on failure. Use IsNull to check.
	 * @error Invalid client index.
	 * @note This will return NULL if used on a valid NavBot that is not a Plugin Bot!
	 */
	public native PluginBot(int client);

	/**
	 * Plugin bots by default will not run player commands (call CBasePlayer::PlayerRunCommand).
	 * This means the plugin is responsible for sending them.
	 * If the plugin bot is attached on a human, then the plugin will always be responsible for running player commands.
	 * 
	 * @param run If true, the extension will run player commands for this PluginBot instance. If false the plugin will need to run player commands.
	 * @error If trying to enable commands on a Plugin Bot attached to a non fake client (human player).
	 */
	public native void SetRunPlayerCommands(bool run);
}

/**
 * Gets a NavBot instance by client index.
 * @param client  Client index
 * @return  NavBot instance or NULL_NAVBOT is a navbot doesn't exists for the given client index.
 */
native NavBot GetNavBotByIndex(int client);

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
const PluginBot NULL_PLUGINBOT = view_as<PluginBot>(0);

#include "interfaces/path.inc"