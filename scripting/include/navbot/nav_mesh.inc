#if defined _navbot_navmesh_included
 #endinput
#endif
#define _navbot_navmesh_included

// In the nav mesh code, this value is used to signal 'ANY' team.
#define NAV_TEAM_ANY -2

// Codes for load results
enum NavBotNavErrorType
{
	NAVBOT_NAV_OK,
	NAVBOT_NAV_CANT_ACCESS_FILE,
	NAVBOT_NAV_INVALID_FILE,
	NAVBOT_NAV_BAD_FILE_VERSION,
	NAVBOT_NAV_FILE_OUT_OF_DATE,
	NAVBOT_NAV_CORRUPT_DATA,
	NAVBOT_NAV_OUT_OF_MEMORY,

	NAVBOT_MAX_NAV_ERROR_TYPES
};

// Nav area functions.
methodmap NavBotNavArea
{
	/**
	 * Returns the nav area's ID.
	 * 
	 * @param ptr			Address of a nav area.
	 * @return				Nav area's ID.
	 */
	public static native int GetID(Address ptr);
	// Retrieves the nav area center world position
	public static native void GetCenter(Address ptr, float center[3]);
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
	public static native Address GetNearestNavArea(const float pos[3], float maxDist, bool checkLOS, bool checkGround, int team = NAV_TEAM_ANY);
	/**
	 * Retrives a nav area instance of the given ID.
	 * 
	 * @param id			Nav area ID to search for.
	 * @return				Nav area instance if found or NULL if not found.
	 */
	public static native Address GetNavAreaByID(const int id);
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
	// Gets the result code of the last load attempt.
	public static native NavBotNavErrorType GetLastLoadResult();
	/**
	 * Generates a nav mesh if the current map doesn't have one.
	 * Generation is delayed by one frame.
	 * If you want to check the result, use a timer.
	 */
	public static native void Generate();
	// Loads a navigation mesh from a file (does nothing if it's already loaded).
	public static native void Load();
	/**
	 * Gets the full path to the nav mesh file for the current map.
	 * 
	 * @param isLoad		True if you want to get the file for loading (only makes a difference for games with workshop map support).
	 * @param path			Buffer to store the path.
	 * @param size			Size of the path buffer.
	 */
	public static native void GetFullPathToNavMeshFile(bool isLoad, char[] path, int size);
}

