#if defined _navbot_navmesh_included
 #endinput
#endif
#define _navbot_navmesh_included

#include "nav.inc"
#include "nav_area.inc"

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
	public static native Address GetNearestNavArea(const float pos[3], float maxDist, bool checkLOS, bool checkGround, int team = NAVBOT_NAV_TEAM_ANY);
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
	/**
	 * Collects nav areas overlapping the given extent.
	 * 
	 * @param mins			Extent lower bounds.
	 * @param maxs			Extent upper bounds.
	 * @return				Handle to a nav area vector. Needs to be deleted.
	 */
	public static native NavBotNavAreaVector CollectAreasOverlappingExtent(const float mins[3], const float maxs[3]);
	/**
	 * Collects nav areas overlapping the given extent. The extent bounds are calculated from the given entity.
	 * 
	 * @param entity		Entity to calculate the extent of.
	 * @param xy_offset		Offset to apply to the extent on the X and Y axis.
	 * @param z_offset		Offset to apply to the extent on the Z axis.
	 * @return				Handle to a nav area vector. Needs to be deleted.
	 */
	public static native NavBotNavAreaVector CollectAreasOverlappingExtentEx(int entity, const float xy_offset = 0.0, const float z_offset = 0.0);
	/**
	 * Collects nav areas overlapping the given entity bounds and collides with the entity via raycast traces.
	 * 
	 * @param entity		Entity to calculate the extent of.
	 * @return				Handle to a nav area vector. Needs to be deleted.
	 */
	public static native NavBotNavAreaVector CollectAreasTouchingEntity(int entity);
}

#include "nav_collector.inc"
#include "nav_blocker.inc"
