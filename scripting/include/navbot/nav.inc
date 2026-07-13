#if defined _navbot_nav_included
 #endinput
#endif
#define _navbot_nav_included

enum NavBotNavDirType
{
	NAVBOT_NAV_DIR_NORTH = 0,
	NAVBOT_NAV_DIR_EAST = 1,
	NAVBOT_NAV_DIR_SOUTH = 2,
	NAVBOT_NAV_DIR_WEST = 3,

	NAVBOT_NAV_DIR_NUM_DIRECTIONS,
};

/**
 * Defines possible ways to move from one area to another
 */
enum NavBotNavTraverseType
{
	// NOTE: First 4 directions MUST match NavDirType
	NAVBOT_NAV_TT_GO_NORTH = 0,
	NAVBOT_NAV_TT_GO_EAST,
	NAVBOT_NAV_TT_GO_SOUTH,
	NAVBOT_NAV_TT_GO_WEST,

	NAVBOT_NAV_TT_GO_LADDER_UP,
	NAVBOT_NAV_TT_GO_LADDER_DOWN,
	NAVBOT_NAV_TT_GO_JUMP,
	NAVBOT_NAV_TT_GO_ELEVATOR_UP,
	NAVBOT_NAV_TT_GO_ELEVATOR_DOWN,

	NAVBOT_NAV_TT_GO_OFF_MESH_CONNECTION,

	NAVBOT_NAV_TT_NUM_TRAVERSE_TYPES
};

enum NavBotNavCornerType
{
	NAVBOT_NAV_CORNER_NORTH_WEST = 0,
	NAVBOT_NAV_CORNER_NORTH_EAST = 1,
	NAVBOT_NAV_CORNER_SOUTH_EAST = 2,
	NAVBOT_NAV_CORNER_SOUTH_WEST = 3,

	NAVBOT_NAV_CORNER_NUM_CORNERS
};

// In the nav mesh code, this value is used to signal 'ANY' team.
#define NAVBOT_NAV_TEAM_ANY -2

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

enum NavBotRouteType
{
	NAVBOT_DEFAULT_ROUTE = 0, // Default route type
	NAVBOT_FASTEST_ROUTE, // Fastest route to the destination, may avoid areas that slow down the bots, doesn't care about danger.
	NAVBOT_SAFEST_ROUTE, // Safest route, will try to avoid dangerous areas
};
