#if defined _navbot_interfaces_sensor_included
 #endinput
#endif
#define _navbot_interfaces_sensor_included

// Interface that handles how the bot moves.
methodmap NavBotSensorInterface < NavBotBotInterface
{
	// Returns true if this entity is ignored by the sensor interface.
	public static native bool IsIgnored(Address ptr, int entity);
	// Returns true if this entity is considered friendly to the bot.
	public static native bool IsFriendly(Address ptr, int entity);
	// Returns true if this entity is considered an enemy to te bot.
	public static native bool IsEnemy(Address ptr, int entity);
	/**
	 * Tests if the bot can see the given position.
	 * 
	 * @param ptr           Pointer to the bot's ISensor interface.
	 * @param position      Position to test.
	 * @param checkFOV      If true, also tests for Field of View.
	 * @return              True if the bot can see, false otherwise.
	 * @error               Bad pointer passed.
	 */
	public static native bool IsAbleToSeePos(Address ptr, const float position[3], const bool checkFOV = true);
	/**
	 * Tests if the bot can see the given entity.
	 * 
	 * @param ptr           Pointer to the bot's ISensor interface.
	 * @param position      Entity to test.
	 * @param checkFOV      If true, also tests for Field of View.
	 * @return              True if the bot can see, false otherwise.
	 * @error               Bad pointer passed, NULL entity.
	 */
	public static native bool IsAbleToSeeEntity(Address ptr, int entity, const bool checkFOV = true);
	// Tests if the bot has a clear line of sight to the given position.
	public static native bool IsLineOfSightClearPos(Address ptr, const float position[3]);
	// Tests if the bot has a clear line of sight to the given entity.
	public static native bool IsLineOfSightClearEntity(Address ptr, int entity);
	// Tests if the given position is in the bot's field of view.
	public static native bool IsInFieldOfView(Address ptr, const float position[3]);
	/**
	 * Overrides the bot's primary threat entity.
	 * 
	 * @param ptr           Pointer to the bot's ISensor interface.
	 * @param entity		Entity to set as the primary threat override. Pass INVALID_ENT_REFERENCE to clear it.
	 * @error               Bad pointer passed.
	 */
	public static native void SetPrimaryThreatOverride(Address ptr, int entity);
	/**
	 * Gets the bot's primary known threat. (This is the entity the bot will attack)
	 * 
	 * @param ptr           Pointer to the bot's ISensor interface.
	 * @param visibleOnly	If true, only visible entities may be selected else any entity.
	 * @return				Primary known threat entity index.
	 * @error               Bad pointer passed.
	 */
	public static native int GetPrimaryKnownThreatIndex(Address ptr, bool visibleOnly = false);
}