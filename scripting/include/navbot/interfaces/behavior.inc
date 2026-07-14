#if defined _navbot_interfaces_behavior_included
 #endinput
#endif
#define _navbot_interfaces_behavior_included

// Behavior queries answer types
enum NavBotQueryAnswerType
{
	// This value is exclusive to the plugin API, it's not a real answer type on the C++ side. It's used to signal plugins a valid answer cannot be obtained.
	NAVBOT_NO_VALID_ANSWER = -1,
	// All values below also exists in the C++ side.
	NAVBOT_ANSWER_NO = 0, // Definitive NO answer.
	NAVBOT_ANSWER_YES, // Definitive YES answer.
	NAVBOT_ANSWER_UNDEFINED, // No definitive answer was given.

	NAVBOT_MAX_ANSWER_TYPES = 3
};

methodmap NavBotBehaviorInterface < NavBotBotInterface
{
	/**
	 * Queries the bot behavior to see if they should attack the given entity.
	 * 
	 * @param ptr			Address to the bot behavior interface.
	 * @param entity		Entity being queried.
	 * @return				Query answer. Returns NAVBOT_NO_VALID_ANSWER if the given entity is not known by the bot's sensor.
	 */
	public static native NavBotQueryAnswerType ShouldAttack(Address ptr, int entity);
	// Queries if the bot should hurry.
	public static native NavBotQueryAnswerType ShouldHurry(Address ptr);
	// Queries if the bot should retreat.
	public static native NavBotQueryAnswerType ShouldRetreat(Address ptr);
	/**
	 * Queries if the bot should assist the given teammate.
	 * 
	 * @param ptr			Address to the bot behavior interface.
	 * @param teammate		Teammate being queried.
	 * @return				Query answer. Returns NAVBOT_NO_VALID_ANSWER if the given entity is not seen as a teammate by the bot.
	 */
	public static native NavBotQueryAnswerType ShouldAssistTeammate(Address ptr, int teammate);
	/**
	 * Queries if the bot is running a plugin command task.
	 * 
	 * @return				True if yes, false if no.
	 */
	public static native bool IsRunningPluginCommand(Address ptr);
}