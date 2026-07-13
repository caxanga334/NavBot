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

	NAVBOT_ZPS_MAX_OBJECTIVE_TYPES
};

/**
 * Zombie Panic! Source exclusive mod interface API. Using these functions outside ZPS will result in errors.
 */
methodmap NavBotZPSModInterface < NavBotModInterface
{
	public static native void SetCurrentObjective(NavBotZPSObjectiveTypes objective);
	public static native void SetObjectiveMoveGoal(const float goal[3]);
	public static native void SetObjectiveUseButton(int entity);
	public static native void SetObjectiveItemSearchID(const char[] itemID);
	public static native void SetObjectiveItemUseTarget(int entity);
	public static native void SetObjectiveDetectionRadius(const float radius);
	public static native void ResetObjective();
}