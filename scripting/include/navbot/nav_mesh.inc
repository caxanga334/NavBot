#if defined _navbot_navmesh_included
 #endinput
#endif
#define _navbot_navmesh_included

// In the nav mesh code, this value is used to signal 'ANY' team.
#define NAV_TEAM_ANY -2

methodmap NavBotNavArea
{
	// Returns the nav area ID
	public native int GetID();
	// Retrieves the nav area center world position
	public native void GetCenter(float center[3]);
	// Is this nav area NULL?
	property bool IsNull
	{
		public get() { return view_as<Address>(this) == Address_Null; }
	}
}

methodmap NavBotNavMesh
{
	// Returns true if the nav mesh is loaded
	public static native bool IsLoaded();
	/**
	 * Gets the nearest nav area to the given position.
	 * 
	 * @param pos			Position to get the nearest nav area of.
	 * @param maxDist		Maximum distance between the nav area and the position.
	 * @param checkLOS		If true, check for line of sight between the Nav Area and the given position.
	 * @param checkGround	If true, check for solid ground.
	 * @param team			Optional team filter when searching for areas.
	 * @return				Nav area instance if found or NULL if not found.
	 */
	public static native NavBotNavArea GetNearestNavArea(const float pos[3], float maxDist, bool checkLOS, bool checkGround, int team = NAV_TEAM_ANY);
	/**
	 * Retrives a nav area instance of the given ID.
	 * 
	 * @param id			Nav area ID to search for.
	 * @return				Nav area instance if found or NULL if not found.
	 */
	public static native NavBotNavArea GetNavAreaByID(const int id);
	/**
	 * Gets the name and Steam ID (in SteamID64 format) of the nav mesh author.
	 * The author is the person who made the nav mesh.
	 * 
	 * @param name			Buffer to store the name.
	 * @param namesize		Size of the name buffer.
	 * @param steamid		Buffer to store the steamid.
	 * @param steamidsize	Size of the steamid buffer.
	 * @return				Returns true if author information is available and was written to the buffers, false otherwise
	 */
	public static native bool GetAuthor(char[] name, int namesize, char[] steamid, int steamidsize);
	/**
	 * Gets the number of people who edited the nav mesh.
	 * 
	 * This does not includes the person who originally made the nav mesh, use GetAuthor for that.
	 */
	public static native int GetEditorCount();
	/**
	 * Gets the name and Steam ID (in SteamID64 format) of a person who edited the nav mesh.
	 * An editor is a person who edited the nav mesh but is not the one who made it originally.
	 * 
	 * @param index			Index of the editor, starts at 0 and goes up to GetEditorCount() - 1;
	 * @param name			Buffer to store the name.
	 * @param namesize		Size of the name buffer.
	 * @param steamid		Buffer to store the steamid.
	 * @param steamidsize	Size of the steamid buffer.
	 * @error				Invalid editor index used.
	 */
	public static native void GetEditor(int index, char[] name, int namesize, char[] steamid, int steamidsize);
}

const NavBotNavArea NULL_NAVBOTNAVAREA = view_as<NavBotNavArea>(Address_Null);