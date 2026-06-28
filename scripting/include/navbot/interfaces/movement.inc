#if defined _navbot_interfaces_movement_included
 #endinput
#endif
#define _navbot_interfaces_movement_included

// Interface that handles how the bot moves.
methodmap NavBotMovementInterface < NavBotBotInterface
{
	/**
	 * Checks if the bot is able to move between 'from' and 'to'.
	 * 
	 * @param ptr			Pointer to the movement interface (that you get from NavBot.GetMovementInterface()).
	 * @param vecFrom		The 'from' position.
	 * @param vecTo			The 'to' position.
	 * @parma fraction		Trace fraction left is stored here.
	 * @param now			If true, only check if the bot is able to move right now. If false, check if the bot is able to move in the future.
	 * @param obstacle		Entity index of the entity the trace collided with.
	 * @return				True if traversable, false otherwise.
	 * @error				Bad pointer address.
	 */
	public static native bool IsPotentiallyTraversable(Address ptr, const float vecFrom[3], const float vecTo[3], float& fraction, bool now, int& obstacle);
	// Returns true if the bot is stuck
	public static native bool IsStuck(Address ptr);
	// Returns how long the bot has been stuck in seconds.
	public static native float GetStuckDuration(Address ptr);
	/**
	 * Checks if the bot can traverse the given area.
	 * 
	 * @param ptr			Pointer to the movement interface.
	 * @param area			Pointer to the nav area.
	 * @return				True if the bot can traverse the given area, false otherwise.
	 * @error				Bad interface pointer. Bad nav area pointer.
	 */
	public static native bool IsAreaTraversable(Address ptr, Address area);
	/**
	 * Clears the bot stuck status. Useful to prevent the bots from thinking they're stuck because they can't move.
	 * 
	 * @param ptr			Pointer to the movement interface.
	 * @param reason		Optional reason of why the stuck status was cleared (for debugging).
	 */
	public static native void ClearStuckStatus(Address ptr, const char[] reason = NULL_STRING);
}