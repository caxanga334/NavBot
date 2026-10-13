#if defined _navbot_nav_astar_search_included
 #endinput
#endif
#define _navbot_nav_astar_search_included

/**
 * Callback for the A* search g cost.
 * 
 * @param toArea		Address of the nav area the search is going to.
 * @param fromArea		Address of the nav area the search is incoming from.
 * @param ladder		Ladder connecting the two areas. Address_Null if not connected by ladder.
 * @param offmeshconn	Off-Mesh connection that connects the two areas. Address_Null if not connected by Off-Mesh connection.
 * @param elevator		Elevator connecting the two areas. Address_Null if not connected by elevator.
 * @return				Cost to go from "fromArea" to "toArea". Return -1 for dead ends.
 */
typedef NavBotNavAStarGCostFunc = function float (Address toArea, Address fromArea, Address ladder, Address offmeshconn, Address elevator);
/**
 * Callback for the A* search h cost.
 * 
 * @param area			Address of the nav area.
 * @param goal			Search goal position.
 * @return				Heuristic cost of the area to the search goal.
 */
typedef NavBotNavAStarHCostFunc = function float (Address area, const float goal[3]);

methodmap NavBotNavAStarSearch < Handle
{
    // Constructs a new a star search object.
    public native NavBotNavAStarSearch();
    // Sets the G cost callback.
    property NavBotNavAStarGCostFunc GCostCallback {
        public native set(NavBotNavAStarGCostFunc callback);
    }
    // Sets the H cost callback.
    property NavBotNavAStarHCostFunc HCostCallback {
        public native set(NavBotNavAStarHCostFunc callback);
    }
    // Sets the search start area
    property Address StartArea {
        public native set(Address ptr);
    }
    // Sets the goal world position.
    public native void SetGoalPosition(const float goal[3]);
    // Total cost from the start to the goal.
    property float TotalCost {
        public native get();
    }
    // Returns true if the search found a complete path.
    property bool FoundPath {
        public native get();
    }
    /**
     * Gets the NavBotNavAStarSearchNode pointer of a nav area.
     * 
     * @param area          Address of the area to get the search node of.
     * @return              Search node ptr. Can return NULL.
     * @error               Bad area ptr.
     */
    public native Address GetNodeForArea(Address area);
    /**
     * Gets a nav area vector containg the areas in the path.
     * The vector starts from the end of the path.
     * Note: The vector is a copy and the search handle can be deleted without issues.
     * 
     * @return              Handle of a nav area vector. Must be deleted.
     */
    public native NavBotNavAreaVector GetPath();
    /**
     * Runs the A* search.
     * Both callbacks must be set.
     */
    public native void RunSearch();
}

methodmap NavBotNavAStarSearchNode
{
    /**
     * Gets the nav area this node belongs to.
     * 
     * @param node          Address of a search node.
     * @return              Address of the nav area the node belongs to.
     * @error               Bad node address.
     */
    public static native Address GetArea(Address node);
    /**
     * Gets the parent node.
     * 
     * @param node          Address of a search node.
     * @return              Address of the parent node.
     * @error               Bad node address.
     */
    public static native Address GetParent(Address node);
    /**
     * Gets the traverse method used to reach this node from the parent.
     * 
     * @param node          Address of a search node.
     * @return              Traverse method.
     * @error               Bad node address.
     */
    public static native NavBotNavTraverseType GetTraverseType(Address node);
}