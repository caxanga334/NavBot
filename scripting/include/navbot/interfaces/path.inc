#if defined _navbot_interfaces_path_included
 #endinput
#endif
#define _navbot_interfaces_path_included

methodmap MeshNavigator
{
	/**
	 * Allocates a new mesh navigator instance. All navigators are destroyed on map end.
	 */
	public native MeshNavigator();

	property int Index
	{
		public get() { return view_as<int>(this); }
	}

	property bool IsNull
	{
		public get() { return this.Index <= 0; }
	}

	/**
	 * Deletes this navigator from memory.
	 */
	public native void Destroy();

	/**
	 * Moves the bot along the path.
	 * Currently plugin bots doesn't move by themselves. You're supposed to handle their movement.
	 * This doesn't need to be called on every frame but should be called frequently. At least ten times per second.
	 * 
	 * @param bot NavBot plugin bot that will use this path.
	 * @error Invalid path, invalid bot or not a plugin bot.
	 */
	public native void Update(NavBot bot);

	/**
	 * Whether the current path has built a path or not.
	 *
	 * @return     True if valid, false otherwise
	 * @error Invalid path
	 */
	public native bool IsValid();

	/**
	 * Computes a path to the given world position.
	 * 
	 * @param bot NavBot plugin bot that will use this path.
	 * @param goalPos World position to compute to.
	 * @param maxPathLength Maximum travel distance.
	 * @param includeGoalIfPathFails If true, then the path will include the goal regardless of failure.
	 * @return True if path reached to goal. If false, path may or may not be valid. Use IsValid() to check.
	 * @error Invalid path, invalid bot or not a plugin bot.
	 */
	public native bool ComputeToPos(NavBot bot, const float goalPos[3], float maxPathLength = 0.0, bool includeGoalIfPathFails = true);

	/**
	 * Gets the position the bot navigation is trying to reach.
	 * @param moveGoal The move goal position will be stored here.
	 * @error Invalid path
	 */
	public native void GetMoveGoal(float moveGoal[3]);
}

const MeshNavigator NULL_MESHNAVIGATOR = view_as<MeshNavigator>(-1);