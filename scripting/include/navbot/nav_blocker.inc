#if defined _navbot_navmesh_nav_blocker_included
 #endinput
#endif
#define _navbot_navmesh_nav_blocker_included

typeset NavBotNavBlockerCB
{
	/**
	 * General callback for nav blockers. Used by multiple callbacks.
	 * 
	 * @param blocker           Handle of the current blocker instance.
	 * @return					Returing false marks the blocker as invalid. Invalid blockers are removed by the nav mesh.
	 */
	function bool (NavBotNavBlocker blocker);
	/**
	 * Callback for OnDestroyed.
	 * 
	 * @param blocker			Handle of the current blocker instance. The handle is NO LONGER VALID and should only be used as a reference.
	 */
	function void (NavBotNavBlocker blocker);
};

methodmap NavBotNavBlocker < Handle
{
	/**
	 * Creates a new nav blocker.
	 * 
	 * @param initCallback		Initialization callback function.
	 * @param updateCallback	Update callback. (This is called at intervals to update the blocker's blocked/unblocked status).
	 * @param name				Optional name for this blocker.
	 */
	public native NavBotNavBlocker(NavBotNavBlockerCB initCallback, NavBotNavBlockerCB updateCallback, const char[] name = NULL_STRING);
	/**
	 * Add areas to the blocker. These are the areas that will be blocked by this blocker instance.
	 * 
	 * @param vector            Handle to a NavBotNavAreaVector.
	 * @error                   Bad handle.
	 */
	public native void AddAreas(NavBotNavAreaVector vector);
	/**
	 * Updates the blocked status.
	 * 
	 * @param teamID			Team to Update the blocked status of. Use NAVBOT_NAV_TEAM_ANY to update all teams.
	 * @param blocked			True to update to blocked, false to update to unblocked.
	 */
	public native void UpdateBlockedStatus(int teamID, bool blocked);

	// Optional callback. Called when the round restarts.
	property NavBotNavBlockerCB OnRoundRestart {
		public native set(NavBotNavBlockerCB callback);
	}
	// Optional callback. Called when recomputing the nav mesh's internal data.
	property NavBotNavBlockerCB OnRecomputeInternalData {
		public native set(NavBotNavBlockerCB callback);
	}
	// Optional callback. Called when the blocker is about to be destroyed. (The handle is no longer valid at this point and should only be used as a reference)
	property NavBotNavBlockerCB OnDestroyed {
		public native set(NavBotNavBlockerCB callback);
	}
	// Get/Set an entity stored in the blocker's internal EHANDLE, For convenience.
	property int Entity {
		public native set(int entity);
		public native get();
	}
}
