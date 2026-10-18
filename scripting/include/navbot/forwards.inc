#if defined _navbot_forwards_included
 #endinput
#endif
#define _navbot_forwards_included

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
 * Called when running the bot's update cycle.
 */
forward void OnPreNavBotUpdate(NavBot bot);

/**
 * Called when NavBot's navigation mesh is loaded.
 */
forward void OnNavBotNavMeshLoaded();

/**
 * Called when NavBot's navigation mesh is destroyed (unloaded).
 */
forward void OnNavBotNavMeshDestroyed();

/**
 * Called when NavBot's navigation mesh is seeding walkable spots for generation.
 * Use NavBotNavMesh.AddWalkableSeed to add your own seeds.
 */
forward void OnNavBotNavMeshSeeding();

/**
 * Called when a bot is stuck.
 * 
 * @param bot		NavBot instance that got stuck.
 * @param count		Count of consecutive stuck events.
 */
forward void OnNavBotStuck(NavBot bot, int count);

/**
 * Called when the mod interface detects a round restart.
 */
forward void OnNavBotModRoundRestart();

enum NavBotComputePathFailureReason
{
	NAVBOT_COMPUTEPATH_FAILURE_NULL_START_AREA = 0, // Bot last known area is NULL
	NAVBOT_COMPUTEPATH_FAILURE_INCOMPLETE_PATH, // Pathfinding algorithm could not build a complete path to the destination

	MAX_NAVBOT_COMPUTEPATH_FAILURE_REASONS
}

/**
 * Called when a bot's navigator cannot build a complete path to the destination.
 * 
 * @param bot			NavBot instance the path is being computed for.
 * @param goal			World position the bot is trying to path to.
 * @param adjustedZ		Adjusted Z coordinate of the goal position.
 * @param reason		Path computation failure reason.
 */
forward void OnNavBotComputePathFailed(NavBot bot, const float goal[3], const float adjustedZ, NavBotComputePathFailureReason reason);

/**
 * Called when the bot's navigator detects an obstacle on the bot's path.
 * Allows plugins to implement custom obstacle handling.
 * 
 * @param bot		NavBot bot instance.
 * @param entity	Index of the entity blocking the path.
 * @param hitWorld	If true, the entity blocking the path is the world.
 * @param goal		Position the bot is currently trying to reach.
 * @return			Returning Plugin_Continue let's the navigator decide what do to about the obstacle. Anything else blocks it.
 */
forward Action OnNavBotObstacleOnPath(NavBot bot, int entity, bool hitWorld, const float goal[3]);
