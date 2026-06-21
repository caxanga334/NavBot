#if defined _navbot_navmesh_collector_included
 #endinput
#endif
#define _navbot_navmesh_collector_included

typedef NavBotNavCollectorNavAreaCB = function bool (Address area);

// Collects nav areas via Breadth-first search.
methodmap NavBotNavAreaCollector < Handle
{
	public native NavBotNavAreaCollector();
	// Initial search area.
	public native void SetSearchStartArea(Address area);

	// Sets the ShouldSearch callback
	property NavBotNavCollectorNavAreaCB ShouldSearch {
		public native set(NavBotNavCollectorNavAreaCB func);
	}
	// Sets the ShouldCollect callback
	property NavBotNavCollectorNavAreaCB ShouldCollect {
		public native set(NavBotNavCollectorNavAreaCB func);
	}
	// Gets/Sets the maximum the travel limit (maximum collect distance)
	property float TravelLimit {
		public native set(float limit);
		public native get();
	}
	// Sets if the collector should search areas connected via ladders
	property bool SearchLadder {
		public native set(bool search);
	}
	// Sets if the collector should search areas connected via off-mesh connections
	property bool SearchOffMeshLinks {
		public native set(bool search);
	}
	// Sets if the collector should search areas connected via elevators
	property bool SearchElevators {
		public native set(bool search);
	}
	// Sets if the collector should search incoming areas (areas with one-way connection to the current area)
	property bool SearchIncoming {
		public native set(bool search);
	}
	// Runs the search.
	public native void Execute();
	// Resets the collector for a new search.
	public native void Reset();
	// Forces the search to end early. This should be called from either ShouldSearch or ShouldCollect callbacks.
	public native void EndSearch();
	// Gets an address to a NavBotNavAreaSearchNode of the given nav area. Returns Address_Null if the given area doesn't have a node.
	public native Address GetSearchNodeForArea(Address area);
	// Number of nav areas collected.
	property int CollectedAreaSize {
		public native get();
	}
	// Returns true if the collected areas vector is empty.
	public bool IsCollectedAreasEmpty()
	{
		return this.CollectedAreaSize == 0;
	}
	// Returns a random collected area.
	public native Address GetRandomCollectedArea();
	// Returns the farthest (in travel distance) collected area.
	public native Address GetFarthestCollectedArea();
	// Returns the nearest (in travel distance) collected area.
	public native Address GetNearestCollectedArea();
	/**
	 * Gets a vector of the collected areas. The handle must be deleted.
	 * The vector is a copy and the collector's handle can be deleted without invalidating the vector.
	 * 
	 * @return			Nav Area Vector Handle.
	 * @error			Invalid collector handle.
	 */
	public native NavBotNavAreaVector GetCollectedAreas();
}

methodmap NavBotNavAreaSearchNode
{
	// The nav area this node belongs to.
	public static native Address GetArea(Address node);
	// Gets the node's parent area.
	public static native Address GetParent(Address node)
	// Travel cost to reach this node from the parent area.
	public static native float GetTravelCostFromParent(Address node);
	// Travel cost to reach this node from the start area.
	public static native float GetTravelCostFromStart(Address node);
}