#if defined _navbot_interfaces_playercontroller_included
 #endinput
#endif
#define _navbot_interfaces_playercontroller_included

/**
 * @brief Priorities for look/aim commands.
 */
enum NavBotLookPriority
{
	LOOK_IDLE = 0, // Idle look.
	LOOK_PATH, // Looking at the direction I am going
	LOOK_INTERESTING, // Looking at something interesting
	LOOK_ALLY, // Looking at my teammate
	LOOK_SEARCH, // Looking at potential enemy positions
	LOOK_ALERT, // Looking at something that alerted me
	LOOK_DANGER, // Looking at a danger source
	LOOK_USE, // Looking to use a door, button, etc
	LOOK_COMBAT, // Looking at en enemy
	LOOK_SUPPORT, // Looking to support my teammates
	LOOK_PRIORITY, // Looking at something that requires high priority
	LOOK_MOVEMENT, // Movement that requires looking in a specific direction (ie: ladders, jumps)
	LOOK_CRITICAL, // Something of very high importance
	LOOK_MANDATORY, // Nothing can interrupt this

	MAX_LOOK_PRIORITY
};

// Interface responsible for player inputs.
methodmap NavBotPlayerControllerInterface < NavBotBotInterface
{
	/**
	 * Instructs the player controller to aim at this entity.
	 * 
	 * @param ptr			Interface pointer.
	 * @param entity		Entity to aim at.
	 * @param priority		Aim at command priority.
	 * @param duration		How long to keep aiming in seconds.
	 * @param reason		An optional message explaining why this command was given (printed when debugging is enabled).
	 */
	public static native void AimAtEntity(Address ptr, int entity, NavBotLookPriority priorty, float duration, const char[] reason = NULL_STRING);
	/**
	 * Instructs the player controller to aim at this entity.
	 * 
	 * @param ptr			Interface pointer.
	 * @param vec			Position to aim at.
	 * @param priority		Aim at command priority.
	 * @param duration		How long to keep aiming in seconds.
	 * @param reason		An optional message explaining why this command was given (printed when debugging is enabled).
	 */
	public static native void AimAtPos(Address ptr, const float vec[3], NavBotLookPriority priorty, float duration, const char[] reason = NULL_STRING);
	// Returns true if the bot aim is currently on the aim target.
	public static native bool IsAimOnTarget(Address ptr);
}