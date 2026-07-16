#if defined _navbot_navarea_included
 #endinput
#endif
#define _navbot_navarea_included

enum OffMeshConnectionType
{
	OFFMESH_INVALID = 0,
	OFFMESH_GROUND, // solid ground
	OFFMESH_TELEPORTER, // map based teleporter (trigger_teleport)
	OFFMESH_BLAST_JUMP, // Blast/Rocket Jump
	OFFMESH_DOUBLE_JUMP, // Double jump (climb)
	OFFMESH_JUMP_OVER_GAP, // Jump over gap
	OFFMESH_CLIMB_UP, // Climb/Jump
	OFFMESH_DROP_DOWN, // Drop from ledge
	OFFMESH_GRAPPLING_HOOK, // Grappling hook
	OFFMESH_CATAPULT, // Catapult/push
	OFFMESH_STRAFE_JUMP, // A jump that requires strafing over an obstacle
	OFFMESH_PUSH_LADDER, // Ladder made using trigger_push/other push entities. (Common in TF2)
	OFFMESH_CHEAT_TELEPORT, // Cheat: Teleport the bot to the end position.

	MAX_OFFMESH_CONNECTION_TYPES // max known link types
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
	/**
	 * Checks if a nav area is connected to another area via any connection method.
	 * 
	 * @param ptr			Address of the first area.
	 * @param other			Address of the second area.
	 * @return				True if the first area has a connection to the second area, false otherwise.
	 */
	public static native bool IsConnectedToAny(Address ptr, Address other);
	/**
	 * Checks if a nav area is connected to another area via ground connections.
	 * 
	 * @param ptr			Address of the first area.
	 * @param other			Address of the second area.
	 * @param dir			Direction to check.
	 * @return				True if the first area has a connection to the second area in the given direction, false otherwise.
	 */
	public static native bool IsConnectedToDir(Address ptr, Address other, NavBotNavDirType dir);
	/**
	 * Checks if a nav area is connected to another area via off-mesh connection.
	 * 
	 * @param ptr			Address of the first area.
	 * @param other			Address of the second area.
	 * @param type			Off-mesh connection type to check.
	 * @return				True if the first area has a connection to the second area via the given off-mesh connection type, false otherwise.
	 */
	public static native bool IsConnectedToOffMesh(Address ptr, Address other, OffMeshConnectionType type);
	// Gets the number of off-mesh connection this nav area have.
	public static native int GetOffMeshConnectionCount(Address ptr);
	/**
	 * Gets the address of an off-mesh connection.
	 * 
	 * @param ptr			Address of a nav area.
	 * @param index			Index of the off mesh connection. Range is 0 to GetOffMeshConnectionCount - 1.
	 * @return				Address of the off-mesh connection object.
	 * @error				Bad area address, out of bounds index.
	 */
	public static native Address GetOffMeshConnection(Address ptr, const int index);
	/**
	 * Collects nav areas this area is connected to.
	 * 
	 * @param ptr			Address of a nav area.
	 * @param vec			Handle to a nav area vector, the connected areas are added to the vector.
	 */
	public static native void CollectConnectedAreas(Address ptr, NavBotNavAreaVector vec);
	/**
	 * Computes the height change between two nav areas.
	 * 
	 * @param ptr			Address of the first area.
	 * @param other			Address of the second area.
	 * @return				Height change between two areas. Returns a very large number if the height change cannot be calculated.
	 */
	public static native float ComputeAdjacentConnectionHeightChange(Address ptr, Address other);
	/**
	 * Computes the gap distance between two nav areas.
	 * 
	 * @param ptr			Address of the first area.
	 * @param other			Address of the second area.
	 * @return				Gap distance between two areas. Returns a very large number if the gap distance cannot be calculated.
	 */
	public static native float ComputeAdjacentConnectionGapDistance(Address ptr, Address other);
}

methodmap NavBotNavOffMeshConnection
{
	/**
	 * Gets the off-mesh connection type.
	 * 
	 * @param ptr			Address of a NavOffMeshConnection object.
	 * @return				Type of this connection.
	 */
	public static native OffMeshConnectionType GetType(Address ptr);
	/**
	 * Gets the conenction start position.
	 * 
	 * @param ptr			Address of a NavOffMeshConnection object.
	 * @param pos			Float array to write the position.
	 */
	public static native void GetStartPosition(Address ptr, float pos[3]);
	/**
	 * Gets the conenction end position.
	 * 
	 * @param ptr			Address of a NavOffMeshConnection object.
	 * @param pos			Float array to write the position.
	 */
	public static native void GetEndPosition(Address ptr, float pos[3]);
	/**
	 * Gets the nav area this off-mesh connection connects to.
	 * 
	 * @param ptr			Address of a NavOffMeshConnection object.
	 * @return				Address of the nav area it's connected to.
	 */
	public static native Address GetConnectedArea(Address ptr);
	/**
	 * Gets the off-mesh connection length.
	 * 
	 * @param ptr			Address of a NavOffMeshConnection object.
	 * @return				Connection length.
	 */
	public static native float GetConnectionLength(Address ptr);
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