#if defined _navbot_mod_zps_interface_included
 #endinput
#endif
#define _navbot_mod_zps_interface_included

enum NavBotZPSObjectiveTypes
{
	NAVBOT_ZPS_OBJECTIVE_NONE = 0,
	NAVBOT_ZPS_OBJECTIVE_MOVETO, // move to a position
	NAVBOT_ZPS_OBJECTIVE_USE_BUTTON, // press a button
	NAVBOT_ZPS_OBJECTIVE_FIND_ITEM, // find item_generic
	NAVBOT_ZPS_OBJECTIVE_USE_ITEM, // use item_generic
	NAVBOT_ZPS_OBJECTIVE_DROP_ITEM, // drop item_generic at another entity
	NAVBOT_ZPS_OBJECTIVE_DESTROY_ENTITY, // destroy an entity

	NAVBOT_ZPS_MAX_OBJECTIVE_TYPES
};

/**
 * Zombie Panic! Source exclusive mod interface API. Using these functions outside ZPS will result in errors.
 */
methodmap NavBotZPSModInterface < NavBotModInterface
{
	// Changes the current objective. This will fire an event to the bot behavior.
	public static native void SetCurrentObjective(NavBotZPSObjectiveTypes objective);
	/**
	 * Sets the move goal position. Used by NAVBOT_ZPS_OBJECTIVE_MOVETO.
	 * 
	 * @param goal			Move goal world position.
	 */
	public static native void SetObjectiveMoveGoal(const float goal[3]);
	/**
	 * Sets the use button target. Used by NAVBOT_ZPS_OBJECTIVE_USE_BUTTON.
	 */
	public static native void SetObjectiveUseButton(int entity);
	/**
	 * Sets the item id to search. The ID must match the ID of an item_generic entity.
	 * Used by NAVBOT_ZPS_OBJECTIVE_FIND_ITEM, NAVBOT_ZPS_OBJECTIVE_USE_ITEM, NAVBOT_ZPS_OBJECTIVE_DROP_ITEM
	 * 
	 * @param id			ID to search.
	 */
	public static native void SetObjectiveItemSearchID(const char[] itemID);
	/**
	 * Sets the trigger_useable target for NAVBOT_ZPS_OBJECTIVE_USE_ITEM.
	 * 
	 * @param entity		Target trigger_useable entity.
	 */
	public static native void SetObjectiveItemUseTarget(int entity);
	/**
	 * Generic detection radius. Depends on the current objective.
	 * 
	 * NAVBOT_ZPS_OBJECTIVE_FIND_ITEM: Sets the minimum distance for the bots to detect an item_generic.
	 */
	public static native void SetObjectiveDetectionRadius(const float radius);
	/**
	 * Generic target entity, multi purpose, depends on the objective.
	 * 
	 * NAVBOT_ZPS_OBJECTIVE_USE_ITEM: Tell bots to use this specific item_generic.
	 * NAVBOT_ZPS_OBJECTIVE_DESTROY_ENTITY: The entity the bots should destroy.
	 */
	public static native void SetObjectiveGenericTargetEntity(int entity);
	// Reset all objective variables and set the current objective to none.
	public static native void ResetObjective();
}