#if defined _navbot_navarea_included
 #endinput
#endif
#define _navbot_navarea_included

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
	// Retrieves the nav area center world position.
	public static native void GetCenter(Address ptr, float center[3]);
	/**
	 * Retrieves the nav area corner world position.
	 * 
	 * @param ptr			Address of a nav area.
	 * @param corner		Corner to get the position of.
	 * @param vec			Float array to write the corner's position.
	 * @error				Bad area pointer. Invalid corner type.
	 */
	public static native void GetCorner(Address ptr, NavBotNavCornerType corner, float vec[3]);
	// Retrieves the nav area's extent.
	public static native void GetExtent(Address ptr, float mins[3], float maxs[3]);
	/**
	 * Tests if this nav area is visible from the given position.
	 * 
	 * @param ptr			Address of a nav area.
	 * @param eye			Position to test for visibiity.
	 * @param visSpot		Which position of the nav area can be seen. This is optional.
	 * @return				True if the nav area is visible, false otherwise.
	 * @error				Bad area pointer.
	 */
	public static native bool IsVisible(Address ptr, const float eye[3], float visSpot[3] = NULL_VECTOR);
	/**
	 * Gets a nav area blocked status.
	 * 
	 * @param ptr				Address of a nav area.
	 * @param teamID			Team index to check. For any team use NAVBOT_NAV_TEAM_ANY.
	 * @param ignoreNavBlockers	If true, ignore areas blocked by nav blockers.
	 * @return					True if the area is blocked for the given team, false otherwise.
	 */
	public static native bool IsBlocked(Address ptr, int teamID, bool ignoreNavBlockers = false);
	/**
	 * Gets the closest point on this area.
	 * 
	 * @param ptr			Address of a nav area.
	 * @param pos			Position to get the closest point of.
	 * @param close			Float array to store the closest point.
	 */
	public static native void GetClosestPointOnArea(Address ptr, const float pos[3], float close[3]);
}

methodmap NavBotNavAreaVector < Handle
{
	// Constructs a new empty area vector. Must be deleted.
	public native NavBotNavAreaVector();
	// Adds a nav area to the vector. This does not checks for duplicates
	public native void Push(Address area);
	// Returns true if the given area is in the vector.
	public native bool Contains(Address area);
	// Removes a nav area from the vector.
	public native void Erase(Address area);
	// Removes all nav areas from the vector.
	public native void Clear();
	/**
	 * Access a nav area at the given index.
	 * 
	 * @param index			Index to access. Range: 0 to (Size - 1)
	 * @return				Pointer to a NavBotNavArea.
	 * @error				Invalid handle. Out of bounds index.
	 */
	public native Address At(int index);
	// The number of nav areas in the vector.
	property int Size {
		public native get();
	}
	// Returns true if the vector is empty.
	public bool IsEmpty()
	{
		return this.Size == 0;
	}
}