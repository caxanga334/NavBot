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
	/**
	 * Instructs the movement interface to climb up a ledge.
	 * 
	 * @param ptr			Pointer to the movement interface.
	 * @param landingGoal	World positon the bot will land after climbing.
	 * @return				Returns false if the bot can't climb, true otherwise.
	 */
	public static native bool ClimbUpToLedge(Address ptr, const float landingGoal[3]);
	/**
	 * Instructs the movement interface to climb up a ledge using double jumps.
	 * 
	 * @param ptr			Pointer to the movement interface.
	 * @param landingGoal	World positon the bot will land after climbing.
	 * @return				Returns false if the bot can't climb, true otherwise.
	 */
	public static native bool DoubleJumpToLedge(Address ptr, const float landingGoal[3]);
	/**
	 * Instructs the movement interface to jump across a gap.
	 * 
	 * @param ptr			Pointer to the movement interface.
	 * @param landingGoal	World positon the bot will land after jumping.
	 */
	public static native void JumpAcrossGap(Address ptr, const float landingGoal[3]);
	/**
	 * Instructs the movement interface to blast jump.
	 * 
	 * @param ptr			Pointer to the movement interface.
	 * @param landingGoal	World positon the bot will land after blast jumping.
	 */
	public static native void BlastJumpTo(Address ptr, const float landingGoal[3]);
	/**
	 * Instructs the movement interface to break an obstacle.
	 * The bot will walk in a straight line to the obstacle, make sure it's reachable before calling this.
	 * 
	 * @param ptr			Pointer to the movement interface.
	 * @param entity		Entity the bot will break.
	 * @error				Bad ptr, invalid entity passed.
	 */
	public static native void BreakObstacle(Address ptr, const int entity);
	/**
	 * Adds a 'dead' area. Dead areas are considered blocked for pathing.
	 * 
	 * @param ptr			Pointer to the movement interface.
	 * @param area			Pointer of the nav area to be added to the dead area list.
	 * @param duration		How long in seconds to consider this area dead, negative for infinite.
	 */
	public static native void AddDeadArea(Address ptr, Address area, const float duration);
	/**
	 * Adds a cost multiplier to a specific nav area, exclusive to this bot only.
	 * The goal of the pathfinding algorithm is to find the shortest path, which is the path with the smallest cost.
	 * Areas are given a cost based on the distance the bot has to travel.
	 * This function allows you to add weights to a specific area.
	 * If the cost multiplier is less than 1, the algorithm will prefer this area.
	 * If the cost is higher than 1, the algorithm will avoid this area.
	 * 
	 * @param ptr			Pointer to the movement interface.
	 * @param area			Pointer of the nav area to be added to the dead area list.
	 * @param costmult		Area traverse cost multiplier. Cannot be zero or negative.
	 * @param duration		How long in seconds to consider this area dead, negative for infinite.
	 */
	public static native void AddCostModArea(Address ptr, Address area, const float costmult, const float duration);
}