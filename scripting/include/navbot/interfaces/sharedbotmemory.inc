#if defined _navbot_sharedbotmemory_interface_included
 #endinput
#endif
#define _navbot_sharedbotmemory_interface_included

// Interface used by bots to share information.
// In games that uses the patrol behavior, this also handles the sharing of enemy positions.
methodmap NavBotSharedBotMemory
{
	// Constructor
	public NavBotSharedBotMemory(int teamIndex)
	{
		return view_as<NavBotSharedBotMemory>(teamIndex);
	}
	// The team this instance belongs to.
	property int TeamNum
	{
		public get() { return view_as<int>(this); }
	}
	/**
	 * Adds or updates an entity report entry.
	 * 
	 * @param entity        Index or reference of an entity to report.
	 * @error               Invalid team index.
	 */
	public native void ReportEntity(int entity);
	/**
	 * Removes an entity report entry if it exists.
	 * 
	 * @param entity        Index or reference of an entity to remove the report of.
	 * @error               Invalid team index.
	 */
	public native void ForgetEntity(int entity);
}