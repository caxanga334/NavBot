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

/**
 * Callback for SearchConnectedAreas.
 * 
 * @param area			The nav area the function was called on.
 * @param neighbor		Nav area connected to the area from the first param.
 * @param how			How the neighbor area can be reached.
 * @param data			Additional plugin specific data passed to the callback.
 * @return				Return false to stop the search, true to continue.
 */
typedef NavBotNavAreaSearchAdjCB = function bool (Address area, Address neighbor, NavBotNavTraverseType how, any data);

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
	/**
	 * Gets the number of adjacent areas in the given direction.
	 * 
	 * @param ptr			Address of a nav area.
	 * @param dir			Direction.
	 * @return				Number of adjacent areas in the given direction.
	 */
	public static native int GetAdjacentAreaCount(Address ptr, NavBotNavDirType dir);
	/**
	 * Gets the address of an adjacent nav area.
	 * 
	 * @param ptr			Address of a nav area.
	 * @param dir			Direction.
	 * @param index			Adjacent area index. Starts at 0 and ends at GetAdjacentAreaCount - 1.
	 */
	public static native Address GetAdjacentArea(Address ptr, NavBotNavDirType dir, const int index);
	/**
	 * Adds an adjacent connection between two nav areas.
	 * 
	 * @param from			Address of the first nav area.
	 * @parma to			Address of the second nav area.
	 * @return				True if a connection was created. False otherwise.
	 * @error				Bad nav area pointer.
	 */
	public static native bool ConnectToAdjacent(Address from, Address to);
	/**
	 * Disconnects two nav areas.
	 * 
	 * @param from			Address of the first nav area.
	 * @parma to			Address of the second nav area.
	 * @error				Bad nav area pointer.
	 */
	public static native void DisconnectFromAdjacent(Address from, Address to);
	/**
	 * Gets the Nav Area's place human readable name.
	 * 
	 * @param ptr			Address of a nav area.
	 * @param buffer		Char buffer to write the name to.
	 * @param size			Size of the char buffer.
	 * @return				True if the nav area has a name and was written to the buffer. False otherwise.
	 * @error				Bad nav area pointer.
	 */
	public static native bool GetPlaceName(Address ptr, char[] buffer, int size);
	/**
	 * Gets if the nav area is underwater.
	 * Note: This only returns true if the water depth is at player crouch height.
	 * 
	 * @param ptr			Address of a nav area.
	 * @return				True if underwater, false otherwise.
	 * @error				Bad nav area pointer.
	 */
	public static native bool IsUnderwater(Address ptr);
	/**
	 * Gets if the nav area is in shallow water.
	 * Enough to stop fall damage in most games. Generally won't extinguish players on fire.
	 * 
	 * @param ptr			Address of a nav area.
	 * @return				True if yes, false otherwise.
	 * @error				Bad nav area pointer.
	 */
	public static native bool IsInShallowWater(Address ptr);
	/**
	 * Gets if the nav area is in any type of water.
	 * 
	 * @param ptr			Address of a nav area.
	 * @return				True if yes, false otherwise.
	 * @error				Bad nav area pointer.
	 */
	public static native bool IsInWater(Address ptr);
	/**
	 * Checks if the given position is overlapping the nav area's 2D extent.
	 * 
	 * @param ptr			Address of a nav area.
	 * @param pos			World position to check.
	 * @param tolerance		Check tolerance.
	 */
	public static native bool IsOverlappingPos(Address ptr, const float pos[3], const float tolerance = 0.0);
	/**
	 * Checks if  given area (first) and 'other' (second) share a colinear edge (ie: no drop-down or step/jump/climb)
	 * 
	 * @param first			Address of the first nav area.
	 * @param second		Address of the second nav area.
	 * @return				True if yes, false otherwise.
	 * @error				Bad nav area pointer.
	 */
	public static native bool IsContiguous(Address first, Address second);
	/**
	 * Gets the nav area danger value for a given team.
	 * 
	 * @param ptr			Address of a nav area.
	 * @param teamID		Index of the team to get the danger value of. This is required and NAVBOT_NAV_TEAM_ANY is NOT supported!
	 * @return				Nav area danger value. Returns 0 if an invalid team index is passed.
	 * @error				Bad nav area pointer.
	 */
	public static native float GetDanger(Address ptr, int teamID);
	/**
	 * Increases this nav area danger value. Danger decays over time.
	 * Bots will try to avoid these areas when pathfinding.
	 * "FASTEST ROUTE" ignores danger and "SAFEST ROUTE" will priorize avoiding these areas.
	 * 
	 * @param ptr			Address of a nav area.
	 * @param teamID		Index of the team to increase the danger value. Use NAVBOT_NAV_TEAM_ANY to apply to all teams.
	 * @param amount		Amount of danger to add.
	 * @error				Bad nav area pointer.
	 */
	public static native void IncreaseDanger(Address ptr, int teamID, float amount);
	/**
	 * Clears the nav area danger value for a given team.
	 * 
	 * @param ptr			Address of a nav area.
	 * @param teamID		Index of the team to get the danger value of. Use NAVBOT_NAV_TEAM_ANY to apply to all teams.
	 * @error				Bad nav area pointer.
	 */
	public static native void ClearDanger(Address ptr, int teamID);
	/**
	 * Gets the nav area size (X axis)
	 * 
	 * @param ptr			Address of a nav area.
	 * @return				Nav area size (X)
	 * @error				Bad nav area pointer.
	 */
	public static native float GetSizeX(Address ptr);
	/**
	 * Gets the nav area size (Y axis)
	 * 
	 * @param ptr			Address of a nav area.
	 * @return				Nav area size (Y)
	 * @error				Bad nav area pointer.
	 */
	public static native float GetSizeY(Address ptr);
	/**
	 * Checks if the given area is smaller than the given size on either X or Y axis.
	 * 
	 * @param ptr			Address of a nav area.
	 * @param size			Size to check.
	 * @return				True if the nav area is smaller than the given size.
	 * @error				Bad nav area pointer.
	 */
	public static bool IsSmallerThan(Address ptr, float size)
	{
		float sx = NavBotNavArea.GetSizeX(ptr);
		float sy = NavBotNavArea.GetSizeY(ptr);

		if (sx < size || sy < size)
		{
			return true;
		}

		return false;
	}
	/**
	 * Checks if the given area is larger than the given size on either X or Y axis.
	 * 
	 * @param ptr			Address of a nav area.
	 * @param size			Size to check.
	 * @return				True if the nav area is larger than the given size.
	 * @error				Bad nav area pointer.
	 */
	public static bool IsLargerThan(Address ptr, float size)
	{
		float sx = NavBotNavArea.GetSizeX(ptr);
		float sy = NavBotNavArea.GetSizeY(ptr);

		if (sx > size || sy > size)
		{
			return true;
		}

		return false;
	}
	/**
	 * Returns the nav area geometric area (2D)
	 * 
	 * @param ptr			Address of a nav area.
	 * @return				Nav area geometric area (2D).
	 * @error				Bad nav area pointer.
	 */
	public static float GetGeometricArea(Address ptr)
	{
		return NavBotNavArea.GetSizeX(ptr) * NavBotNavArea.GetSizeY(ptr);
	}
	/**
	 * Gets a random position within the nav area bounds.
	 * 
	 * @param ptr			Address of a nav area.
	 * @param pos			Float array to write the position to.
	 * @error				Bad nav area pointer.
	 */
	public static native void GetRandomPoint(Address ptr, float pos[3]);
	/**
	 * Gets the squared distance between the given area and position.
	 * 
	 * @param ptr			Address of a nav area.
	 * @param pos			Position to compute the squared distance to.
	 * @return				Squared distance to the given point.
	 * @error				Bad nav area pointer.
	 */
	public static native float GetDistanceSquaredToPoint(Address ptr, const float point[3]);
	/**
	 * Used by pathfinding, return value only has a meaning when called from a path cost callback.
	 * 
	 * @param ptr			Address of a nav area.
	 * @return				Cost so far.
	 * @error				Bad nav area pointer.
	 */
	public static native float GetCostSoFar(Address ptr);
	/**
	 * Compute largest portal to adjacent area, returning direction.
	 * 
	 * @param from			Address of the first nav area.
	 * @param to			Address of the second nav area.
	 * @param center		Float array to write the portal center to.
	 * @param halfWidth		Portal half width value.
	 * @return				Direction from the first area to the second area.
	 * @error				Bad area address.
	 */
	public static native NavBotNavDirType ComputeLargestPortal(Address from, Address to, float center[3], float& halfWidth);
	/**
	 * Compute portal to adjacent area.
	 * 
	 * @param from			Address of the first area.
	 * @param to			Address of the second area.
	 * @param dir			Direction.
	 * @param center		Float array to write the portal center to.
	 * @param halfWidth		Portal half width value.
	 * @error				Bad area address.
	 */
	public static native void ComputePortal(Address from, Address to, NavBotNavDirType dir, float center[3], float& halfWidth);
	/**
	 * Compute closest point within the "portal" between to adjacent areas.
	 * 
	 * @param from			Address of the first area.
	 * @param to			Address of the second area.
	 * @param dir			Direction.
	 * @param fromPos		Position to compute the closest point to.
	 * @param closePos		Computed closest position in portal.
	 * @error				Bad area address.
	 */
	public static native void ComputeClosestPointInPortal(Address from, Address to, NavBotNavDirType dir, const float fromPos[3], float closePos[3]);
	/**
	 * Return true if there are no bi-directional links on the given side.
	 * 
	 * @param area			Address of a nav area.
	 * @param dir			Direction to check.
	 * @return 				True if there are no bi-directional links on the given side, false otherwise.
	 * @error				Bad area address.
	 */
	public static native bool IsEdge(Address area, NavBotNavDirType dir);
	/**
	 * Computes the area's normal based on the north west corner position.
	 * If 'alternate' is specified, the south east corner is used instead.
	 * 
	 * @param area			Address of a nav area.
	 * @param normal		Float arrat to write the normal to.
	 * @param alternate		If true, uses the south east corner position instead of north west.
	 */
	public static native void ComputeNormal(Address area, float normal[3], const bool alternate = false);
	/**
	 * Searches all areas the given area is connected to.
	 * 
	 * @param area			Address of a nav area.
	 * @param callback		Callback function that will be called for each connected area.
	 * @param data			Optional arbitrary data value to pass through to the callback function.
	 * @return				Returns false if the callback returned false, else return true.
	 * @error				Bad area address, invalid callback function.
	 */
	public static native bool SearchConnectedAreas(Address area, NavBotNavAreaSearchAdjCB callback, any data = 0);
	/**
	 * Gets the address of an off-mesh connection connecting the "source area" to "destination area".
	 * 
	 * @param from			Address of the source area.
	 * @param to			Address of the destination area.
	 * @return				Address of the off-mesh connection. Address_Null if none.
	 * @error				Bad area ptr.
	 */
	public static native Address GetOffMeshConnectionToArea(Address from, Address to);
	/**
	 * Gets the address of a ladder connecting the "source area" to "destination area".
	 * 
	 * @param from			Address of the source area.
	 * @param to			Address of the destination area.
	 * @return				Address of the ladder. Address_Null if none.
	 * @error				Bad area ptr.
	 */
	public static native Address GetLadderConnectionToArea(Address from, Address to);
	/**
	 * Gets the address of a elevator connecting the "source area" to "destination area".
	 * 
	 * @param from			Address of the source area.
	 * @param to			Address of the destination area.
	 * @return				Address of the elevator. Address_Null if none.
	 * @error				Bad area ptr.
	 */
	public static native Address GetElevatorConnectionToArea(Address from, Address to);
	/**
	 * Creates an off-mesh connection between the "from" area and the "to" area.
	 * Notes: 
	 *   Off-mesh connections are one-way!
	 *   Bots don't use pathfinding and obstacle avoidance when using off-mesh connections. Make sure the start position is reachable from the "from" area directly.
	 *   Off-mesh connections only works correctly after the nav mesh is analyzed.
	 * 
	 * @param from			Address of the source area.
	 * @param to			Address of the destination area.
	 * @param type			Off-mesh connection type.
	 * @param start			Off-mesh connection start position.
	 * @param end			Off-mesh connection end position.
	 * @return				True if a connection was created, false otherwise.
	 * @error				Bad area ptr, invalid off-mesh type passed.
	 */
	public static native bool ConnectToOffMesh(Address from, Address to, OffMeshConnectionType type, const float start[3], const float end[3]);
	/**
	 * Removes an off-mesh connection between the "from" area and the "to" area.
	 * 
	 * @param from			Address of the source area.
	 * @param to			Address of the destination area.
	 * @param type			Off-mesh connection type.
	 * @error				Bad area ptr, invalid off-mesh type passed.
	 */
	public static native void DisconnectOffMesh(Address from, Address to, OffMeshConnectionType type);
	/**
	 * Gets the number of incoming one-ways to this nav area.
	 * 
	 * @param area			Address of a nav area.
	 * @param dir			Direction to get the incoming one-ways count.
	 * @return				Number of incoming one-ways in the specified direction.
	 * @error				Bad area ptr, invalid direction.
	 */
	public static native int GetIncomingConnectionsCount(Address area, NavBotNavDirType dir);
	/**
	 * Gets a pointer to an incoming one-way nav area to the given area.
	 * 
	 * @param area			Address of a nav area.
	 * @param dir			Direction to get the incoming one-ways.
	 * @param index			Index of the incoming one-way vector to read. Values are between 0 and GetIncomingConnectionsCount() - 1.
	 * @return				Address of a nav area with an one-way connection to the area passed in the first param.
	 * @error				Bad area ptr, invalid direction, out of range index passed.
	 */
	public static native Address GetIncomingConnection(Address area, NavBotNavDirType dir, const int index);
	/**
	 * Compute change in actual ground height from the "from" area to the "to" area.
	 * 
	 * @param from			From area address.
	 * @param to			To area address.
	 * @return				Computed ground height change.
	 */
	public static native float ComputeGroundHeightChange(Address from, Address to);
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