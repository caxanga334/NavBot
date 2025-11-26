#if defined _navbot_bots_included
 #endinput
#endif
#define _navbot_bots_included

#include "interfaces/base_interface.inc"
#include "interfaces/inventory.inc"

// Pass this to SetSkillLevel to generate a random difficulty profile for the bot.
#define NAVBOT_SKILL_RANDOM -1

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
	/**
	 * Returns a memory address to the bot's inventory interface.
	 * 
	 * @return			Memory address of the bot's inventory interface.
	 */
	public native NavBotInventoryInterface GetInventoryInterface();

	property int Index
	{
		public get() { return view_as<int>(this); }
	}

	property bool IsNull
	{
		public get() { return this.Index == 0; }
	}
}

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
